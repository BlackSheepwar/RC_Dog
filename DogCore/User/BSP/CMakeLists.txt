# =============================================================================
# BSP 层：硬件驱动层
#
#   每个模块一个静态库，只暴露自己的头文件目录给上层；
#   这一层直接操作寄存器，必须能看见 HAL，所以链接 stm32cubemx。
#
#   句柄不透明：头文件只有 typedef 名字，布局、引脚号、定时器号都在 .c。
#   因此本层的依赖（HAL / common）全部写 PRIVATE：上层链接 BSP
#   既拿不到 stm32h7xx_hal.h，也拿不到 common.h。
# =============================================================================

# -----------------------------------------------------------------------------
# bsp_gpio：引脚电平读写（句柄里只有端口/引脚/方向）
# -----------------------------------------------------------------------------
add_library(bsp_gpio STATIC
    BSP_GPIO/bsp_gpio.c
)
target_include_directories(bsp_gpio
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/BSP_GPIO
)
target_link_libraries(bsp_gpio
    PRIVATE stm32cubemx
)
target_compile_options(bsp_gpio PRIVATE ${USER_WARN_FLAGS})

# -----------------------------------------------------------------------------
# bsp_pwm：定时器通道（句柄里带通道号与计数频率，脉宽/占空比两种下法）
# -----------------------------------------------------------------------------
add_library(bsp_pwm STATIC
    BSP_PWM/bsp_pwm.c
)
target_include_directories(bsp_pwm
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/BSP_PWM
)
target_link_libraries(bsp_pwm
    PRIVATE stm32cubemx
            common_utils
)
target_compile_options(bsp_pwm PRIVATE ${USER_WARN_FLAGS})

# -----------------------------------------------------------------------------
# bsp_encoder：正交编码器（句柄里带定时器与方向取反）
# -----------------------------------------------------------------------------
add_library(bsp_encoder STATIC
    BSP_Encoder/bsp_encoder.c
)
target_include_directories(bsp_encoder
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/BSP_Encoder
)
target_link_libraries(bsp_encoder
    PRIVATE stm32cubemx
)
target_compile_options(bsp_encoder PRIVATE ${USER_WARN_FLAGS})

# -----------------------------------------------------------------------------
# bsp_can：FDCAN（帧是值类型，接收中断里投递到 CubeMX 建的队列）
# -----------------------------------------------------------------------------
add_library(bsp_can STATIC
    BSP_CAN/bsp_can.c
)
target_include_directories(bsp_can
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/BSP_CAN
)
target_link_libraries(bsp_can
    PRIVATE stm32cubemx
)
target_compile_options(bsp_can PRIVATE ${USER_WARN_FLAGS})

# -----------------------------------------------------------------------------
# bsp_uart：DMA 收发 + GFIFO 字节流（gfifo 只在 .c 里出现）
# -----------------------------------------------------------------------------
add_library(bsp_uart STATIC
    BSP_UART/bsp_uart.c
)
target_include_directories(bsp_uart
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/BSP_UART
)
target_link_libraries(bsp_uart
    PRIVATE stm32cubemx
            gfifo
            common_utils
)
target_compile_options(bsp_uart PRIVATE ${USER_WARN_FLAGS})

# -----------------------------------------------------------------------------
# bsp_iic：IIC 总线（互斥量排队 + 传输完成，HAL 句柄不外泄）
# -----------------------------------------------------------------------------
add_library(bsp_iic STATIC
    BSP_IIC/bsp_iic.c
)
target_include_directories(bsp_iic
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/BSP_IIC
)
target_link_libraries(bsp_iic
    PRIVATE stm32cubemx
)
target_compile_options(bsp_iic PRIVATE ${USER_WARN_FLAGS})

# -----------------------------------------------------------------------------
# bsp_sys：毫秒时间基准（换 tick 源只改这一个文件）
# -----------------------------------------------------------------------------
add_library(bsp_sys STATIC
    BSP_SYS/bsp_sys.c
)
target_include_directories(bsp_sys
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/BSP_SYS
)
target_link_libraries(bsp_sys
    PRIVATE stm32cubemx
)
target_compile_options(bsp_sys PRIVATE ${USER_WARN_FLAGS})
