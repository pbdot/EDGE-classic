cmake_minimum_required(VERSION 3.12..3.20)

set (EDGE_INCLUDE_DIRECTORIES "")
set (EDGE_LINK_LIBRARIES "")
set (EDGE_SOURCE_FILES "")

if (EMSCRIPTEN)
	include("cmake/EdgeWeb.cmake")
endif()

if (NOT EDGE_GL_ES2)
	set(EDGE_INCLUDE_DIRECTORIES ${EDGE_INCLUDE_DIRECTORIES} source_files/glad/include/glad)
	set(EDGE_LINK_LIBRARIES ${EDGE_LINK_LIBRARIES} glad)
	add_subdirectory(source_files/glad)
else()
	add_compile_options(-DEDGE_GL_ES2)
	set(EDGE_INCLUDE_DIRECTORIES ${EDGE_INCLUDE_DIRECTORIES} source_files/gl4es/include source_files/gl4es/include/GL)
	set(EDGE_LINK_LIBRARIES ${EDGE_LINK_LIBRARIES} gl4es)
	add_subdirectory(source_files/gl4es)
endif()

if (NOT EDGE_MINIMAL_SOUND)
	set(EDGE_INCLUDE_DIRECTORIES ${EDGE_INCLUDE_DIRECTORIES}
		source_files/libgme/gme
		source_files/libxmp
		source_files/crsid
		source_files/minimp3)

	set (EDGE_SOURCE_FILES ${EDGE_SOURCE_FILES}
		source_files/edge/s_flac.cc
		source_files/edge/s_sid.cc
		source_files/edge/s_vgm.cc
		source_files/edge/s_mp3.cc
		source_files/edge/s_gme.cc
		source_files/edge/s_xmp.cc)

	set(EDGE_LINK_LIBRARIES ${EDGE_LINK_LIBRARIES} libxmp gme crsid)

	add_subdirectory(source_files/crsid)
	add_subdirectory(source_files/libxmp)
	add_subdirectory(source_files/libgme)
else()
	add_compile_options(-DEDGE_MINIMAL_SOUND)
endif()

add_subdirectory(source_files/ajbsp)
add_subdirectory(source_files/coal)
add_subdirectory(source_files/ddf)
add_subdirectory(source_files/dehacked)
add_subdirectory(source_files/ec_voxelib)
add_subdirectory(source_files/epi)
add_subdirectory(source_files/fmmidi)
add_subdirectory(source_files/fluidlite)
add_subdirectory(source_files/libymfm)
add_subdirectory(source_files/miniz)
add_subdirectory(source_files/ymfmidi)

project(
  edge-classic
  LANGUAGES CXX
  VERSION 0.1.0
)

if (NOT EMSCRIPTEN)
	set(CMAKE_MODULE_PATH cmake)
	if(MSVC OR MINGW)
		set(SDL2_DIR "${CMAKE_SOURCE_DIR}/source_files/sdl2")
  	endif()
	if (NOT EDGE_GL_ES2)
		find_package(OpenGL REQUIRED)
		set (EDGE_LINK_LIBRARIES ${EDGE_LINK_LIBRARIES} ${OPENGL_LIBRARIES})
	endif()
	find_package(SDL2 REQUIRED)
	set (EDGE_LINK_LIBRARIES ${EDGE_LINK_LIBRARIES} ${SDL2_LIBRARIES})
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (NOT MSVC AND NOT EMSCRIPTEN)
	set(CMAKE_CXX_FLAGS "-O2 -Wall")
endif()

if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast /EHs")
  set(CMAKE_EXE_LINKER_FLAGS "/SUBSYSTEM:WINDOWS")
  set(EDGE_INCLUDE_DIRECTORIES ${EDGE_INCLUDE_DIRECTORIES} source_files/sdl2/include)
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math -fno-strict-aliasing")
endif()

if (MSVC OR MINGW OR MSYS)
	add_compile_options(-DWIN32)
	set (EDGE_SOURCE_FILES ${EDGE_SOURCE_FILES} source_files/edge/w32_res.rc)
	set (EDGE_LINK_LIBRARIES ${EDGE_LINK_LIBRARIES} wsock32)
