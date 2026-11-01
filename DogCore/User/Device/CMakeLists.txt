# =============================================================================
# Device 层：设备抽象层
#
#   把"一台舵机""一台电机""一个按键"封装成设备，对外只暴露能力，不暴露引脚。
#   每个模块在自己的 .c 里实例化设备句柄与引脚/通道句柄，头文件只做 extern 声明。
#
#   本层不延时、不重试、不去抖、不跑闭环：只回答"给这个值，硬件现在是什么"。
#   句柄里出现了 bsp_xxx_t 指针（bsp_pwm_t、bsp_gpio_t、bsp_encoder_t），
#   所以那几个 BSP 模块必须 PUBLIC 传递，否则上层拿到句柄也看不见声明。
# =============================================================================

# -----------------------------------------------------------------------------
# dev_key：单个按键（引脚 + 极性，只回答"按下没有"）
#   消抖与长短按在 Service/Svc_Key_Logic
# -----------------------------------------------------------------------------
add_library(dev_key STATIC
    Dev_Key/dev_key.c
)
target_include_directories(dev_key
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Dev_Key
)
target_link_libraries(dev_key
    PRIVATE bsp_gpio
            common_utils
)
target_compile_options(dev_key PRIVATE ${USER_WARN_FLAGS})

# -----------------------------------------------------------------------------
# dev_led：单个指示灯（引脚 + 极性）
#   闪烁/呼吸这类带时间轴的花样属于 Service，不在本层
# -----------------------------------------------------------------------------
add_library(dev_led STATIC
    Dev_Led/dev_led.c
)
target_include_directories(dev_led
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Dev_Led
)
target_link_libraries(dev_led
    PRIVATE bsp_gpio
)
target_compile_options(dev_led PRIVATE ${USER_WARN_FLAGS})

# -----------------------------------------------------------------------------
# dev_servo：一台舵机（一个 PWM 通道 + 安装参数 + 角度→脉宽换算）
#   平滑/限速在 Service/Svc_Servo_Motion
# -----------------------------------------------------------------------------
add_library(dev_servo STATIC
    Dev_Servo/dev_servo.c
)
target_include_directories(dev_servo
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Dev_Servo
)
target_link_libraries(dev_servo
    PRIVATE bsp_pwm
            common_utils
)
target_compile_options(dev_servo PRIVATE ${USER_WARN_FLAGS})

# -----------------------------------------------------------------------------
# dev_motor：一台电机（双 PWM + 编码器 + 每转脉冲数 + 安装极性）
#   测速与 PID 在 Service/Svc_Motor_Speed
# -----------------------------------------------------------------------------
add_library(dev_motor STATIC
    Dev_Motor/dev_motor.c
)
target_include_directories(dev_motor
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Dev_Motor
)
target_link_libraries(dev_motor
    PRIVATE bsp_pwm
            bsp_encoder
            common_utils
)
target_compile_options(dev_motor PRIVATE ${USER_WARN_FLAGS})

# -----------------------------------------------------------------------------
# dev_oled：SSD1306 屏（I2C2）
#   驱动本体沿用原厂写法，函数名保持 OLED_*；唯一的总线调用已收口到 BSP_IIC_Write
# -----------------------------------------------------------------------------
add_library(dev_oled STATIC
    Dev_Oled/oled.c
    Dev_Oled/oled_extend.c
    Dev_Oled/font.c
    Dev_Oled/oled_ui.c
)
target_include_directories(dev_oled
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Dev_Oled
)
target_link_libraries(dev_oled
    PUBLIC  bsp_iic
    PRIVATE bsp_encoder
            bsp_sys
            common_utils
)
target_compile_options(dev_oled PRIVATE ${USER_WARN_FLAGS})
