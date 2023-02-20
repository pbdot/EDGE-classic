
add_library(ymfmidi
  patches.cpp
  patchnames.cpp
  ymf_player.cpp
)

target_include_directories(ymfmidi PRIVATE ../libymfm)
target_include_directories(ymfmidi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})