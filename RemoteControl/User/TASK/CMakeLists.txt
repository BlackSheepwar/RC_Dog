# =============================================================================
# TASK 层：线程体、周期、优先级；不写业务
#
# 用 OBJECT 库：线程入口被 CubeMX 生成的 freertos.c 引用，
# OBJECT 库保证 .o 一定进可执行文件。
# =============================================================================

# --- 串口三个线程 -----------------------------------------------------------
add_library(task_uart OBJECT
    TASK_UART/task_uart_rx.c
    TASK_UART/task_uart_tx.c
    TASK_UART/task_uart_rx_cmd.c
)
target_link_libraries(task_uart
    PUBLIC  svc_uart svc_proto app_uart
    PRIVATE stm32cubemx
)
target_compile_options(task_uart PRIVATE ${USER_WARN_FLAGS})

# --- 按键两个线程 -----------------------------------------------------------
add_library(task_key OBJECT
    TASK_KEY/task_key_cmd.c
    TASK_KEY/task_key_debounce.c
)
target_link_libraries(task_key
    PUBLIC  app_key
    PRIVATE stm32cubemx
)
target_compile_options(task_key PRIVATE ${USER_WARN_FLAGS})

# --- OLED 线程 --------------------------------------------------------------
add_library(task_oled OBJECT
    TASK_OLED/task_oled.c
)
target_link_libraries(task_oled
    PUBLIC  dev_oled
    PRIVATE stm32cubemx
)
target_compile_options(task_oled PRIVATE ${USER_WARN_FLAGS})
