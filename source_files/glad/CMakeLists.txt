
add_library(glad src/gl.c)

target_link_libraries(glad PUBLIC ${CMAKE_DL_LIBS})

target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/glad)
