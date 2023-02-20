
add_library(
  edge_coal
  c_compile.cc
  c_execute.cc
  c_memory.cc
)

target_include_directories(edge_coal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})