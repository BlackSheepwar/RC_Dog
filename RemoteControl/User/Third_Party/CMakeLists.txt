# =============================================================================
# Third_Party 层：别人写的代码，只读
#
# 一律 -w：第三方库的告警不该变成我们的错误。
# =============================================================================

# --- GFIFO 环形缓冲 ---------------------------------------------------------
add_library(gfifo STATIC
    GFIFO/gfifo.c
)
target_include_directories(gfifo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/GFIFO)
target_compile_options(gfifo PRIVATE -w)
