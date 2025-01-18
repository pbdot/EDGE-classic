
set (SMC_SOURCES 
    smc.cc        
    sokol_nuklear.c)

add_library(smc ${SMC_SOURCES})
target_link_libraries(smc epi moonnuklear sokol)

target_include_directories(smc PUBLIC ./)