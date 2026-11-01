# =============================================================================
# Device 层：一台设备一个模块，知道"这是什么设备"，但不知道业务
#
# 允许调 BSP，禁止 include Service/APP；不延时、不重试、不去抖。
# 引脚与总线句柄都来自 BSP，本层不出现端口号与寄存器。
# =============================================================================

# --- OLED 显示屏（SSD1306 @ I2C2） ------------------------------------------
# @note 驱动本体（oled.c/font.c）沿用原厂写法，函数名保持 OLED_*；
#       唯一的总线调用 OLED_Send() 已经收口到 BSP_IIC_Write。
#       oled_ui.c 是画 FPS/计数的显示组件，会读旋钮编码器，所以还要 bsp_encoder。
add_library(dev_oled STATIC
    Dev_Oled/oled.c
    Dev_Oled/font.c
    Dev_Oled/oled_extend.c
    Dev_Oled/oled_ui.c
)
target_include_directories(dev_oled PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Dev_Oled)
target_link_libraries(dev_oled
    PUBLIC  bsp_iic
    PRIVATE bsp_encoder stm32cubemx
)
target_compile_options(dev_oled PRIVATE ${USER_WARN_FLAGS})
