# =============================================================================
# Service 层：时间轴、流程、协议解析，与产品无关
# =============================================================================

# --- 帧协议（纯函数，可在 PC 上单测） ---------------------------------------
add_library(svc_proto STATIC
    Svc_Proto/svc_proto.c
)
target_include_directories(svc_proto PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Svc_Proto)
target_compile_options(svc_proto PRIVATE ${USER_WARN_STRICT})

# --- 串口链路（字节流→帧、帧队列、发送泵） ---------------------------------
add_library(svc_uart STATIC
    Svc_Uart/svc_uart.c
)
target_include_directories(svc_uart PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Svc_Uart)
target_link_libraries(svc_uart
    PUBLIC  svc_proto
    PRIVATE bsp_uart gfifo common_utils stm32cubemx
)
target_compile_options(svc_uart PRIVATE ${USER_WARN_FLAGS})
