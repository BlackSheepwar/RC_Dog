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
    User/APP/APP_Usart/app_usart.c
    User/APP/APP_Usart/app_usart_cmd.c
    User/APP/APP_Key/app_key.c
    User/APP/APP_Key/app_key_cmd.c
    User/APP/APP_SE/app_se.c
    User/APP/APP_CAN/app_can.c
    User/APP/APP_CAN/app_can_cmd_1.c
    User/APP/APP_CAN/app_can_cmd_2.c

    # Common
    User/Common/protocol.c

    # BSP
    User/BSP/BSP_Key/bsp_key.c
    User/BSP/BSP_Usart/bsp_usart.c
    User/BSP/BSP_SE/bsp_se.c
    User/BSP/BSP_CAN/bsp_can.c

    # Middlewares
    User/Middlewares/Debounce/debounce.c
    User/Middlewares/Codec/codec.c
    User/Middlewares/OLED/oled.c
    User/Middlewares/OLED/oled_extend.c
    User/Middlewares/OLED/font.c
    # TASK
    User/TASK/task_rx.c
    User/TASK/task_tx.c
    User/TASK/task_key.c
    User/TASK/task_par.c
    User/TASK/task_se.c
    User/TASK/task_can_f0.c
    User/TASK/task_can_f1.c
    User/TASK/task_oled.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    # APP
    User/APP/APP_Usart
    User/APP/APP_Key
    User/APP/APP_SE
    User/APP/APP_CAN

    # Common
    User/Common

    # BSP
    User/BSP/BSP_Key
    User/BSP/BSP_Usart
    User/BSP/BSP_SE
    User/BSP/BSP_CAN

    # Middlewares
    User/Middlewares/Debounce
    User/Middlewares/Codec
    User/Middlewares/OLED
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
