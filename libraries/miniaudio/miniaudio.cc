#define MA_NO_ENCODING
#define MA_NO_GENERATION
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_WINMM
#define MA_NO_DSOUND
// We use our own custom minivorbis decoder
#define MA_NO_VORBIS
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#define VERBLIB_IMPLEMENTATION
#include "ma_reverb_node.cc"