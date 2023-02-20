
add_library(fmmidi
  filter.cpp
  midisynth.cpp
)

target_include_directories(fmmidi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})