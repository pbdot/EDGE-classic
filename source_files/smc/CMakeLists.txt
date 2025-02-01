
file(GLOB SMC_SOURE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

add_library(smc ${SMC_SOURE_FILES})

target_include_directories(smc PUBLIC ./)
target_link_libraries(smc PUBLIC imgui)