# =============================================================================
# APP 层：应用层
#
#   业务只有两种入口：Init（freertos.c 调度前调用）和"一步/一条命令"（线程里调用），
#   本层不建线程、不碰寄存器、不碰 OS。
#
#   一个模块一件事，目录名 = 模块名，文件名 = 模块名小写：
#     APP_Motion/app_motion.c          腿级：关节角 → 舵机角 + 步态调度
#     APP_Chassis/app_chassis.c        底盘：四个轮子成组下发速度
#     APP_Uart/app_uart_cmd.c          串口命令表（命令字 → 动作）
#     APP_Key_Cmd/app_key_cmd.c        按键命令表（事件 → 动作）
#     APP_CAN/app_can_cmd_1.c          CAN 接收命令表
#     APP_Debug/app_debug.c            调试输出（可整块删掉）
#
#   依赖方向单向：命令表 → 运动/底盘 → 服务/设备，反向没有边。
#   头文件不外泄下层类型的，那一层写 PRIVATE；外泄的（common_ik 的杆长类型）
#   才写 PUBLIC。
# =============================================================================

# -----------------------------------------------------------------------------
# app_motion：腿级控制 + 步态调度（杆长类型来自 common_ik，PUBLIC 传递）
# -----------------------------------------------------------------------------
add_library(app_motion STATIC
    APP_Motion/app_motion.c
    APP_Motion/app_motion_scheduler.c
)
target_include_directories(app_motion
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/APP_Motion
)
target_link_libraries(app_motion
    PUBLIC  common_ik
    PRIVATE bsp_sys
            svc_servo_motion
            common_utils
)
target_compile_options(app_motion PRIVATE ${USER_WARN_FLAGS})

# -----------------------------------------------------------------------------
# app_chassis：底盘（轮号 → 速度环句柄，四个轮子成组下发）
# -----------------------------------------------------------------------------
add_library(app_chassis STATIC
    APP_Chassis/app_chassis.c
)
target_include_directories(app_chassis
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/APP_Chassis
)
target_link_libraries(app_chassis
    PRIVATE dev_motor
            svc_motor_speed
            common_utils
)
target_compile_options(app_chassis PRIVATE ${USER_WARN_FLAGS})

# -----------------------------------------------------------------------------
# app_uart：串口命令表（在 app_uart.c 里还提供统一的发送出口）
# -----------------------------------------------------------------------------
add_library(app_uart STATIC
    APP_UART/app_uart.c
    APP_UART/app_uart_cmd.c
)
target_include_directories(app_uart
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/APP_UART
)
target_link_libraries(app_uart
    PUBLIC  svc_uart
            svc_proto
    PRIVATE app_motion
            app_chassis
            dev_led
            common_utils
)
target_compile_options(app_uart PRIVATE ${USER_WARN_FLAGS})

# -----------------------------------------------------------------------------
# app_key_cmd：按键命令表（事件类型来自 svc_key_logic，PUBLIC 传递）
# -----------------------------------------------------------------------------
add_library(app_key_cmd STATIC
    APP_Key_Cmd/app_key_cmd.c
)
target_include_directories(app_key_cmd
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/APP_Key_Cmd
)
target_link_libraries(app_key_cmd
    PUBLIC  svc_key_logic
    PRIVATE dev_led
            app_uart
            common_utils
)
target_compile_options(app_key_cmd PRIVATE ${USER_WARN_FLAGS})

# -----------------------------------------------------------------------------
# app_can：CAN 收发口 + 命令表（帧类型来自 bsp_can，PUBLIC 传递）
# -----------------------------------------------------------------------------
add_library(app_can STATIC
    APP_CAN/app_can.c
    APP_CAN/app_can_cmd_1.c
    APP_CAN/app_can_cmd_2.c
)
target_include_directories(app_can
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/APP_CAN
)
target_link_libraries(app_can
    PUBLIC  bsp_can
            svc_can_tx
    PRIVATE common_utils
)
target_compile_options(app_can PRIVATE ${USER_WARN_FLAGS})

# -----------------------------------------------------------------------------
# app_debug：调试输出（只读 Service 的公开状态，可整块删掉）
# -----------------------------------------------------------------------------
add_library(app_debug STATIC
    APP_Debug/app_debug.c
)
target_include_directories(app_debug
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/APP_Debug
)
target_link_libraries(app_debug
    PUBLIC  svc_motor_speed
    PRIVATE bsp_uart
            common_utils
)
target_compile_options(app_debug PRIVATE ${USER_WARN_FLAGS})
