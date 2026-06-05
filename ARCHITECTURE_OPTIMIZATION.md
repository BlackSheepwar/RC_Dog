# RC Dog 程序架构优化指南

> **目标：** 将"搭积木"代码重构为高内聚、低耦合、可移植、可维护的嵌入式软件架构
> **适用项目：** Gateway (STM32F103) + FrontDog (STM32F405) — 三MCU舵机狗控制系统
> **作者注：** 本文档旨在成为你的第一个"完整作品级"项目的架构蓝图

---

## 目录

1. [现状评估](#1-现状评估)
2. [第一阶段：基础规范（必须先做）](#2-第一阶段基础规范必须先做)
3. [第二阶段：架构分层与模块化](#3-第二阶段架构分层与模块化)
4. [第三阶段：引入成熟架构模式](#4-第三阶段引入成熟架构模式)
5. [第四阶段：提高移植性](#5-第四阶段提高移植性)
6. [第五阶段：工程化与质量保障](#6-第五阶段工程化与质量保障)
7. [Roadmap 时间线建议](#7-roadmap-时间线建议)
8. [附录：参考与推荐阅读](#8-附录参考与推荐阅读)

---

## 1. 现状评估

### 1.1 当前架构概览

```
┌─────────────────────────────────────────────────────┐
│                   RTOS Tasks (OS Layer)              │
│  tsak_key  tsak_par  tsak_rx  tsak_tx  tsak_can_*   │
│  tsak_se  task_oled                                  │
├─────────────────────────────────────────────────────┤
│              APP Layer (Application Logic)           │
│  APP_CAN  APP_Key  APP_SE  APP_Usart  APP_Knob      │
│  APP_Operation  APP_CAN_CMD  APP_Key_CMD             │
│  APP_Usart_CMD                                       │
├─────────────────────────────────────────────────────┤
│              BSP Layer (Board Support)               │
│  BSP_CAN  BSP_Key  BSP_SE  BSP_Usart  BSP_ADC       │
├─────────────────────────────────────────────────────┤
│         Middlewares (Platform-Independent)           │
│  Debounce  Codec  OLED  Protocol                     │
├─────────────────────────────────────────────────────┤
│     HAL / CMSIS / FreeRTOS (Vendor & 3rd-Party)     │
└─────────────────────────────────────────────────────┘
```

### 1.2 做得好的地方 ✅

| 优点 | 说明 |
|------|------|
| 静态分配 | 所有 RTOS 对象静态分配，无动态内存碎片风险 |
| BSP/APP 分离 | 已有基本分层意识，BSP 封装硬件操作 |
| 中间件独立 | Debounce、Codec 等不依赖 MCU 平台 |
| 队列驱动通信 | 任务间通过消息队列解耦，ISR 不直接调用任务逻辑 |
| CMake 构建 | 比 IDE 工程更具可移植性 |
| HW_VERSION 宏 | FrontDog 支持前后腿同一代码编译 |

### 1.3 问题总结 ❌

| 问题 | 严重程度 | 具体表现 |
|------|----------|----------|
| 命名混乱 | 🔴 高 | `tsak`/`task` 混用、`bsp_ADC` 大小写不统一、文件名风格不一致 |
| 目录结构扁平 | 🔴 高 | OS Layer 所有任务文件在同一个目录，APP 按外设而不是按功能组织 |
| 任务-APP 紧耦合 | 🔴 高 | Task 函数中混合调用 APP 和 BSP 初始化，职责不清晰 |
| 代码重复 | 🟡 中 | Gateway 和 FrontDog 之间大量重复代码（common/protocol/debounce/codec/oled 完全重复） |
| 错误处理薄弱 | 🟡 中 | 大部分函数无返回值检查，错误未被处理或上报 |
| 回调分散 | 🟡 中 | HAL 回调在 BSP 中直接操作队列，ISR 和业务逻辑未完全隔离 |
| 缺乏统一配置 | 🟢 低 | 魔数散落在各处（角度限幅、滤波器配置、任务周期等） |
| 缺乏文档注释 | 🟢 低 | 大部分函数无 Doxygen 注释 |

---

## 2. 第一阶段：基础规范（必须先做）

> **时间估计：1-2 天**
> **原则：** 不动业务逻辑，只做规范化整理

### 2.1 统一命名规范

建立并遵守以下命名规则：

```
模块           前缀        示例
──────────────────────────────────────────
任务           Task_       Task_KeyProcess, Task_CAN1F0
应用层         APP_        APP_CAN_RegisterPort
BSP 层         BSP_        BSP_CAN_Init
中间件         (无前缀)    Debounce_Update, Codec_Parse
回调函数       onXxx       onCANFrameReceived, onUartDataReady
类型定义       _t          can_port_t, servo_angle_t
宏定义        大写         CAN_DEFAULT_BAUDRATE, SERVO_ANGLE_MIN
私有函数       static      prvCalculateChecksum, prvFindPortById
```

### 2.2 规范文件结构

每个源文件/头文件遵循固定模板：

```c
/**
 * @file    bsp_can.c
 * @brief   CAN bus board support package
 * @author  Your Name
 * @date    2026-06-05
 *
 * @note    Implements CAN hardware abstraction for bxCAN peripheral.
 *          Supports up to BSP_CAN_MAX_INSTANCES instances.
 */

/* Includes ---------------------------------------------------------------- */
#include "bsp_can.h"
#include "FreeRTOSConfig.h"
/* ...其他头文件 */

/* Private defines --------------------------------------------------------- */
#define BSP_CAN_MAX_INSTANCES   2
#define BSP_CAN_TIMEOUT_MS     100

/* Private macros ---------------------------------------------------------- */

/* Private types ----------------------------------------------------------- */

/* Private variables ------------------------------------------------------- */

/* Private function prototypes --------------------------------------------- */

/* Exported functions ------------------------------------------------------ */

/* Private functions ------------------------------------------------------- */
```

### 2.3 统一目录结构

重构后的目录结构（以 Gateway 为例）：

```
User/
├── App/
│   ├── Can/
│   │   ├── app_can.c          # CAN 应用层 (原 APP_CAN)
│   │   └── app_can_cmd.c      # CAN 命令分发 (原 APP_CAN_CMD)
│   ├── Key/
│   │   ├── app_key.c          # 按键应用
│   │   └── app_key_cmd.c      # 按键命令映射
│   ├── Usart/
│   │   ├── app_usart.c
│   │   └── app_usart_cmd.c
│   ├── Servo/                 # 仅 FrontDog
│   │   └── app_servo.c
│   ├── Knob/
│   │   └── app_knob.c
│   └── Operation/             # 仅 Gateway
│       └── app_operation.c
│
├── Bsp/
│   ├── bsp_can.c
│   ├── bsp_key.c
│   ├── bsp_usart.c
│   ├── bsp_adc.c
│   └── bsp_servo.c            # 仅 FrontDog (原 BSP_SE)
│
├── Middleware/
│   ├── debounce.c/h
│   ├── codec.c/h
│   ├── protocol.c/h
│   └── oled.c/h               # 放在此目录 (跨平台中间件)
│
├── Common/
│   ├── common.h               # 类型定义、错误码、基础宏
│   ├── error.h                # 统一的错误处理
│   └── assert.h               # 断言和调试辅助
│
├── Task/
│   ├── task_key.c
│   ├── task_key_process.c     # 原 task_par
│   ├── task_can_f0.c
│   ├── task_can_f1.c
│   ├── task_usart_rx.c
│   ├── task_usart_tx.c
│   ├── task_servo.c           # 仅 FrontDog (原 tsak_se)
│   └── task_oled.c
│
└── Hal/                       # HAL 抽象层 (新增)
    ├── hal_can.c/h            # 硬件无关的 CAN 接口
    └── hal_timer.c/h          # 硬件无关的定时器接口
```

### 2.4 立即执行清单

```markdown
- [ ] 统一所有文件名（大小写、命名风格）
- [ ] 统一所有头文件注释模板
- [ ] 给所有公开函数添加 Doxygen 注释
- [ ] 给所有重要宏添加注释
- [ ] 删除无用注释代码（如被注释的源文件引用）
- [ ] 确保每行末尾无空格、缩进统一
- [ ] 将 Gateway 和 FrontDog 通用代码提取到独立仓库或符号链接
```

---

## 3. 第二阶段：架构分层与模块化

> **时间估计：3-5 天**
> **目标：** 消除重复、明确职责、建立清晰分层

### 3.1 建立 Three-Layer 架构

参考嵌入式标准的**三层架构**：

```
┌──────────────────────────────────────────────────────────────┐
│                    应用层 (Application)                       │
│  职责：实现业务逻辑，不关心硬件                                 │
│  包含：App_ 模块、Task_ 模块                                  │
│  依赖：只依赖接口层，不直接依赖 BSP 或 HAL                      │
├──────────────────────────────────────────────────────────────┤
│                    接口层 (Interface)                         │
│  职责：定义业务需要的硬件接口抽象                               │
│  包含：Hal_ 抽象接口（结构体函数指针或虚表）                     │
│  特点：是「契约」，不包含实现                                  │
├──────────────────────────────────────────────────────────────┤
│                    硬件层 (Hardware)                          │
│  职责：实现真正的硬件操作                                      │
│  包含：BSP_ 模块、STM32 HAL、外设寄存器操作                     │
│  特点：可被替换、可被 mock                                     │
└──────────────────────────────────────────────────────────────┘
```

### 3.2 任务层重构：Task 只做"调度"

每个 Task 文件的职责**只有**：

```c
// ✅ 正确的 Task 模式
void Task_CAN1F0(void *argument)
{
    /* 1. 初始化（只调 Init，不写配置） */
    APP_CAN_Init();

    /* 2. 无限循环 */
    for (;;) {
        /* 3. 等待事件 */
        osMessageQueueGet(can_f0_q, &packet, NULL, osWaitForever);

        /* 4. 委托给应用层处理，自己不写逻辑 */
        APP_CAN_RxFIFO0Process(&packet);
    }
}
```

```c
// ❌ 错误的 Task 模式（当前代码的问题）
void Task_CAN_F0(void *argument)
{
    // 直接把 CAN 配置逻辑写在 Task 里
    APP_CAN_Register(1, &hcan);
    APP_CAN_FilterConfig(...);  // 配置在任务中硬编码
    ...
    for (;;) {
        // 混杂业务逻辑和调度
    }
}
```

**重构规则：** Task 文件中禁止出现：
- ❌ 业务逻辑判断（if-else 处理 CAN 帧内容）
- ❌ 硬件配置细节（波特率、滤波器、GPIO 引脚）
- ❌ 数据类型转换/编码解码

✅ Task 只做：初始化→等待→分发

### 3.3 应用层重构：每个模块自包含

以 CAN 模块为例，当前问题：

```
❌ APP_CAN 只是 BSP_CAN 的"透传包装"（无增值逻辑）
❌ APP_CAN_CMD 是巨大的 switch-case（硬编码的 CAN ID 列表）
❌ CAN 滤波器配置散落在 Task 层和 APP 层
```

重构后的 App/Can/ 目录：

```
App/Can/
├── app_can.h                 # 公开接口
├── app_can_port.c            # CAN 端口管理（注册/启动/停止）
├── app_can_rx.c              # CAN 接收处理（帧路由到不同 handler）
├── app_can_tx.c              # CAN 发送处理（优先级队列/重试）
└── app_can_cfg.h             # CAN 配置（ID 映射、滤波器表）
```

### 3.4 引入配置表驱动

将硬编码的 switch-case 替换为**配置表**：

```c
// ❌ 旧的硬编码方式
void APP_CAN1_F0_Cmd(BSP_CAN_Packet_t *pkt) {
    switch (pkt->id) {
        case 0x201: /* leg1 */ break;
        case 0x202: /* leg2 */ break;
        case 0x203: /* leg3 */ break;
        case 0x204: /* leg4 */ break;
    }
}

// ✅ 新的配置表驱动
typedef struct {
    uint32_t        can_id;
    uint32_t        id_mask;
    can_handler_t   handler;
    const char     *name;
} can_rx_entry_t;

static const can_rx_entry_t CAN_RX_TABLE[] = {
    { .can_id = 0x201, .id_mask = 0x7FF,
      .handler = prvHandleLeg1Cmd, .name = "Leg1_Front" },
    { .can_id = 0x202, .id_mask = 0x7FF,
      .handler = prvHandleLeg2Cmd, .name = "Leg2_Front" },
    { .can_id = 0x203, .id_mask = 0x7FF,
      .handler = prvHandleLeg3Cmd, .name = "Leg3_Rear"  },
    { .can_id = 0x204, .id_mask = 0x7FF,
      .handler = prvHandleLeg4Cmd, .name = "Leg4_Rear"  },
};

static void prvDispatchByTable(BSP_CAN_Packet_t *pkt)
{
    for (size_t i = 0; i < ARRAY_SIZE(CAN_RX_TABLE); i++) {
        if ((pkt->id & CAN_RX_TABLE[i].id_mask) == CAN_RX_TABLE[i].can_id) {
            CAN_RX_TABLE[i].handler(pkt);
            return;
        }
    }
    /* 未匹配 → 走默认处理 */
}
```

**优势：**
- 新 CAN ID 只需加一行表项，不需要改 switch-case
- 配置集中管理，一目了然
- 支持运行时修改（如果表放在 RAM）

### 3.5 建立共享代码库

Gateway 和 FrontDog 之间大量重复代码（common.h、protocol.c、debounce、codec、oled），必须消除重复。

**推荐方案：** 提取为独立组件目录

```
RC_Dog/
├── Gateway/                   # 网关工程
├── FrontDog/                  # 前腿/后腿工程
├── Shared/                    # ✨ 共享代码库 ✨
│   ├── Common/
│   │   ├── common.h           # 类型定义、错误码
│   │   └── assert.h
│   ├── Protocol/
│   │   ├── protocol.c/h       # CAN 协议编解码
│   │   └── protocol_cfg.h     # 协议配置（ID 表、角度范围）
│   ├── Middleware/
│   │   ├── debounce.c/h
│   │   ├── codec.c/h
│   │   └── oled/
│   └── CMakeLists.txt         # 共享库的 CMake 构建
```

每个工程通过 CMake 引用 Shared 库：

```cmake
# Gateway/CMakeLists.txt
add_subdirectory(../Shared Shared)
target_link_libraries(Gateway PRIVATE rc_dog_shared)
```

---

## 4. 第三阶段：引入成熟架构模式

> **时间估计：5-7 天**
> **目标：** 用业界验证的设计模式重构核心模块

### 4.1 设计模式：观察者模式（发布-订阅）

**现状问题：** 按键事件走特定队列 `KEY_Q` → `Task_PAR` 直接调用 `App_Key_CMD_Packet`，形式固定不灵活。如果将来需要"按键→发 CAN 帧"和"按键→OLED 显示"和"按键→改变舵机模式"，当前架构需要修改 Task_PAR 的代码。

**重构方案：** 事件总线（Event Bus）

```c
// Middleware/EventBus/event_bus.h
typedef enum {
    EVENT_KEY_DOWN,
    EVENT_KEY_UP,
    EVENT_KEY_CLICK,
    EVENT_KEY_LONG,
    EVENT_KEY_LONG_UP,
    EVENT_CAN_FRAME_RX,
    EVENT_USART_CMD_RX,
} event_id_t;

typedef struct {
    event_id_t  id;
    uint32_t    data;       // 具体事件数据（key_id、can_id 等）
    uint32_t    timestamp;  // 事件发生时间（可选）
} event_t;

typedef void (*event_handler_t)(const event_t *evt);
typedef uint32_t subscription_id_t;

/* 接口 */
void        EventBus_Init(void);
subscription_id_t EventBus_Subscribe(event_id_t event, event_handler_t handler);
bool        EventBus_Unsubscribe(subscription_id_t sub_id);
void        EventBus_Publish(const event_t *evt);
void        EventBus_PublishFromISR(const event_t *evt);
```

```c
// 使用示例 — 按键按下，多个模块响应

// 模块 A：按键 → 站位命令
EventBus_Subscribe(EVENT_KEY_CLICK, onKeyToStand);

// 模块 B：按键 → OLED 显示更新
EventBus_Subscribe(EVENT_KEY_CLICK, onKeyToOLED);

// 模块 C：按键记录日志（通过 UART）
EventBus_Subscribe(EVENT_KEY_CLICK, onKeyToLog);
```

**实现注意：**
- 订阅者表可以是静态数组（不用动态分配）
- ISR 中使用 `EventBus_PublishFromISR`（内部用 `xQueueSendFromISR`）
- 事件处理在任务上下文中执行（ISR 只 post 到队列）

### 4.2 设计模式：硬件抽象层（HAL）接口

**现状问题：** APP 层直接调 BSP_xxx，如果换 MCU 所有 APP 需要修改。

**重构方案：** 用结构体函数指针表定义硬件接口

```c
// Hal/hal_can.h — 硬件无关的 CAN 接口抽象
typedef struct {
    bool (*init)(void *self);
    bool (*send)(void *self, uint32_t id, const uint8_t *data, uint8_t len);
    bool (*register_rx_callback)(void *self, void (*cb)(uint32_t id, uint8_t *data, uint8_t len));
    bool (*start)(void *self);
    bool (*stop)(void *self);
} hal_can_ops_t;

typedef struct {
    const hal_can_ops_t *ops;   // 虚函数表
    void *hw_obj;               // 具体硬件对象（STM32 的 CAN_HandleTypeDef*）
} hal_can_t;

// 使用方式
static inline bool HAL_CAN_Init(hal_can_t *dev) {
    return dev->ops->init(dev->hw_obj);
}
```

```c
// Bsp/bsp_can_stm32f1.c — STM32F1 的具体实现
static bool prvStm32F1_Init(void *hw_obj) {
    CAN_HandleTypeDef *hcan = (CAN_HandleTypeDef *)hw_obj;
    return (HAL_CAN_Init(hcan) == HAL_OK);
}

const hal_can_ops_t BSP_CAN_STM32F1_OPS = {
    .init   = prvStm32F1_Init,
    .send   = prvStm32F1_Send,
    .start  = prvStm32F1_Start,
    .stop   = prvStm32F1_Stop,
    .register_rx_callback = prvStm32F1_RegisterCallback,
};
```

```c
// 初始化时绑定
hal_can_t g_can1 = { .ops = &BSP_CAN_STM32F1_OPS, .hw_obj = &hcan1 };
HAL_CAN_Init(&g_can1);
```

**注意：** 对于 STM32F1 和 STM32F4 的共存的场景，这个抽象层的价值会非常高 —— 但不要过度设计。**只对"未来确实可能换"的模块做抽象**，比如：
- CAN（F1 一个 CAN，F4 两个 CAN）
- Timer/PWM（完全不同的定时器架构）
- UART（基本兼容但端口不同）

不要对 GPIO、ADC 等做过度抽象。

### 4.3 设计模式：状态机（有限状态机）

**现状问题：** 舵机控制目前是简单的"朝向目标插值"，没有状态概念。机器人应该有明确的状态（站立、蹲下、行走中、摔倒、急停等）。

```c
// App/RobotState/robot_state.h
typedef enum {
    ROBOT_IDLE,         // 空闲/待机
    ROBOT_STANDING,     // 站立
    ROBOT_WALKING,      // 行走中
    ROBOT_SITTING,      // 蹲下
    ROBOT_FALLEN,       // 摔倒
    ROBOT_EMERGENCY,    // 急停
    ROBOT_CALIBRATING,  // 校准中
} robot_state_t;

typedef struct {
    robot_state_t   current;
    robot_state_t   previous;
    uint32_t        state_start_time;
    bool            (*transitions[ROBOT_STATE_COUNT][ROBOT_STATE_COUNT])(void);
} robot_state_machine_t;
```

状态机应该**独立于硬件**，可以放在 Common/Middleware 层。

### 4.4 设计模式：命令模式

**现状问题：** USART 命令分发也是巨大的 switch-case，而且命令和数据耦合。

```c
// ❌ 旧的命令分发
void APP_USART_Cmd(uint8_t cmd, uint8_t *payload, uint16_t len) {
    switch (cmd) {
        case 0xA8: /* ... */
        case 0xA9: /* ... */
        case 0xAA: /* ... */
    }
}

// ✅ 命令模式
typedef struct {
    uint8_t     cmd_id;
    const char *name;
    error_t     (*handler)(uint8_t *payload, uint16_t len);
    const char *help;
} usart_cmd_entry_t;

static const usart_cmd_entry_t CMD_TABLE[] = {
    { .cmd_id = 0xA8, .name = "ECHO",   .handler = Cmd_Echo,   .help = "Echo back payload x10" },
    { .cmd_id = 0xA9, .name = "LED",    .handler = Cmd_LED,    .help = "Toggle LED" },
    { .cmd_id = 0xAA, .name = "RELAY",  .handler = Cmd_Relay,  .help = "Toggle relay PB11" },
    { .cmd_id = 0xB0, .name = "STATUS", .handler = Cmd_Status, .help = "Get system status" },
};
```

### 4.5 设计模式：资源池统一管理

当前 BSP 模块各自有各自的资源池，但都重复着"数组+计数+线性查找"的模式。可以提取为一个通用组件：

```c
// Common/registry.h — 通用资源注册表
typedef struct {
    void     **items;       // 资源数组
    uint8_t    capacity;    // 最大容量
    uint8_t    count;       // 当前数量
    size_t     item_size;   // 单个资源大小
} registry_t;

#define REGISTRY_DEFINE(name, type, max) \
    static type name##_pool[max];        \
    static registry_t name = {           \
        .items = (void**)name##_pool,    \
        .capacity = max,                 \
        .item_size = sizeof(type)        \
    };

int  Registry_Add(registry_t *reg, void *item);
int  Registry_Find(registry_t *reg, bool (*match)(void *item, void *ctx), void *ctx);
void Registry_Clear(registry_t *reg);
```

### 4.6 错误处理与断言

**现状问题：** 几乎没有错误处理。

**重构方案：** 建立统一的错误处理层级

```c
// Common/error.h
typedef enum {
    ERR_OK      = 0,
    ERR_PARAM   = -1,    // 参数错误
    ERR_BUSY    = -2,    // 资源繁忙
    ERR_TIMEOUT = -3,    // 超时
    ERR_NOMEM   = -4,    // 内存不足 (静态池满)
    ERR_HW      = -5,    // 硬件错误
    ERR_STATE   = -6,    // 状态错误
    ERR_NODEV   = -7,    // 设备未找到
} error_t;

// Common/assert.h — 编译时可配置的断言
// DEBUG 模式：触发断点
// RELEASE 模式：记录错误并恢复
// 最小化 RAM 版本：直接调用错误处理
#if defined(DEBUG)
    #define ASSERT(cond) do {                         \
        if (!(cond)) {                                 \
            Error_Handler(__FILE__, __LINE__);         \
        }                                              \
    } while(0)
#else
    #define ASSERT(cond) ((void)0)
#endif
```

---

## 5. 第四阶段：提高移植性

> **时间估计：3-4 天**
> **目标：** 代码可跨平台、可跨项目复用

### 5.1 平台抽象层

建立一个文件集中管理所有平台相关的宏：

```c
// Common/platform.h — 平台抽象层
// 这个文件是移植到新 MCU 时需要修改的唯一文件

/* MCU 型号选择（由 CMake 定义） */
#if defined(STM32F103xB)
    #define PLATFORM_STM32F1
#elif defined(STM32F405xx)
    #define PLATFORM_STM32F4
#else
    #error "Unsupported MCU platform"
#endif

/* 编译器相关 */
#if defined(__GNUC__)
    #define PLATFORM_WEAK       __attribute__((weak))
    #define PLATFORM_PACKED     __attribute__((packed))
    #define PLATFORM_INLINE     static inline
    #define PLATFORM_ALIGN(n)   __attribute__((aligned(n)))
#else
    #error "Unsupported compiler"
#endif

/* 字节序 */
#define PLATFORM_BIG_ENDIAN     0   // Cortex-M 是小端

/* IRQ 关键段 */
#define PLATFORM_CRITICAL_ENTER()   taskENTER_CRITICAL()
#define PLATFORM_CRITICAL_EXIT()    taskEXIT_CRITICAL()

/* 断言控制 */
#if defined(DEBUG)
    #define PLATFORM_ASSERT(cond)   platform_assert(__FILE__, __LINE__, #cond)
#else
    #define PLATFORM_ASSERT(cond)   ((void)0)
#endif
```

### 5.2 构建系统统一

**目标：** 一个构建脚本模板，三个工程共享

```cmake
# cmake/rc_dog_common.cmake — 共享的 CMake 配置

# 通用编译选项
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)
add_compile_options(-ffunction-sections -fdata-sections)

# 根据 MCU 设置
function(rc_dog_set_target TARGET MCU_TYPE)
    if(${MCU_TYPE} STREQUAL "STM32F103")
        target_compile_definitions(${TARGET} PRIVATE STM32F103xB)
        target_link_options(${TARGET} PRIVATE -T${CMAKE_SOURCE_DIR}/STM32F103XX_FLASH.ld)
    elseif(${MCU_TYPE} STREQUAL "STM32F405")
        target_compile_definitions(${TARGET} PRIVATE STM32F405xx)
        target_link_options(${TARGET} PRIVATE -T${CMAKE_SOURCE_DIR}/STM32F405XX_FLASH.ld)
    endif()
    target_link_options(${TARGET} PRIVATE -Wl,--gc-sections -Wl,-Map=${TARGET}.map)
endfunction()
```

### 5.3 配置集中化

建立每个工程的 `project_cfg.h`，集中管理所有可配置参数：

```c
// User/Common/project_cfg.h — 项目配置文件
#pragma once

/* ========== CAN 配置 ========== */
#define CFG_CAN_BITRATE             500000U
#define CFG_CAN_LEG1_ID             0x201U
#define CFG_CAN_LEG2_ID             0x202U
#define CFG_CAN_LEG3_ID             0x203U
#define CFG_CAN_LEG4_ID             0x204U
#define CFG_CAN_RX_QUEUE_LEN        16U

/* ========== Servo 配置 ========== */
#define CFG_SERVO_ANGLE_MIN         (-125)
#define CFG_SERVO_ANGLE_MAX         125
#define CFG_SERVO_SPEED_DEFAULT     3U
#define CFG_SERVO_PWM_FREQ_HZ       330U

/* ========== UART 配置 ========== */
#define CFG_UART_BAUDRATE           115200U
#define CFG_UART_RX_BUF_SIZE        256U
#define CFG_UART_TX_QUEUE_SIZE      11U

/* ========== 任务配置 ========== */
#define CFG_TASK_KEY_PERIOD_MS      10U
#define CFG_TASK_SERVO_PERIOD_MS    10U
#define CFG_TASK_OLED_PERIOD_MS     10U
#define CFG_TASK_KEY_STACK_SIZE     256U
#define CFG_TASK_CAN_STACK_SIZE     256U

/* ========== 按键配置 ========== */
#define CFG_KEY_DEBOUNCE_TICKS      2U
#define CFG_KEY_LONG_PRESS_TICKS    18U   // ~180ms
#define CFG_KEY_MAX_COUNT           6U

/* ========== Debug 配置 ========== */
#define CFG_ENABLE_ASSERT           1U
#define CFG_ENABLE_STACK_MONITOR    1U
#define CFG_ENABLE_CPU_USAGE        0U
```

这样移植时只需要修改 `project_cfg.h`，不需要在代码里搜 `500000` 或 `330` 这样的魔数。

### 5.4 将协议定义放到独立仓库

```
RC_Dog/
├── docs/
│   └── can_protocol.md          # 协议文档
├── Shared/
│   └── Protocol/
│       ├── protocol.h           # 所有 CAN ID、DLC、数据格式
│       ├── protocol.c           # 编解码实现
│       └── protocol_test.c      # 协议单元测试（可在 PC 上运行）
```

**协议文档样例：**

```markdown
# RC Dog CAN Bus Protocol v1.1

## 物理层
- 波特率：500 kbps
- 标准帧 (11-bit ID)
- 数据帧 (非远程帧)

## 帧结构
| Byte | 内容 | 类型 | 说明 |
|------|------|------|------|
| 0-1  | Servo 1 | int16_t LE | 角度值 -125 ~ +125 |
| 2-3  | Servo 2 | int16_t LE | 角度值 -125 ~ +125 |
| 4-5  | Servo 3 | int16_t LE | 角度值 -125 ~ +125 |

## CAN ID 映射
| ID   | 目标 | 说明 |
|------|------|------|
|0x201 | FrontDog Leg1 | 左前腿 |
|0x202 | FrontDog Leg2 | 右前腿 |
|0x203 | FrontDog Leg3 / RearDog Leg1 | 左后腿 |
|0x204 | FrontDog Leg4 / RearDog Leg2 | 右后腿 |
```

---

## 6. 第五阶段：工程化与质量保障

> **时间估计：2-3 天 + 持续**
> **目标：** 可测试、可调试、可追溯

### 6.1 引入单元测试

对于**平台无关**的代码（Protocol、Debounce、Codec、StateMachine），可以在 PC 上运行单元测试：

```c
// test/test_protocol.c — 协议编码解码测试
void TEST_Protocol_EncodeDecode(void) {
    int16_t angles[] = {100, -50, 25};
    uint8_t frame[6];

    Protocol_EncodeServoFrame(angles, 3, frame);

    int16_t decoded[3];
    Protocol_DecodeServoFrame(frame, decoded, 3);

    assert(decoded[0] == 100);
    assert(decoded[1] == -50);
    assert(decoded[2] == 25);
}
```

**推荐框架：** Unity (C 单元测试框架) 或直接自己写一个简单的宏：

```c
// Common/test.h — 极简单元测试框架
#define TEST(name)    static void test_##name(void)
#define RUN(name)     do { test_##name(); passed++; } while(0)
#define ASSERT(cond)  do { if (!(cond)) { failed++; }} while(0)
```

### 6.2 运行时诊断

在 Release 版本中保留这些诊断功能：

```c
// 1. 栈使用监控
void Task_Monitor(void *arg) {
    for (;;) {
        UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(NULL);
        if (highWaterMark < STACK_WARN_THRESHOLD) {
            /* 通过 UART 发出警告 */
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// 2. CPU 使用率监控
#if CFG_ENABLE_CPU_USAGE
    // 用空闲任务的执行计数估算 CPU 占用率
    volatile uint32_t idle_counter = 0;
    void vApplicationIdleHook(void) { idle_counter++; }
#endif

// 3. 错误日志环形缓冲区
typedef struct {
    uint32_t  timestamp;
    error_t   error;
    uint8_t   module_id;
    uint8_t   extra_info;
} error_log_t;

#define ERROR_LOG_SIZE  32
static error_log_t s_error_log[ERROR_LOG_SIZE];
static uint8_t     s_error_log_idx = 0;

void Error_Log(error_t err, uint8_t module, uint8_t info) {
    s_error_log[s_error_log_idx].timestamp = xTaskGetTickCount();
    s_error_log[s_error_log_idx].error     = err;
    s_error_log[s_error_log_idx].module_id = module;
    s_error_log[s_error_log_idx].extra_info = info;
    s_error_log_idx = (s_error_log_idx + 1) % ERROR_LOG_SIZE;
}
```

### 6.3 版本规范

使用语义化版本：

```
v1.0.0
 ├── Major: 架构变更、不兼容的协议修改
 ├── Minor: 新功能、兼容性修改
 └── Patch: Bug 修复、小优化
```

每次 commit 遵循 [Conventional Commits](https://www.conventionalcommits.org/)：

```
feat(can): 添加 CAN 帧重发机制
fix(servo): 修复舵机回零偏移量错误
refactor(task): 将任务初始化移至统一入口
docs(protocol): 更新 CAN 协议文档 v1.1
style(key): 统一按键模块命名风格
test(debounce): 添加边界条件测试
```

### 6.4 Code Review Checklist

每次提交前检查：

- [ ] 是否有编译警告？启用 `-Wall -Wextra -Wpedantic` 检查
- [ ] 是否有未使用的变量/函数？
- [ ] 所有函数参数是否检查了 NULL/越界？
- [ ] 所有返回值是否被处理？
- [ ] 是否有 magic number 应该改为宏定义？
- [ ] 是否有可以 const 但没有 const 的变量？
- [ ] ISR 中是否调用了非 FromISR 结尾的 FreeRTOS API？
- [ ] 模块间是否通过队列/事件总线而不是直接函数调用耦合？
- [ ] 新功能是否有对应的文档或注释？
- [ ] 平台相关代码是否用 `#ifdef` 保护？

---

## 7. Roadmap 时间线建议

```
Week 1 — 基础规范
├── Day 1-2: 统一命名、目录结构、文件头
├── Day 3:   魔数→宏、配置集中化
└── Day 4-5: 提取 Shared 共享库

Week 2 — 架构分层
├── Day 1-2: Task 层剥离业务逻辑
├── Day 3-4: 引入配置表替代 switch-case
└── Day 5:   事件总线实现

Week 3 — 设计模式
├── Day 1-2: 状态机实现
├── Day 3:   命令模式重构 USART 分发
├── Day 4:   资源池统一
└── Day 5:   错误处理体系

Week 4 — 移植性与工程化
├── Day 1-2: 协议文档化 + PC 端单元测试
├── Day 3:   运行时诊断（栈监测、错误日志）
└── Day 4-5: 构建系统优化、CI/CD 接入

持续 — 日常规范
├── Conventional Commits
├── Code Review Checklist
└── 逐步补全 Doxygen 注释
```

### 优先级建议

| 优先级 | 事项 | 理由 |
|--------|------|------|
| 🔴 P0 | 提取 Shared 库 | 消除重复，避免两边改 bug |
| 🔴 P0 | 命名规范 + 目录整理 | 后续所有操作的基础 |
| 🔴 P0 | 配置集中化 | 魔数分散是 bug 之源 |
| 🟡 P1 | 任务层剥离业务逻辑 | 提高可读性，分模块测试 |
| 🟡 P1 | 配置表驱动 | 减少 if/switch，易扩展 |
| 🟡 P1 | 错误处理体系 | 现场可排查问题 |
| 🟢 P2 | 事件总线 | 提高模块解耦度 |
| 🟢 P2 | 状态机 | 逻辑更清晰 |
| 🟢 P2 | 单元测试 | 长远质量保障 |
| 🔵 P3 | 硬件抽象层 HAL | 当前不换 MCU，收益不高 |

**建议：** P0 立刻做，P1 在两周内做，P2/P3 在功能稳定后逐步推进。

---

## 8. 附录：参考与推荐阅读

### 嵌入式软件架构
- [Embedded Software Architecture Patterns](https://www.oreilly.com/library/view/real-time-embedded/9780128119062/) — 实时嵌入式系统经典
- [QPC — Quantum Leaps 状态机框架](https://www.state-machine.com/qpc/) — 层级状态机参考实现
- [UML State Machine 入门](https://www.state-machine.com/uml-state-machines-intro)

### 设计模式
- [Design Patterns for Embedded Systems in C](https://www.embedded.com/design-patterns-for-embedded-systems-in-c/) — 嵌入式 C 设计模式
- [Event-Driven Architecture for Embedded Systems](https://www.embedded.com/event-driven-architecture-for-embedded-systems/)

### STM32 相关
- [STM32 HAL/Low-Layer 驱动开发指南](https://www.st.com/resource/en/user_manual/dm00105879.pdf)
- [FreeRTOS 官方文档](https://www.freertos.org/Documentation/RTOS_book.html)

### 工具链
- [CMake 嵌入式项目模板](https://github.com/ObKo/stm32-cmake)
- [Unity Test — C 单元测试框架](http://www.throwtheswitch.org/unity)
- [CppUTest — 嵌入式 C/C++ 测试框架](https://cpputest.github.io/)

### 代码规范
- [Linux Kernel Coding Style](https://www.kernel.org/doc/html/v4.10/process/coding-style.html) — C 代码风格权威参考
- [SEI CERT C Coding Standard](https://wiki.sei.cmu.edu/confluence/display/c/SEI+CERT+C+Coding+Standard) — 安全编码标准

---

> **最后的话：**
>
> 架构优化的核心不是"用多好的模式"，而是**一致性**和**可预测性**：
> - 团队（或以后的你）打开任何文件，都知道它应该长什么样
> - 改一个功能时，能准确判断应该改哪个文件的哪一行
> - 新加功能时，不用纠结"这个放哪"，放对应目录就行
>
> 不要试图一次性做完所有事情。**先改规范、再分层、再优化**，每一步都是可独立交付的质量提升。
>
> 这个项目能成为你的第一个"完整作品"——不是因为代码完美，而是因为你**有意识地在构建它**，而不是在"搭积木"。

---

*文档版本: v1.0 | 最后更新: 2026-06-05*
