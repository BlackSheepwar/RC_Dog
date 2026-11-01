# =============================================================================
# 用户层总入口
#
# 这一层只做三件事：
#   1. 打开告警开关（必须在 add_subdirectory 之前，子目录才能继承）
#   2. 逐层收集（一行一层，add_subdirectory 只允许出现在本文件）
#   3. 把全部用户库挂到主可执行文件上
#
# 子目录顺序 = 依赖顺序（下层在前）。
# =============================================================================

set(USER_WARN_FLAGS  -Wall -Wextra -Wno-unused-parameter)
set(USER_WARN_STRICT ${USER_WARN_FLAGS} -Werror)

# --- 横向层：谁都能用 -------------------------------------------------------
add_subdirectory(Common)
# --- 硬件抽象层 -------------------------------------------------------------
add_subdirectory(BSP)
# --- 设备层（屏、按键、传感器） ---------------------------------------------
add_subdirectory(Device)
# --- 第三方只读区 -----------------------------------------------------------
add_subdirectory(Third_Party)
# --- 服务层 -----------------------------------------------------------------
add_subdirectory(Service)
# --- 业务层 -----------------------------------------------------------------
add_subdirectory(APP)
# --- 线程层 -----------------------------------------------------------------
add_subdirectory(TASK)

# --- 挂到主目标：plain 签名，不带 PUBLIC/PRIVATE -----------------------------
target_link_libraries(${CMAKE_PROJECT_NAME}
    # Common
    common_utils
    # Third_Party
    gfifo
    # BSP
    bsp_gpio
    bsp_sys
    bsp_encoder
    bsp_uart
    bsp_iic
    # Device
    dev_oled
    # Service
    svc_proto
    svc_uart
    # APP
    app_key
    app_led
    app_uart
    # TASK（OBJECT 库，直接进可执行文件）
    task_key
    task_oled
    task_uart
)
