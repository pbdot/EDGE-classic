
add_library(
  crsid
  libcRSID.c
)

target_include_directories(crsid PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})