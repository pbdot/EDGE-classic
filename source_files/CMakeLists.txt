
if (EDGE_SNAPMAP)
  add_definitions(-DEDGE_SNAPMAP)
  add_subdirectory(smc)
endif()

add_subdirectory(ajbsp)
add_subdirectory(ddf)
add_subdirectory(epi)
add_subdirectory(edge)

if (EDGE_CLASSIC)
  add_subdirectory(coal)
  add_subdirectory(dehacked)
endif()