# UART 串口子系统架构说明

> 适用于 STM32F405，双串口（UART4 / UART5），FreeRTOS + CMSIS-RTOS2

---

## 目录

- [分层结构](#分层结构)
- [文件清单](#文件清单)
- [接收数据流](#接收数据流)
- [发送数据流（双缓冲）](#发送数据流双缓冲)
- [DMA 循环模式](#dma-循环模式)
- [关键结构体](#关键结构体)
- [内存占用](#内存占用)
- [如何新增一个串口](#如何新增一个串口)
- [配置项速查](#配置项速查)
- [RTOS 对象](#rtos-对象)

---

## 分层结构

```
┌───────────────────────────────────────────┐
│              TASK 层                       │
│  [task_uart_rx.c](..\User\TASK\task_uart_rx.c)         │
│  [task_uart_rx_cmd.c](..\User\TASK\task_uart_rx_cmd.c) │
│  [task_uart_tx.c](..\User\TASK\task_uart_tx.c)         │
├───────────────────────────────────────────┤
│              APP 层                        │
│  [app_uart.h](..\User\APP\APP_Uart\app_uart.h)       │
│  [app_uart.c](..\User\APP\APP_Uart\app_uart.c)       │
│  [app_uart_cmd.h](..\User\APP\APP_Uart\app_uart_cmd.h) │
│  [app_uart_cmd.c](..\User\APP\APP_Uart\app_uart_cmd.c) │
├───────────────────────────────────────────┤
│              BSP 层                        │
│  [bsp_uart.h](..\User\BSP\BSP_Uart\bsp_uart.h)       │
│  [bsp_uart.c](..\User\BSP\BSP_Uart\bsp_uart.c)       │
├───────────────────────────────────────────┤
│            Middleware                      │
│  [codec.h](..\User\Middlewares\Codec\codec.h)        │
│  [codec.c](..\User\Middlewares\Codec\codec.c)        │
└───────────────────────────────────────────┘
```

**BSP** – 薄硬件层，编译期映射表 (id ↔ huart)，不做缓冲管理。  
**APP** – 所有缓冲、调度、协议解析都在此。  
**TASK** – 三个 RTOS 任务，各司其职。  
**Codec** – 数据包格式（帧头/校验/粘包处理）。

---

## 文件清单

### BSP 层

| 文件 | 职责 |
|---|---|
| [bsp_uart.h](..\User\BSP\BSP_Uart\bsp_uart.h) | API 声明：Init / ConfigDMARx / GetDMAPos / SendDMA / IsTxBusy / SetTxCpltFn |
| [bsp_uart.c](..\User\BSP\BSP_Uart\bsp_uart.c) | 映射表 `BSP_UART_MAP`，DMA 配置，三个 RX 中断入口，TX 完成中断 + 用户回调 |

### APP 层

| 文件 | 职责 |
|---|---|
| [app_uart.h](..\User\APP\APP_Uart\app_uart.h) | 结构体定义（端口上下文/描述符/帧/DMA缓冲），API 声明 |
| [app_uart.c](..\User\APP\APP_Uart\app_uart.c) | 核心逻辑：DMA 数据读取、Codec 解包、共享池管理、双缓冲发送 |
| [app_uart_cmd.h](..\User\APP\APP_Uart\app_uart_cmd.h) | `APP_UART_Cmd()` 声明 |
| [app_uart_cmd.c](..\User\APP\APP_Uart\app_uart_cmd.c) | 命令分发实现 |

### TASK 层

| 文件 | 职责 |
|---|---|
| [task_uart_rx.c](..\User\TASK\task_uart_rx.c) | 等待 BSP 中断通知 → `ProcessRxData()` → 发信号量唤醒 CMD 任务 |
| [task_uart_rx_cmd.c](..\User\TASK\task_uart_rx_cmd.c) | 等待信号量 → 从描述符 FIFO 弹出 → `APP_UART_Cmd()` |
| [task_uart_tx.c](..\User\TASK\task_uart_tx.c) | 等待队列 → 区分数据帧/标记帧 → `TrySendDual()` / `OnTxComplete()` |

### Middleware

| 文件 | 职责 |
|---|---|
| [codec.h](..\User\Middlewares\Codec\codec.h) | `Codec_ParseRxPacket()` 解包，`Codec_BuildTxPacket()` 打包 |
| [codec.c](..\User\Middlewares\Codec\codec.c) | 帧格式：`0xFF 0xAA LEN CMD [PAYLOAD] CHK` |

---

## 接收数据流

```
UART 硬件 → DMA (Circular, 永不停止)
  ↓ HT/TC/IDLE 中断
BSP ISR: osMessageQueuePut(UART_RX_QHandle, &id)   ← 零拷贝
  ↓
Task_UART_RX: osMessageQueueGet()
  → APP_UART_ProcessRxData(id)
  ↓  ① BSP_UART_GetDMAPos() 算出 DMA 写到哪里
  ↓  ② app_read_dma_data() 从 DMA 缓冲拷贝新字节到 rx_buf
  ↓  ③ Codec_ParseRxPacket() 从 rx_buf 解析完整包
  ↓  ④ 完整包 payload → 共享数据池 (app_rx_pool)
  ↓  ⑤ 描述符 {offset, len, cmd, id} → rx_desc_fifo
  → osSemaphoreRelease(UART_RX_BS)
  ↓
Task_UART_RX_CMD: osSemaphoreAcquire()
  → APP_UART_SendRxPacket()
  ↓  ① rx_desc_pop() 弹出描述符
  ↓  ② app_rx_pool_read() 从共享池读 payload
  ↓  ③ 推进池读指针 app_rx_pool_advance()
  → APP_UART_Cmd(cmd, payload, len)
```

### 关键函数调用链

`Task_UART_RX` → [`APP_UART_ProcessRxData`](..\User\APP\APP_Uart\app_uart.c#L297)():
- [`app_read_dma_data()`](..\User\APP\APP_Uart\app_uart.c#L198) — 从 DMA 环形缓冲直接拷贝到 rx_buf，处理回绕
- [`Codec_ParseRxPacket()`](..\User\Middlewares\Codec\codec.c#L33) — 解析完整数据包
- [`app_rx_pool_write()`](..\User\APP\APP_Uart\app_uart.c#L70) — payload 写入共享池
- [`rx_desc_push()`](..\User\APP\APP_Uart\app_uart.c#L133) — 描述符入 FIFO

`Task_UART_RX_CMD` → [`APP_UART_SendRxPacket`](..\User\APP\APP_Uart\app_uart.c#L391)():
- [`rx_desc_pop()`](..\User\APP\APP_Uart\app_uart.c#L150) — 描述符出 FIFO
- [`app_rx_pool_read()`](..\User\APP\APP_Uart\app_uart.c#L98) — 从池读 payload
- [`app_rx_pool_advance()`](..\User\APP\APP_Uart\app_uart.c#L117) — 释放已消费空间

---

## 发送数据流（双缓冲）

```
APP_UART_BuildTxPacket(id, cmd, data, len)
  → Codec_BuildTxPacket() 打包
  → 编码为线缆格式
  → osMessageQueuePut(UART_TX_QHandle)     ← 入队
  ↓
Task_UART_TX: osMessageQueueGet()
  ↓
├── 数据帧 (len > 0):  APP_UART_TrySendDual(&frame)
│   └── app_dual_try_send()
│       ├── DMA 空闲 → 拷贝到 tx_buf[0] → BSP_UART_SendDMA()
│       ├── DMA 忙、预装空闲 → 拷贝到 tx_buf[1] → preloaded=1
│       └── 双缓冲都忙 → 放回队尾
│
└── 标记帧 (len == 0):  APP_UART_OnTxComplete(id)
    └── 尝试从队列取帧 → app_dual_try_send() 装填空闲缓冲
              ↑
              └── ISR 回调 on_uart_tx_complete()
                   → 有预装 → 切缓冲 → 启动下一轮 DMA
                   → osMessageQueuePut(标记帧) 唤醒 TX 任务
```

### 双缓冲工作图

```
时间 ──────────────────────────────────────→
     buf[0] DMA发送中          buf[0] DMA完成 → ISR切到buf[1]
     buf[1] 任务预装中          buf[1] 开始发送, buf[0] 被任务重新装填
     ─────────────────────────────────────────────────────────
     帧间零间隙 ──── 两缓冲乒乓，ISR 自动切换
```

### 关键函数

- [`APP_UART_TrySendDual()`](..\User\APP\APP_Uart\app_uart.c#L530) — TX 任务入口，尝试发/预装
- [`app_dual_try_send()`](..\User\APP\APP_Uart\app_uart.c#L496) — 内部双缓冲状态机
- [`on_uart_tx_complete()`](..\User\APP\APP_Uart\app_uart.c#L465) — ISR 回调，切缓冲 + 发标记帧
- [`APP_UART_OnTxComplete()`](..\User\APP\APP_Uart\app_uart.c#L530) — 任务收到标记帧后装填空闲缓冲

---

## DMA 循环模式

### 原理

传统模式（`DMA_NORMAL`）每次收满指定字节后 DMA **停止**，需手动重启，重启窗口的数据会丢失。

循环模式（`DMA_CIRCULAR`）的 DMA **永不停止**，写满末尾后自动回绕到开头继续写。APP 通过 DMA 的 Counter 寄存器计算写位置：

```c
写入位置 = 缓冲大小 - __HAL_DMA_GET_COUNTER(huart->hdmarx)
```

代码实现在 [`BSP_UART_GetDMAPos()`](..\User\BSP\BSP_Uart\bsp_uart.c#L181)。

### 三种中断

| 中断 | 触发条件 | 作用 |
|---|---|---|
| HT (半满) | DMA 写到缓冲正中间 | 任务读前半段，DMA 写后半段 |
| TC (全满) | DMA 写到缓冲末尾 | 任务读后半段，DMA 回绕写前半段 |
| IDLE | 总线空闲一个字节周期 | 一包数据结束，即时处理 |

三者均触发 [`BSP_UART_OnRxEvent()`](..\User\BSP\BSP_Uart\bsp_uart.c#L248)，只做 `osMessageQueuePut(id)`，零拷贝。

### 位置追踪与回绕

```
上次读位置 (rx_last_pos)    当前写位置 (cur_pos)
       │                      │
       ▼                      ▼
  DMA缓冲 [0◄═══已读════last═══新数据═══cur_pos═══未写►511]
       
  顺序情况 (cur_pos > last): 直接拷贝 last ~ cur_pos
  回绕情况 (cur_pos < last): 分两段拷贝 last~末尾 + 开头~cur_pos
```

代码实现在 [`app_read_dma_data()`](..\User\APP\APP_Uart\app_uart.c#L198)。

---

## 关键结构体

### [`APP_UART_Port_t`](..\User\APP\APP_Uart\app_uart.h#L75) — 每端口上下文

```c
typedef struct {
    uint8_t  id;                            // 串口编号
    uint8_t  rx_dma_buf[512];               // DMA 循环缓冲（APP 拥有）
    uint16_t rx_last_pos;                   // 上次处理到的 DMA 位置
    uint8_t  rx_buf[256];                   // 拼包缓冲
    uint16_t rx_len;                        // 拼包缓冲有效长度
    uint32_t last_rx_time;                  // 最后接收时间戳
} APP_UART_Port_t;
```

### [`APP_TxFrame_t`](..\User\APP\APP_Uart\app_uart.h#L66) — 发送帧

```c
typedef struct {
    uint8_t id;                         // 目标串口
    uint8_t len;                        // 帧长度
    uint8_t data[128];                  // 编码后完整帧
} APP_TxFrame_t;
```

特殊用法：`len == 0` 作为标记帧（ISR → 任务，表示 TX 完成）。

### [`APP_RxDesc_t`](..\User\APP\APP_Uart\app_uart.h#L43) — 接收描述符

```c
typedef struct {
    uint16_t offset;    // 在共享池中的偏移
    uint8_t  len;       // payload 长度
    uint8_t  cmd;       // 命令字
    uint8_t  id;        // 来源串口
} APP_RxDesc_t;          // 仅 6 字节
```

替代原来的 `Codec_Packet_t buffer[11]`（每个 129 字节），**RAM 节省约 1.3KB**。

### [`APP_TxDualBuf_t`](..\User\APP\APP_Uart\app_uart.c#L49) — 双缓冲上下文

```c
typedef struct {
    uint8_t  buf[2][128];    // ping-pong 发送缓冲
    uint8_t  len[2];         // 各缓冲数据长度
    uint8_t  active;         // 当前 DMA 读哪个缓冲
    uint8_t  preloaded;      // 另一缓冲是否有预装数据
    volatile uint8_t in_progress;  // 是否有 DMA 正在进行
} APP_TxDualBuf_t;
```

---

## 内存占用

> 数据基于 STM32F405, -O0, arm-none-eabi-gcc 14.3.1。  
> 项目总计 RAM 25,736 B / 128 KB (19.6%), FLASH 61,308 B / 1 MB (5.9%)。

### 总览

| | UART 子系统 | 项目占比 |
|---|---|---|
| **RAM** | ~10,400 B | ~40% |
| **FLASH** | 4,792 B | 7.8% |

### RAM 详细

#### 任务栈 (2,048 B)

| 对象 | 大小 |
|---|---|
| `UART_RX_TBuffer` | 1,024 B (256 words) |
| `UART_TX_TBuffer` | 512 B (128 words) |
| `UART_RX_CMDBuffer` | 512 B (128 words) |

#### APP 数据缓冲 (6,304 B)

| 符号 | 大小 | 用途 |
|---|---|---|
| `app_rx_pool` | 4,096 B | 共享接收数据池 (环形字节池) |
| `app_port_pool` | 1,560 B | 2 × `APP_UART_Port_t` (DMA 512B + 拼包 256B 每端口) |
| `tx_dual_pool` | 522 B | 2 × 双缓冲上下文 (2×128B + 状态) |
| `rx_desc_fifo` | 122 B | 共享描述符 FIFO (20 槽 × 6B + 索引) |
| BSS 小量 | ~4 B | 全局变量、池指针 |

#### RTOS 对象 (2,100 B)

| 符号 | 大小 |
|---|---|
| `UART_TX_QBuffer` | 2,080 B (16 × 130B) |
| `UART_RX_QBuffer` | 16 B (16 × uint8_t) |
| `UART_RX_BSHandle` | 4 B |

### FLASH 详细 (4,792 B)

| 文件 | 代码量 |
|---|---|
| `app_uart.c` | 2,914 B |
| `bsp_uart.c` | 908 B |
| `codec.c` | 596 B |
| `app_uart_cmd.c` | 186 B |
| `task_uart_rx_cmd.c` | 68 B |
| `task_uart_tx.c` | 60 B |
| `task_uart_rx.c` | 60 B |

---



## 如何新增一个串口

以增加 UART3（ID=3）为例：

### 1. BSP 映射表

[bsp_uart.c](..\User\BSP\BSP_Uart\bsp_uart.c#L55) 加一项：

```c
static const BSP_UART_Map_t BSP_UART_MAP[BSP_UART_MAX_NUM] = {
    { .id = 1, .huart = &huart4 },
    { .id = 2, .huart = &huart5 },
    { .id = 3, .huart = &huart3 },   // ← 新增
};
```

同时把 `BSP_UART_MAX_NUM` 改为 3。

### 2. APP 端口列表

[app_uart.c](..\User\APP\APP_Uart\app_uart.c#L37) 加一项：

```c
static const uint8_t APP_UART_PORT_IDS[] = { 1, 2, 3 };
```

### 3. CubeMX

- 使能 UART3 + DMA（RX circular, TX normal）
- 在 freertos.c 的 `UART_RX_QHandle` 之类的队列深度确认够用（16 个 id 一般够）

---

## 配置项速查

所有可调参数集中在 [app_uart.h](..\User\APP\APP_Uart\app_uart.h#L28)：

| 宏 | 默认值 | 说明 |
|---|---|---|
| `APP_RX_DMA_BUF_SIZE` | 512 | DMA 循环缓冲大小（每端口），越大响应越宽松 |
| `APP_RX_BUF_MAX` | 256 | 拼包缓冲大小 |
| `APP_RX_POOL_SIZE` | 4096 | 共享数据池，够存约 32 个最大帧 payload |
| `APP_RX_DESC_MAX` | 20 | 描述符 FIFO 深度 |
| `APP_RX_TIMEOUT_MS` | 50 | 半包超时，超过此时间未收全则丢弃 |
| `APP_TX_BUF_SIZE` | 128 | 发送帧最大长度，应 ≥ `MAX_PACKET_LEN` |

BSP 配置在 [bsp_uart.h](..\User\BSP\BSP_Uart\bsp_uart.h#L32)：

| 宏 | 默认值 | 说明 |
|---|---|---|
| `BSP_UART_MAX_NUM` | 2 | 串口实例数量 |

---

## RTOS 对象

| 对象 | 类型 | 用途 |
|---|---|---|
| `UART_RX_QHandle` | msg queue, 16 × uint8_t | BSP ISR → RX 任务（数据就绪通知） |
| `UART_TX_QHandle` | msg queue, 16 × APP_TxFrame_t | 数据帧 + 标记帧（双缓冲通知） |
| `UART_RX_BSHandle` | binary semaphore | RX 任务 → CMD 任务（包已解析通知） |
| `Task_UART_RX` | task, 256 words stack | DMA 数据读取 + 协议解析 |
| `Task_UART_RX_CMD` | task, 128 words stack | 命令分发（`APP_UART_Cmd`） |
| `Task_UART_TX` | task, 128 words stack | 双缓冲发送调度 |

---

> 最后更新：2026-06-20
