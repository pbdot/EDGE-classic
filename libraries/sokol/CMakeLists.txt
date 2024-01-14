
add_library(sokol sokol.cc)

target_link_libraries(sokol imgui)

target_include_directories(sokol PUBLIC ./)