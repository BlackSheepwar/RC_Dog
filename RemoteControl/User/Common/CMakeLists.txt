# =============================================================================
# Common 层：纯软件，可拿到 PC 上编译
# =============================================================================

# --- 通用定义与工具 ---------------------------------------------------------
add_library(common_utils STATIC
    Common_Utils/common_utils.c
)
target_include_directories(common_utils PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Common_Utils
)
target_compile_options(common_utils PRIVATE ${USER_WARN_FLAGS})
