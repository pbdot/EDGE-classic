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
    int total_w = MakeValidTextureSize(current_screen_width);
    int total_h = MakeValidTextureSize(current_screen_height);

    ImageData img(total_w, total_h, 4);

    img.Clear();

    current_wipe_right = current_screen_width / (float)total_w;
    current_wipe_top   = current_screen_height / (float)total_h;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (int y = 0; y < current_screen_height; y++)
    {
        uint8_t *dest = img.PixelAt(0, y);

        glReadPixels(0, y, current_screen_width, 1, GL_RGBA, GL_UNSIGNED_BYTE, dest);

        int rnd_val = y;

        if (spooky)
        {
            for (int x = 0; x < total_w; x++)
                dest[4 * x + 3] = SpookyAlpha(x, y);
        }
        else if (speckly)
        {
            for (int x = 0; x < total_w; x++)
            {
                rnd_val = rnd_val * 1103515245 + 12345;

                dest[4 * x + 3] = (rnd_val >> 16);
            }
        }
    }

    current_wipe_texture = UploadTexture(&img);
}
