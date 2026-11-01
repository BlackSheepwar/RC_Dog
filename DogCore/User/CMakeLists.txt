# =============================================================================
# User 顶层：按层收集子目录
#
#   Common      横向层  纯软件  错误码、PID、二连杆逆解（可在 PC 上编译）
#   Third_Party 只读区  别人的代码，只覆盖不改
#   BSP         第 1 层  硬件驱动    GPIO/PWM/CAN/编码器/串口/IIC 句柄
#   Device      第 2 层  设备抽象    舵机、电机、按键、指示灯、OLED
#   Service     第 3 层  服务        消抖、运动平滑、速度环、协议、链路
#   APP         第 4 层  应用        步态、底盘、命令表
#   TASK        任务层   线程体       一个任务一个 .c，覆盖 CubeMX 的 __weak 入口
#
# 分层规则由"链接关系"保证，不靠自觉：
#   每层的模块都是一个静态库，只链接下一层的模块。
#   头文件里出现了下层类型（dev_servo_t 指针、bsp_pwm_t 指针）的，
#   那一层就必须 PUBLIC 传递；只在 .c 里用到的写 PRIVATE。
#
# 线程由 CubeMX 的 .ioc 生成（osThreadNew 在 Core/Src/freertos.c），
# User/ 这边只写线程体和业务，不手写 osThreadNew。
# =============================================================================

# 统一编译选项：警告即错误（子目录里直接引用这个变量）。
# 第三方目录自己加 -w 覆盖，不让别人的告警变成我们的错误。
set(USER_WARN_FLAGS -Wall -Wextra -Werror)

add_subdirectory(Common)
add_subdirectory(Third_Party)
add_subdirectory(BSP)
add_subdirectory(Device)
add_subdirectory(Service)
add_subdirectory(APP)
add_subdirectory(TASK)

# -----------------------------------------------------------------------------
# 挂到主可执行文件：只列"主程序自己直接引用"的库，不重复抄依赖树
#   Core/Src/freertos.c 的初始化链会直接调用 bsp_pwm / dev_key，
#   所以这两个模块也要列进来（它们的头文件路径是 PUBLIC 的，链接即传递）。
# -----------------------------------------------------------------------------
target_link_libraries(${CMAKE_PROJECT_NAME}
    # TASK 层
    task_objects
    # APP 层
    app_key_cmd
    app_chassis
    app_can
    app_motion
    app_uart
    app_debug
    # Service 层
    svc_servo_motion
    svc_motor_speed
    svc_uart
    # Device 层
    dev_oled
    dev_key
    # BSP 层
    bsp_iic
    bsp_pwm
    # CubeMX
    stm32cubemx
)
