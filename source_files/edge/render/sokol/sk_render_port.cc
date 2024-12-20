#include "r_sky.h"

#include <math.h>

#include "dm_state.h"
#include "epi.h"
#include "g_game.h" // current_map
#include "i_defs_gl.h"
#include "im_data.h"
#include "m_math.h"
#include "r_colormap.h"
#include "r_gldefs.h"
#include "r_image.h"
#include "r_misc.h"
#include "r_modes.h"
#include "r_sky.h"
#include "r_texgl.h"
#include "r_units.h"
#include "r_render_port.h"
#include "stb_sprintf.h"
#include "w_flat.h"
#include "w_wad.h"

extern ConsoleVariable fliplevels;
extern SkyStretch current_sky_stretch;

void SetupSkyMatrices(void)
{

}

void RendererRevertSkyMatrices(void)
{
}

GLuint UploadTexture(ImageData *img, int flags, int max_pix)
{
    return 0;
}
