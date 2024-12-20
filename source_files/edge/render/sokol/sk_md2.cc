#include <stddef.h>

#include <unordered_map>
#include <vector>

#include "ddf_types.h"
#include "dm_state.h" // EDGE_IMAGE_IS_SKY
#include "epi.h"
#include "epi_endian.h"
#include "epi_str_compare.h"
#include "g_game.h" //current_map
#include "i_defs_gl.h"
#include "n_network.h"
#include "p_blockmap.h"
#include "p_tick.h"
#include "r_colormap.h"
#include "r_effects.h"
#include "r_gldefs.h"
#include "r_image.h"
#include "r_md2.h"
#include "r_mdcommon.h"
#include "r_misc.h"
#include "r_modes.h"
#include "r_shader.h"
#include "r_state.h"
#include "r_units.h"

short MD2FindFrame(MD2Model *md, const char *name)
{
    return 0;
}

MD2Model *MD2Load(epi::File *f)
{
    return nullptr;
}

MD2Model *MD3Load(epi::File *f)
{
    return nullptr;
}

void MD2RenderModel(MD2Model *md, const Image *skin_img, bool is_weapon, int frame1, int frame2, float lerp, float x,
                    float y, float z, MapObject *mo, RegionProperties *props, float scale, float aspect, float bias,
                    int rotation)
{
}

void MD2RenderModel2D(MD2Model *md, const Image *skin_img, int frame, float x, float y, float xscale, float yscale,
                      const MapObjectDefinition *info)
{
}
