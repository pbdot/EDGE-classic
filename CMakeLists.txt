cmake_minimum_required(VERSION 3.12..3.20)

project(
  edge-classic
  LANGUAGES C CXX
  VERSION 0.1.0
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_MODULE_PATH cmake)
if(MSVC OR MINGW)
  set(SDL2_DIR "${CMAKE_SOURCE_DIR}/source_files/sdl2")
endif()

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast /EHs")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -ffast-math -fno-strict-aliasing")
endif()

add_subdirectory(source_files/ajbsp)
add_subdirectory(source_files/coal)
add_subdirectory(source_files/crsid)
add_subdirectory(source_files/ddf)
add_subdirectory(source_files/dehacked)
add_subdirectory(source_files/ec_voxelib)
add_subdirectory(source_files/epi)
add_subdirectory(source_files/fmmidi)
add_subdirectory(source_files/fluidlite)
add_subdirectory(source_files/glad)
add_subdirectory(source_files/libgme)
add_subdirectory(source_files/libymfm)
add_subdirectory(source_files/m4p)
add_subdirectory(source_files/miniz)
add_subdirectory(source_files/ymfmidi)
add_subdirectory(source_files/edge)