else()
	add_compile_options(-DUNIX)
endif()

if(MSYS)
  set(CMAKE_EXE_LINKER_FLAGS "-static -mwindows")
endif()
if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm64" AND APPLE)
	add_compile_definitions(APPLE_SILICON)
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64" AND APPLE)
	add_compile_definitions(NOT_APPLE_SILICON)
endif()
if(APPLE OR ${CMAKE_SYSTEM} MATCHES "BSD")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I /usr/local/include")
endif()


set (EDGE_SOURCE_FILES ${EDGE_SOURCE_FILES}
	source_files/edge/i_main.cc
	source_files/edge/i_ctrl.cc
	source_files/edge/i_video.cc
	source_files/edge/i_sound.cc
	source_files/edge/i_net.cc
	source_files/edge/am_map.cc
	source_files/edge/bot_nav.cc
	source_files/edge/bot_think.cc
	source_files/edge/con_con.cc
	source_files/edge/con_main.cc
	source_files/edge/con_var.cc
	source_files/edge/e_input.cc
	source_files/edge/e_main.cc
	source_files/edge/e_player.cc
	source_files/edge/f_finale.cc
	source_files/edge/f_interm.cc
	source_files/edge/g_game.cc
	source_files/edge/hu_draw.cc
	source_files/edge/hu_font.cc
	source_files/edge/hu_stuff.cc
	source_files/edge/hu_style.cc
	source_files/edge/i_system.cc
	source_files/edge/l_ajbsp.cc
	source_files/edge/l_deh.cc
	source_files/edge/m_argv.cc
	source_files/edge/m_bbox.cc
	source_files/edge/m_cheat.cc
	source_files/edge/m_math.cc
	source_files/edge/m_menu.cc
	source_files/edge/m_misc.cc
	source_files/edge/m_option.cc
	source_files/edge/m_netgame.cc
	source_files/edge/m_random.cc
	source_files/edge/n_bcast.cc
	source_files/edge/n_reliable.cc
	source_files/edge/n_network.cc
	source_files/edge/p_action.cc
	source_files/edge/p_blockmap.cc
	source_files/edge/p_enemy.cc
	source_files/edge/p_inter.cc
	source_files/edge/p_lights.cc
	source_files/edge/p_map.cc
	source_files/edge/p_maputl.cc
	source_files/edge/p_mobj.cc
	source_files/edge/p_plane.cc
	source_files/edge/p_setup.cc
	source_files/edge/p_sight.cc
	source_files/edge/p_spec.cc
	source_files/edge/p_switch.cc
	source_files/edge/p_tick.cc
	source_files/edge/p_user.cc
	source_files/edge/p_forces.cc
	source_files/edge/p_telept.cc
	source_files/edge/p_weapon.cc
	source_files/edge/rad_act.cc
	source_files/edge/rad_pars.cc
	source_files/edge/rad_trig.cc
	source_files/edge/r_draw.cc
	source_files/edge/r_shader.cc
	source_files/edge/r_render.cc
	source_files/edge/r_effects.cc
	source_files/edge/r_main.cc
	source_files/edge/r_occlude.cc
	source_files/edge/r_things.cc
	source_files/edge/r_units.cc
	source_files/edge/r_wipe.cc
	source_files/edge/r_misc.cc
	source_files/edge/r_sky.cc
	source_files/edge/r_colormap.cc
	source_files/edge/r_modes.cc
	source_files/edge/r_mdl.cc
	source_files/edge/r_md2.cc
	source_files/edge/r_voxel.cc
	source_files/edge/r_image.cc
	source_files/edge/r_doomtex.cc
	source_files/edge/r_texgl.cc
	source_files/edge/s_blit.cc
	source_files/edge/s_cache.cc
	source_files/edge/s_sound.cc
	source_files/edge/s_music.cc
	source_files/edge/s_ogg.cc
	source_files/edge/s_fluid.cc
	source_files/edge/s_opl.cc
	source_files/edge/s_wav.cc
	source_files/edge/s_fmm.cc
	source_files/edge/sv_chunk.cc
	source_files/edge/sv_glob.cc
	source_files/edge/sv_level.cc
	source_files/edge/sv_load.cc
	source_files/edge/sv_main.cc
	source_files/edge/sv_misc.cc
	source_files/edge/sv_mobj.cc
	source_files/edge/sv_play.cc
	source_files/edge/sv_save.cc
	source_files/edge/w_files.cc
	source_files/edge/w_flat.cc
	source_files/edge/w_model.cc
	source_files/edge/w_pk3.cc
	source_files/edge/w_sprite.cc
	source_files/edge/w_texture.cc
	source_files/edge/w_wad.cc
	source_files/edge/vm_coal.cc
	source_files/edge/vm_hud.cc
	source_files/edge/vm_player.cc
	source_files/edge/scanner.cc
	source_files/edge/umapinfo.cc
)

