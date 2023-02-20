
add_library(
  miniz
  miniz.c 
  mini_gzip.c)

  target_include_directories(miniz PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})