
#include "hu_font.h"

#include "ddf_font.h"
#include "ddf_main.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "epi.h"
#include "epi_filesystem.h"
#include "epi_str_compare.h"
#include "epi_str_util.h"
#include "i_defs_gl.h"
#include "im_data.h"
#include "r_colormap.h"
#include "r_draw.h"
#include "r_image.h"
#include "r_misc.h"
#include "r_modes.h"
#include "r_texgl.h"
#include "stb_truetype.h"
#include "w_files.h"
#include "w_wad.h"

extern ImageData *ReadAsEpiBlock(Image *rim);

void Font::LoadPatches()
{
}
