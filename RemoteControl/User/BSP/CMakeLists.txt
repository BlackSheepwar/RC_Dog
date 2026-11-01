# =============================================================================
# BSP 层：只碰寄存器与 HAL，不知道接的是什么
#
# stm32cubemx 是 CubeMX 生成的 INTERFACE 库（提供 HAL 头与芯片宏），
# 只在 PRIVATE 位置链接：上层链接 BSP 不会把 HAL 拖进自己的依赖树。
# =============================================================================

# --- GPIO -------------------------------------------------------------------
add_library(bsp_gpio STATIC
    BSP_GPIO/bsp_gpio.c
)
target_include_directories(bsp_gpio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/BSP_GPIO)
target_link_libraries(bsp_gpio PUBLIC common_utils PRIVATE stm32cubemx)
target_compile_options(bsp_gpio PRIVATE ${USER_WARN_FLAGS})

# --- 系统（时钟/延时等） ----------------------------------------------------
add_library(bsp_sys STATIC
    BSP_SYS/bsp_sys.c
)
target_include_directories(bsp_sys PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/BSP_SYS)
target_link_libraries(bsp_sys PUBLIC common_utils PRIVATE stm32cubemx)
target_compile_options(bsp_sys PRIVATE ${USER_WARN_FLAGS})

# --- 编码器（旋钮） ---------------------------------------------------------
add_library(bsp_encoder STATIC
    BSP_Encoder/bsp_encoder.c
)
target_include_directories(bsp_encoder PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/BSP_Encoder)
target_link_libraries(bsp_encoder PUBLIC common_utils PRIVATE stm32cubemx)
target_compile_options(bsp_encoder PRIVATE ${USER_WARN_FLAGS})

# --- IIC（统一传输入口，OLED 挂这一路） -------------------------------------
add_library(bsp_iic STATIC
    BSP_IIC/bsp_iic.c
)
target_include_directories(bsp_iic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/BSP_IIC)
target_link_libraries(bsp_iic PUBLIC common_utils PRIVATE stm32cubemx)
target_compile_options(bsp_iic PRIVATE ${USER_WARN_FLAGS})

# --- 串口（DMA + GFIFO 字节流） ---------------------------------------------
add_library(bsp_uart STATIC
    BSP_UART/bsp_uart.c
)
target_include_directories(bsp_uart PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/BSP_UART)
target_link_libraries(bsp_uart
    PUBLIC  common_utils
    PRIVATE stm32cubemx gfifo
)
target_compile_options(bsp_uart PRIVATE ${USER_WARN_FLAGS})
