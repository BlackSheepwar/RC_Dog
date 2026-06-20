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
set(CMAKE_PROJECT_NAME FrontDog)

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
    # Add user sources here
    # APP
    User/APP/APP_Uart/app_uart.c
    User/APP/APP_Uart/app_uart_cmd.c
    User/APP/APP_Key/app_key.c
    User/APP/APP_Key/app_key_cmd.c
    User/APP/APP_Servo/app_servo.c
    User/APP/APP_Motion/app_gait.c
    User/APP/APP_Motion/app_gait.h
    User/APP/APP_Motion/app_limb.c
    User/APP/APP_Motion/app_limb.h
    User/APP/APP_CAN/app_can.c
    User/APP/APP_CAN/app_can_cmd_1.c
    User/APP/APP_CAN/app_can_cmd_2.c
    
    # BSP
    User/BSP/BSP_GPIO/bsp_gpio.c
    User/BSP/BSP_Sys/bsp_sys.c
    User/BSP/BSP_Uart/bsp_uart.c
    User/BSP/BSP_PWM/bsp_pwm.c
    User/BSP/BSP_CAN/bsp_can.c

    # Middlewares
    User/Middlewares/Codec/codec.c
    User/Middlewares/IK/ik_two_link.c
    User/Middlewares/IK/ik_two_link.h
    User/Third_Party/OLED/oled.c
    User/Third_Party/OLED/oled_extend.c
    User/Third_Party/OLED/font.c
    # TASK
    User/TASK/task_uart_rx.c
    User/TASK/task_uart_rx_cmd.c
    User/TASK/task_uart_tx.c
    User/TASK/task_key_debounce.c
    User/TASK/task_key_cmd.c
    User/TASK/task_servo.c
    User/TASK/task_can_rxf0.c
    User/TASK/task_can_rxf1.c
    User/TASK/task_oled.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    # APP
    User/APP/APP_Uart
    User/APP/APP_Key
    User/APP/APP_Servo
    User/APP/APP_Motion
    User/APP/APP_CAN

    # Common
    User/Common

    # BSP
    User/BSP/BSP_GPIO
    User/BSP/BSP_Sys
    User/BSP/BSP_Uart
    User/BSP/BSP_PWM
    User/BSP/BSP_CAN

    # Middlewares
    User/Middlewares/Codec
    User/Middlewares/IK
    User/Third_Party/OLED
    # TASK
    User/TASK
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
