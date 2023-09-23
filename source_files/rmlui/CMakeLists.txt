

add_subdirectory(Dependencies/freetype)

# Include list of source files
include("${CMAKE_CURRENT_SOURCE_DIR}/FileList.cmake")

# RmlUi Core Library
add_library(RmlUi_Core ${Core_HDR_FILES} ${Core_PUB_HDR_FILES} ${MASTER_Core_PUB_HDR_FILES} ${Core_SRC_FILES})
target_include_directories(RmlUi_Core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Include)
target_compile_definitions(RmlUi_Core PUBLIC -DRMLUI_STATIC_LIB)
target_link_libraries(RmlUi_Core PRIVATE RmlUi_FreeType)

# RmlUi Debugger Library
add_library(RmlUi_Debugger ${Debugger_HDR_FILES} ${Debugger_PUB_HDR_FILES} ${MASTER_Debugger_PUB_HDR_FILES} ${Debugger_SRC_FILES})
target_include_directories(RmlUi_Debugger PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Include)
target_link_libraries(RmlUi_Debugger PRIVATE RmlUi_Core RmlUi_FreeType)

# RmlUi Lua Library
add_library(RmlUi_Lua ${Lua_HDR_FILES} ${Lua_PUB_HDR_FILES} ${MASTER_Lua_PUB_HDR_FILES} ${Lua_SRC_FILES})
target_include_directories(RmlUi_Lua PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Include)
target_link_libraries(RmlUi_Lua PRIVATE RmlUi_Core lua )

# ToDo: SVG