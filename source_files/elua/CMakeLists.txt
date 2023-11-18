
set (ELUA_SOURCES
    elua_module.cc
    elua_vm.cc)

add_library(edge_lua ${ELUA_SOURCES})

target_include_directories(edge_lua PUBLIC ./ ../epi ../../libraries/lua ../../libraries/luabridge)
target_link_libraries(edge_lua PUBLIC lua)