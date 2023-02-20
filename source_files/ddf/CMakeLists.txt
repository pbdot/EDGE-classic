
add_library(
  edge_ddf
  anim.cc
  attack.cc
  boom.cc
  colormap.cc
  flat.cc
  font.cc
  game.cc
  image.cc
  language.cc
  level.cc
  line.cc
  main.cc
  playlist.cc
  sector.cc
  sfx.cc
  states.cc
  style.cc
  switch.cc
  thing.cc
  wadfixes.cc
  weapon.cc
)

target_include_directories(edge_ddf PRIVATE ../edge ../epi)

target_include_directories(edge_ddf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
