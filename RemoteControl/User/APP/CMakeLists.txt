# =============================================================================
# APP 层：业务状态 + 命令表，不建线程、不碰寄存器
# =============================================================================

# --- 指示灯 -----------------------------------------------------------------
add_library(app_led STATIC
    APP_LED/app_led.c
)
target_include_directories(app_led PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/APP_LED)
target_link_libraries(app_led PUBLIC bsp_gpio PRIVATE stm32cubemx)
target_compile_options(app_led PRIVATE ${USER_WARN_FLAGS})

# --- 按键业务 ---------------------------------------------------------------
add_library(app_key STATIC
    APP_Key/app_key.c
    APP_Key/app_key_cmd.c
)
target_include_directories(app_key PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/APP_Key)
target_link_libraries(app_key
    PUBLIC  bsp_gpio app_led app_uart
    PRIVATE stm32cubemx
)
target_compile_options(app_key PRIVATE ${USER_WARN_FLAGS})

# --- 串口业务（命令表） -----------------------------------------------------
add_library(app_uart STATIC
    APP_UART/app_uart.c
    APP_UART/app_uart_cmd.c
)
target_include_directories(app_uart PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/APP_UART)
target_link_libraries(app_uart
    PUBLIC  svc_uart svc_proto
    PRIVATE bsp_gpio stm32cubemx
)
target_compile_options(app_uart PRIVATE ${USER_WARN_FLAGS})
