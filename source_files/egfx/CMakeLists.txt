
set (EGFX_SOURCES
    private/gfx_main.cc )

add_library(edge_gfx ${EGFX_SOURCES})

target_include_directories(edge_gfx PUBLIC ./public ${CMAKE_SOURCE_DIR}/libraries/hmm)
target_link_libraries(edge_gfx sokol)
