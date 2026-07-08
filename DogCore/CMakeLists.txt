cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME DogCore)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # User sources

    ## APP层
    # CAN
    User/APP/APP_CAN/app_can_cmd_1.c
    User/APP/APP_CAN/app_can_cmd_2.c
    User/APP/APP_CAN/app_can.c
    # KEY
    User/APP/APP_Key/app_key_cmd.c
    User/APP/APP_Key/app_key.c
    # MOTION
    User/APP/APP_Motion/app_motion_ik.c
    User/APP/APP_Motion/app_motion_scheduler.c
    User/APP/APP_Motion/app_motion.c
    # MOTOR
    User/APP/APP_Motor/app_motor.c
    User/APP/APP_Motor/app_motor_debug.c
    # SERVO
    User/APP/APP_Servo/app_servo.c
    # UART
    User/APP/APP_Uart/app_uart_cmd.c
    User/APP/APP_Uart/app_uart.c

    ## BSP层
    User/BSP/BSP_CAN/bsp_can.c          # CAN 
    User/BSP/BSP_Encoder/bsp_encoder.c  # ENCODER
    User/BSP/BSP_GPIO/bsp_gpio.c        # GPIO
    User/BSP/BSP_PWM/bsp_pwm.c          # PWM
    User/BSP/BSP_Sys/bsp_sys.c          # SYS
    User/BSP/BSP_Uart/bsp_uart.c        # UART

    ## Common层
    User/Common/common_utils.c

    ## Middlewares层
    User/Middlewares/Codec/codec.c      # CODEC
    User/Middlewares/IK/ik_two_link.c   # IK
    User/Middlewares/PID/pid.c          # PID

    ## TASK层
    # CAN
    User/TASK/TASK_CAN/task_can_rxf0.c
    User/TASK/TASK_CAN/task_can_rxf1.c
    User/TASK/TASK_CAN/task_can_tx.c
    # Execute
    User/TASK/TASK_Execute/task_motion.c    
    User/TASK/TASK_Execute/task_motor.c
    User/TASK/TASK_Execute/task_servo.c
    # KEY
    User/TASK/TASK_KEY/task_key_cmd.c  
    User/TASK/TASK_KEY/task_key_debounce.c
    # OLED
    User/TASK/TASK_OLED/task_oled.c
    # UART
    User/TASK/TASK_UART/task_uart_rx_cmd.c    
    User/TASK/TASK_UART/task_uart_rx.c
    User/TASK/TASK_UART/task_uart_tx.c

    ## Third Party层
    # OLED
    User/Third_Party/OLED/font.c
    User/Third_Party/OLED/oled_extend.c
    User/Third_Party/OLED/oled_ui.c
    User/Third_Party/OLED/oled.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # User include directories

    ## APP
    User/APP/APP_CAN                # CAN
    User/APP/APP_Key                # KEY
    User/APP/APP_Motion             # MOTION
    User/APP/APP_Motor              # MOTOR
    User/APP/APP_Servo              # SERVO
    User/APP/APP_Uart               # UART

    ## Common
    User/Common

    ## BSP
    User/BSP/BSP_CAN                # CAN 
    User/BSP/BSP_Encoder            # ENCODER
    User/BSP/BSP_GPIO               # GPIO
    User/BSP/BSP_PWM                # PWM    
    User/BSP/BSP_Sys                # SYS
    User/BSP/BSP_Uart               # UART

    ## Middlewares
    User/Middlewares/Codec           # CODEC
    User/Middlewares/IK              # IK
    User/Middlewares/PID             # PID

    ## Third Party
    User/Third_Party/OLED            # OLED
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
