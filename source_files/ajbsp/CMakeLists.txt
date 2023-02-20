# AJBSP CMake Script

if (NOT MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fwrapv -fno-exceptions -fno-rtti")
endif()




add_library(
	edge_ajbsp
	ajbsp.cc
	bsp_level.cc
	bsp_node.cc
	bsp_util.cc
	lib_util.cc
	w_wad.cc
)

target_include_directories(edge_ajbsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})