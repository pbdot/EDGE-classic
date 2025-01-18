
add_library(smc smc.cc)
target_link_libraries(smc epi)

target_include_directories(smc PUBLIC ./)