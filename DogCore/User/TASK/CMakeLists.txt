# =============================================================================
# TASK 层：任务体
#
#   CubeMX 在 .ioc 里配置任务（名字、优先级、栈、静态分配），
#   生成的 freertos.c 负责 osThreadNew，并给出与任务同名的 __weak 空入口；
#   这里为每个任务提供同名强定义，覆盖那个空入口。
#
#   所以：加任务 = 先在 CubeMX 里加（入口名与函数同名），再加一个 task_xxx.c。
#   不用手写 osThreadNew，也不用改 freertos.c。
#
#   任务体直接平铺在 TASK/ 下，不按任务再分子目录：
#     task_key_debounce.c   Task_KEY_Debounce   10 ms 扫描按键，产出事件
#     task_key_cmd.c        TASK_KEY_CMD        阻塞收事件，执行按键业务
#     task_servo.c          Task_Servo          10 ms 推一次 Svc_Servo_Motion
#     task_motor.c          Task_Motor          10 ms 推一次 Svc_Motor_Speed
#     task_motion.c         Task_Motion         10 ms 推进步态相位
#     task_can_rxf0/rxf1.c  Task_CAN_RXF0/RXF1  阻塞收 CAN 帧，按表分发
#     task_can_tx.c         Task_CAN_TX         1 ms 排空发送缓冲
#     task_uart_rx/tx.c     Task_UART_RX/TX     字节流 ↔ 帧、DMA 泵
#     task_uart_rx_cmd.c    Task_UART_RX_CMD    取一帧，交给命令表
#     task_oled.c           Task_OLED           刷屏
#
#   初始化全部在 Core/Src/freertos.c 的 USER CODE 段里，于第一个线程启动
#   之前完成（定时器频率 → 设备 → 服务 → APP），任一步失败就调 Error_Handler()。
#   按 OS 对象依赖分段：不碰 OS 对象的放 "BEGIN Init"，要互斥量的放
#   "BEGIN RTOS_MUTEX"（IIC/OLED），要队列的放 "BEGIN RTOS_QUEUES"（UART/CAN）。
#   任务体里不初始化任何全局状态。链上的 Init 仍然幂等，
#   重复调用不会把已下发的目标值与 PID 积分拽回去。
#
#   为什么用 OBJECT 库再 target_sources 塞进可执行文件，而不是 STATIC 库：
#     静态库里的强符号如果没有被引用，链接器会整段丢掉，而 freertos.c 里的
#     __weak 空实现恰好"满足"了 osThreadNew 的引用，归档成员根本不会被拉进来，
#     结果任务线程空转。目标文件是强制链接的，强定义必定覆盖 __weak。
# =============================================================================

add_library(task_objects OBJECT
    task_key_debounce.c
    task_key_cmd.c
    task_servo.c
    task_motor.c
    task_motion.c
    task_can_rxf0.c
    task_can_rxf1.c
    task_can_tx.c
    task_uart_rx.c
    task_uart_tx.c
    task_uart_rx_cmd.c
    task_oled.c
)

target_link_libraries(task_objects
    PRIVATE stm32cubemx
            common_utils
            bsp_pwm
            dev_key
            dev_motor
            dev_servo
            dev_oled
            svc_key_logic
            svc_servo_motion
            svc_motor_speed
            svc_uart
            svc_proto
            app_key_cmd
            app_chassis
            app_motion
            app_can
            app_uart
)

target_compile_options(task_objects PRIVATE ${USER_WARN_FLAGS})
