
#include "smc_sys_macro.h"
#include "smc_sys_type.h"
#include "smc_im_color.h"

namespace smc
{

// config items
bool swap_sidedefs           = false;
bool show_full_one_sided     = false;
bool sidedef_add_del_buttons = false;

bool browser_small_tex   = false;
bool browser_combine_tex = false;

int floor_bump_small  = 1;
int floor_bump_medium = 8;
int floor_bump_large  = 64;

int light_bump_small  = 4;
int light_bump_medium = 16;
int light_bump_large  = 64;

rgb_color_t dotty_axis_col  = RGB_MAKE(0, 128, 255);
rgb_color_t dotty_major_col = RGB_MAKE(0, 0, 238);
rgb_color_t dotty_minor_col = RGB_MAKE(0, 0, 187);
rgb_color_t dotty_point_col = RGB_MAKE(0, 0, 255);

rgb_color_t normal_axis_col  = RGB_MAKE(0, 128, 255);
rgb_color_t normal_main_col  = RGB_MAKE(0, 0, 238);
rgb_color_t normal_flat_col  = RGB_MAKE(60, 60, 120);
rgb_color_t normal_small_col = RGB_MAKE(60, 60, 120);

typedef enum
{
    LINFO_Nothing = 0,
    LINFO_Length,
    LINFO_Angle,
    LINFO_Ratio,
    LINFO_Length_Angle,
    LINFO_Length_Ratio

} line_info_mode_e;

int highlight_line_info = (int)LINFO_Length;

//
//  the apparent radius of a vertex, in pixels
//
int vertex_radius(double scale)
{
    int r = 6 * (0.26 + scale / 2);

    if (r > 12)
        r = 12;

    return r;
}

} // namespace smc