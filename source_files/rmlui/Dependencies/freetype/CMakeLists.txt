

set (SOURCE_FILES
    src/autofit/autofit.c
    src/base/ftbase.c
    src/base/ftbbox.c
    src/base/ftbdf.c
    src/base/ftbitmap.c
    src/base/ftcid.c
    src/base/ftfstype.c
    src/base/ftgasp.c
    src/base/ftglyph.c
    src/base/ftgxval.c
    src/base/ftinit.c
    src/base/ftmm.c
    src/base/ftotval.c
    src/base/ftpatent.c
    src/base/ftpfr.c
    src/base/ftstroke.c
    src/base/ftsynth.c
    src/base/fttype1.c
    src/base/ftwinfnt.c
    src/bdf/bdf.c
    src/bzip2/ftbzip2.c
    src/cache/ftcache.c
    src/cff/cff.c
    src/cid/type1cid.c
    src/gzip/ftgzip.c
    src/lzw/ftlzw.c
    src/pcf/pcf.c
    src/pfr/pfr.c
    src/psaux/psaux.c
    src/pshinter/pshinter.c
    src/psnames/psnames.c
    src/raster/raster.c
    src/sdf/sdf.c
    src/sfnt/sfnt.c
    src/smooth/smooth.c
    src/svg/svg.c
    src/truetype/truetype.c
    src/type1/type1.c
    src/type42/type42.c
    src/winfonts/winfnt.c)

if (UNIX)
    list(APPEND SOURCE_FILES "builds/unix/ftsystem.c")
elseif (WIN32)
    list(APPEND SOURCE_FILES "builds/windows/ftsystem.c" "builds/windows/ftdebug.c")
else ()
    list(APPEND SOURCE_FILES "src/base/ftsystem.c")
endif ()



add_library(RmlUi_FreeType STATIC ${SOURCE_FILES})

target_include_directories(RmlUi_FreeType PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_compile_definitions(RmlUi_FreeType PRIVATE -DFT2_BUILD_LIBRARY)