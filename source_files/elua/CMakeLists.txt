
set (ELUA_SOURCES
    elua_vm.cc)

add_library(elua ${ELUA_SOURCES})

target_include_directories(elua PUBLIC ../epi ../../libraries/lua ../../libraries/luabridge)


