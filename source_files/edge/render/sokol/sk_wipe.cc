#include "r_wipe.h"

#include "i_defs_gl.h"
#include "i_system.h"
#include "im_data.h"
#include "m_random.h"
#include "n_network.h"
#include "r_gldefs.h"
#include "r_image.h"
#include "r_modes.h"
#include "r_texgl.h"
#include "r_units.h"

extern float  current_wipe_right;
extern float  current_wipe_top;
extern GLuint current_wipe_texture;

static inline uint8_t SpookyAlpha(int x, int y)
{
    y += (x & 32) / 2;

    x = (x & 31) - 15;
    y = (y & 31) - 15;

    return (x * x + y * y) / 2;
}

void CaptureScreenAsTexture(bool speckly, bool spooky)
{

}