set(EDGE_INCLUDE_DIRECTORIES ${EDGE_INCLUDE_DIRECTORIES}
	source_files/ajbsp
	source_files/coal
	source_files/ddf
	source_files/dehacked
	source_files/dr_libs
	source_files/ec_voxelib
	source_files/epi
	source_files/fluidlite/include
	source_files/fmmidi
	source_files/libymfm
	source_files/minivorbis
	source_files/miniz
	source_files/stb
	source_files/ymfmidi
)

set(EDGE_LINK_LIBRARIES ${EDGE_LINK_LIBRARIES}
	edge_ajbsp
	edge_coal
	edge_ddf
	edge_deh
	edge_epi
	ec_voxelib
	fmmidi
	fluidlite
	miniz
	libymfm
	ymfmidi
)

add_executable(edge-classic ${EDGE_SOURCE_FILES})
target_include_directories(edge-classic PRIVATE ${EDGE_INCLUDE_DIRECTORIES})
target_link_libraries(edge-classic PRIVATE ${EDGE_LINK_LIBRARIES})

# Copies executable to local install directory after build
add_custom_command(
TARGET edge-classic
POST_BUILD
COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:edge-classic>"
		"${CMAKE_CURRENT_LIST_DIR}"
)

# Copy appropriate SDL2.dll to local install directory when built with MSVC
if(WIN32 AND NOT MSYS AND NOT MINGW)
  if (${CMAKE_SIZEOF_VOID_P} MATCHES 8)
    add_custom_command(
	TARGET edge-classic
	POST_BUILD
	COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/source_files/sdl2/lib/x64/SDL2.dll"
			"${CMAKE_CURRENT_LIST_DIR}"
	)
  else ()
    add_custom_command(
	TARGET edge-classic
	POST_BUILD
	COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/source_files/sdl2/lib/x86/SDL2.dll"
			"${CMAKE_CURRENT_LIST_DIR}"
	)
  endif ()
# Copy appropriate SDL2.dll to local install directory when built with MSYS2
elseif(MSYS)
	if (${CMAKE_SIZEOF_VOID_P} MATCHES 8)
	add_custom_command(
	TARGET edge-classic
	POST_BUILD
	COMMAND "${CMAKE_COMMAND}" -E copy "/mingw64/bin/SDL2.dll"
			"${CMAKE_CURRENT_LIST_DIR}"
	)
	else ()
	add_custom_command(
	TARGET edge-classic
	POST_BUILD
	COMMAND "${CMAKE_COMMAND}" -E copy "/mingw32/bin/SDL2.dll"
			"${CMAKE_CURRENT_LIST_DIR}"
		COMMAND "${CMAKE_COMMAND}" -E copy "/mingw32/bin/libgcc_s_dw2-1.dll"
			"${CMAKE_CURRENT_LIST_DIR}"
		COMMAND "${CMAKE_COMMAND}" -E copy "/mingw32/bin/libstdc++-6.dll"
			"${CMAKE_CURRENT_LIST_DIR}"
		COMMAND "${CMAKE_COMMAND}" -E copy "/mingw32/bin/libwinpthread-1.dll"
			"${CMAKE_CURRENT_LIST_DIR}"
	)
	endif ()
endif()