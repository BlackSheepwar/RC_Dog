# =============================================================================
# Service 层：服务层
#
#   存放"设备怎么用"的规则：消抖、平滑、闭环、协议、排队。
#   不碰寄存器、不看时钟（时间由 TASK 以 elapsed_ms 传进来），
#   因此这一层的大部分模块可以在 PC 上喂假时间单测。
#
#   依赖方向：只往下看（BSP / Device / Common），禁止 include APP / TASK。
#   头文件里出现 dev_servo_t / dev_motor_t 指针的，dev_* 必须 PUBLIC 传递。
# =============================================================================

# -----------------------------------------------------------------------------
# svc_proto：帧协议 FF AA LEN CMD [PAYLOAD] CHK（打包 + 流式解包，纯函数）
# -----------------------------------------------------------------------------
add_library(svc_proto STATIC
    Svc_Proto/svc_proto.c
)
target_include_directories(svc_proto
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Svc_Proto
)
target_compile_options(svc_proto PRIVATE ${USER_WARN_FLAGS})

# -----------------------------------------------------------------------------
# svc_key_logic：按键逻辑（消抖 + 短按/长按，只收 raw + elapsed_ms）
# -----------------------------------------------------------------------------
add_library(svc_key_logic STATIC
    Svc_Key_Logic/svc_key_logic.c
)
target_include_directories(svc_key_logic
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Svc_Key_Logic
)
target_compile_options(svc_key_logic PRIVATE ${USER_WARN_FLAGS})

# -----------------------------------------------------------------------------
# svc_servo_motion：舵机运动（限速 + 平滑逼近 + 到位判定）
#   头文件收 dev_servo_t 指针，所以 dev_servo 必须 PUBLIC
# -----------------------------------------------------------------------------
add_library(svc_servo_motion STATIC
    Svc_Servo_Motion/svc_servo_motion.c
)
target_include_directories(svc_servo_motion
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Svc_Servo_Motion
)
target_link_libraries(svc_servo_motion
    PUBLIC  dev_servo
    PRIVATE common_utils
)
target_compile_options(svc_servo_motion PRIVATE ${USER_WARN_FLAGS})

# -----------------------------------------------------------------------------
# svc_motor_speed：电机速度环（测速 + PID 闭环 + 限幅 + 停稳）
#   PID 算法在 Common/Common_Pid，只在 .c 里出现，所以是 PRIVATE
# -----------------------------------------------------------------------------
add_library(svc_motor_speed STATIC
    Svc_Motor_Speed/svc_motor_speed.c
)
target_include_directories(svc_motor_speed
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Svc_Motor_Speed
)
target_link_libraries(svc_motor_speed
    PUBLIC  dev_motor
    PRIVATE common_pid
            common_utils
)
target_compile_options(svc_motor_speed PRIVATE ${USER_WARN_FLAGS})

# -----------------------------------------------------------------------------
# svc_can_tx：CAN 发送缓冲（环形队列 + 排空策略，不碰 OS）
#   帧值类型来自 bsp_can，头文件要用，所以 PUBLIC
# -----------------------------------------------------------------------------
add_library(svc_can_tx STATIC
    Svc_Can_Tx/svc_can_tx.c
)
target_include_directories(svc_can_tx
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Svc_Can_Tx
)
target_link_libraries(svc_can_tx
    PUBLIC  bsp_can
    PRIVATE common_utils
)
target_compile_options(svc_can_tx PRIVATE ${USER_WARN_FLAGS})

# -----------------------------------------------------------------------------
# svc_uart：串口链路（字节流→帧、帧队列、发送泵）
#   这里确实要碰 HAL 的 DMA 回调，所以链接 stm32cubemx
# -----------------------------------------------------------------------------
add_library(svc_uart STATIC
    Svc_Uart/svc_uart.c
)
target_include_directories(svc_uart
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Svc_Uart
)
target_link_libraries(svc_uart
    PUBLIC  svc_proto
    PRIVATE bsp_uart
            gfifo
            common_utils
            stm32cubemx
)
target_compile_options(svc_uart PRIVATE ${USER_WARN_FLAGS})
