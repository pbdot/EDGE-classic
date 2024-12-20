#include "hu_draw.h"

#include "am_map.h"
#include "con_main.h"
#include "ddf_font.h"
#include "epi.h"
#include "epi_str_compare.h"
#include "g_game.h"
#include "i_defs_gl.h"
#include "r_colormap.h"
#include "r_gldefs.h"
#include "r_image.h"
#include "r_misc.h"
#include "r_misc.h" //  R_Render
#include "r_modes.h"
#include "r_units.h"

extern ConsoleVariable fliplevels;

void HUDRenderAutomap(float x, float y, float w, float h, MapObject *player, int flags)
{
    HUDPushScissor(x, y, x + w, y + h, (flags & 1) == 0);

    // [ FIXME HACKY ]
    if ((flags & 1) == 0)
    {
        if (x < 1 && x + w > hud_x_middle * 2 - 1)
        {
            x = hud_x_left;
            w = hud_x_right - x;
        }
    }

    if (fliplevels.d_)
    {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho((float)current_screen_width, 0.0f, 0.0f, (float)current_screen_height, -1.0f, 1.0f);
    }

    AutomapRender(x, y, w, h, player);

    if (fliplevels.d_)
    {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0f, (float)current_screen_width, 0.0f, (float)current_screen_height, -1.0f, 1.0f);
    }

    HUDPopScissor();
}
