
set (SMC_SOURCES 
    smc.cc        
    sokol_nuklear.c)

add_library(smc ${SMC_SOURCES})
target_link_libraries(smc epi moonnuklear sokol)

target_include_directories(smc PUBLIC ./)

if(WIN32 AND (MSVC OR CLANG))
  target_include_directories(smc PRIVATE ${EDGE_LIBRARY_DIR}/sdl2/msvc/include)
elseif (MINGW)
  target_include_directories(smc SYSTEM PRIVATE ${EDGE_LIBRARY_DIR}/sdl2/mingw/include)
endif()
