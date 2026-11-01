# =============================================================================
# Common 层：纯软件
#
#   这一层不依赖任何人：没有 HAL、没有 OS、没有句柄，只有数学与工具函数，
#   因此可以直接拿宿主 gcc 编译并单测（本工程里 common_pid / common_ik
#   都是这样验过的）。
#
#   依赖方向：谁都可以用它，它不反向依赖任何人。
# =============================================================================

# -----------------------------------------------------------------------------
# common_utils：错误码、ARRAY_SIZE、float 转字符串
# -----------------------------------------------------------------------------
add_library(common_utils STATIC
    Common_Utils/common_utils.c
)
target_include_directories(common_utils
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Common_Utils
)
target_compile_options(common_utils PRIVATE ${USER_WARN_FLAGS})

# -----------------------------------------------------------------------------
# common_pid：PID 算法（位置式 + 增量式，上下文由调用方持有）
# -----------------------------------------------------------------------------
add_library(common_pid STATIC
    Common_Pid/common_pid.c
)
target_include_directories(common_pid
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Common_Pid
)
target_link_libraries(common_pid
    PRIVATE common_utils
)
target_compile_options(common_pid PRIVATE ${USER_WARN_FLAGS})

# -----------------------------------------------------------------------------
# common_ik：二连杆逆/正解（纯数学，杆长由调用方给）
#   libm 要出现在 libcommon_ik.a 之后，所以把 m 写成它的依赖，让 CMake 排序
# -----------------------------------------------------------------------------
add_library(common_ik STATIC
    Common_Ik/common_ik.c
)
target_include_directories(common_ik
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Common_Ik
)
target_link_libraries(common_ik
    PRIVATE common_utils
            m
)
target_compile_options(common_ik PRIVATE ${USER_WARN_FLAGS})
