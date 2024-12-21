
add_library(sokol sokol.cc)

target_link_libraries(sokol imgui)

target_include_directories(sokol PUBLIC ./)
target_compile_definitions(sokol PUBLIC SOKOL_GLCORE SOKOL_NO_DEPRECATED SOKOL_TRACE_HOOKS SOKOL_IMGUI_NO_SOKOL_APP)