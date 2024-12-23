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

extern SkyStretch current_sky_stretch;

void SetupSkyMatrices(void)
{
    if (custom_skybox)
    {
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();

        glFrustum(view_x_slope * renderer_near_clip.f_, -view_x_slope * renderer_near_clip.f_,
                    -view_y_slope * renderer_near_clip.f_, view_y_slope * renderer_near_clip.f_,
                    renderer_near_clip.f_, renderer_far_clip.f_);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        glRotatef(270.0f - epi::DegreesFromBAM(view_vertical_angle), 1.0f, 0.0f, 0.0f);
        glRotatef(epi::DegreesFromBAM(view_angle), 0.0f, 0.0f, 1.0f);
    }
    else
    {
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();

        glFrustum(-view_x_slope * renderer_near_clip.f_, view_x_slope * renderer_near_clip.f_,
                -view_y_slope * renderer_near_clip.f_, view_y_slope * renderer_near_clip.f_, renderer_near_clip.f_,
                renderer_far_clip.f_ * 4.0);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        glRotatef(270.0f - epi::DegreesFromBAM(view_vertical_angle), 1.0f, 0.0f, 0.0f);
        glRotatef(-epi::DegreesFromBAM(view_angle), 0.0f, 0.0f, 1.0f);

        if (current_sky_stretch == kSkyStretchStretch)
            glTranslatef(0.0f, 0.0f,
                         (renderer_far_clip.f_ * 2 * 0.15));  // Draw center above horizon a little
        else
            glTranslatef(0.0f, 0.0f,
                         -(renderer_far_clip.f_ * 2 * 0.15)); // Draw center below horizon a little
    }
}

 void RendererRevertSkyMatrices(void)
{
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

GLuint UploadTexture(ImageData *img, int flags, int max_pix)
{
    /* Send the texture data to the GL, and returns the texture ID
     * assigned to it.
     */

    EPI_ASSERT(img->depth_ == 3 || img->depth_ == 4);

    bool clamp  = (flags & kUploadClamp) ? true : false;
    bool nomip  = (flags & kUploadMipMap) ? false : true;
    bool smooth = (flags & kUploadSmooth) ? true : false;

    int total_w = img->width_;
    int total_h = img->height_;

    int new_w, new_h;

    // scale down, if necessary, to fix the maximum size
    for (new_w = total_w; new_w > maximum_texture_size; new_w /= 2)
    { /* nothing here */
    }

    for (new_h = total_h; new_h > maximum_texture_size; new_h /= 2)
    { /* nothing here */
    }

    while (new_w * new_h > max_pix)
    {
        if (new_h >= new_w)
            new_h /= 2;
        else
            new_w /= 2;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    GLuint id;

    glGenTextures(1, &id);
    global_render_state->BindTexture(id);

    GLint tmode = GL_REPEAT;

    if (clamp)
        tmode = renderer_dumb_clamp.d_ ? GL_CLAMP : GL_CLAMP_TO_EDGE;

    global_render_state->TextureWrapS(tmode);
    global_render_state->TextureWrapT(tmode);

    texture_clamp_s.emplace(id, tmode);
    texture_clamp_t.emplace(id, tmode);

    // magnification mode
    global_render_state->TextureMagFilter(smooth ? GL_LINEAR : GL_NEAREST);

    // minification mode
    int mip_level = HMM_Clamp(0, image_mipmapping, 2);

    // special logic for mid-masked textures.  The kUploadThresh flag
    // guarantees that each texture level has simple alpha (0 or 255),
    // but we must also disable Trilinear Mipmapping because it will
    // produce partial alpha values when interpolating between mips.
    if (flags & kUploadThresh)
        mip_level = HMM_Clamp(0, mip_level, 1);

    static GLuint minif_modes[2 * 3] = {GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR,

                                        GL_LINEAR,  GL_LINEAR_MIPMAP_NEAREST,  GL_LINEAR_MIPMAP_LINEAR};

    global_render_state->TextureMinFilter(minif_modes[(smooth ? 3 : 0) + (nomip ? 0 : mip_level)]);

    for (int mip = 0;; mip++)
    {
        if (img->width_ != new_w || img->height_ != new_h)
        {
            img->ShrinkMasked(new_w, new_h);

            if (flags & kUploadThresh)
                img->ThresholdAlpha((mip & 1) ? 96 : 144);
        }

        glTexImage2D(GL_TEXTURE_2D, mip, (img->depth_ == 3) ? GL_RGB : GL_RGBA, new_w, new_h, 0 /* border */,
                     (img->depth_ == 3) ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, img->PixelAt(0, 0));

        // stop if mipmapping disabled or we have reached the end
        if (nomip || !image_mipmapping || (new_w == 1 && new_h == 1))
            break;

        new_w = HMM_MAX(1, new_w / 2);
        new_h = HMM_MAX(1, new_h / 2);
    }

    return id;
}
