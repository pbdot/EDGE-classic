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
#include "r_render_port.h"
#include "r_sky.h"
#include "r_texgl.h"
#include "r_units.h"
#include "sk_local.h"
#include "stb_sprintf.h"
#include "w_flat.h"
#include "w_wad.h"
#include "sk_state.h"

GLuint UploadTexture(ImageData *img, int flags, int max_pix)
{

    SokolRenderState *fixme = (SokolRenderState *)global_render_state;

    /* Send the texture data to the GL, and returns the texture ID
     * assigned to it.
     */

    EPI_ASSERT(img->depth_ == 3 || img->depth_ == 4);

    // OPTIMIZE this and the memory copies below!
    if (img->depth_ == 3)
    {
        img->SetAlpha(255);
    }

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

    // glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // GLuint id;

    // glGenTextures(1, &id);
    // global_render_state->BindTexture(id);

    GLint tmode = GL_REPEAT;

    if (clamp)
        tmode = renderer_dumb_clamp.d_ ? GL_CLAMP : GL_CLAMP_TO_EDGE;

    global_render_state->TextureWrapS(tmode);
    global_render_state->TextureWrapT(tmode);

    // texture_clamp_s.emplace(id, tmode);
    // texture_clamp_t.emplace(id, tmode);

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

    sg_image_data img_data = {0};
    std::vector<void *> mip_data;

    sg_image_desc img_desc = {0};
    img_desc.width         = new_w;
    img_desc.height        = new_h;

    img_desc.pixel_format = SG_PIXELFORMAT_RGBA8;

    for (int mip = 0;; mip++)
    {
        if (img->width_ != new_w || img->height_ != new_h)
        {
            img->ShrinkMasked(new_w, new_h);

            if (flags & kUploadThresh)
                img->ThresholdAlpha((mip & 1) ? 96 : 144);
        }

        size_t   sz = new_w * new_h * 4;
        sg_range range;
        range.ptr  = malloc(sz);
        range.size = sz;
        memcpy((void *)range.ptr, (void *)img->PixelAt(0, 0), sz);
        img_data.subimage[0][mip] = range;

        mip_data.push_back((void *)range.ptr);

        // glTexImage2D(GL_TEXTURE_2D, mip, (img->depth_ == 3) ? GL_RGB : GL_RGBA, new_w, new_h, 0 /* border */,
        //              (img->depth_ == 3) ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, img->PixelAt(0, 0));

        // stop if mipmapping disabled or we have reached the end
        if (nomip || !image_mipmapping || (new_w == 1 && new_h == 1))
            break;

        new_w = HMM_MAX(1, new_w / 2);
        new_h = HMM_MAX(1, new_h / 2);
    }

    img_desc.num_mipmaps = (int)mip_data.size();
    img_desc.data        = img_data;

    sg_image image = sg_make_image(&img_desc);

    //sg_gl_image_info info = sg_gl_query_image_info(image);

    for (void *mipmap : mip_data)
    {
        free(mipmap);
    }    

    // Default sampler
    sg_sampler_desc sdesc = {0};

    sdesc.wrap_u = tmode ==  GL_REPEAT ? SG_WRAP_REPEAT : SG_WRAP_CLAMP_TO_EDGE;
    sdesc.wrap_v = tmode ==  GL_REPEAT ? SG_WRAP_REPEAT : SG_WRAP_CLAMP_TO_EDGE;

    // filtering
    sdesc.mag_filter    = SG_FILTER_NEAREST;
    sdesc.min_filter    = SG_FILTER_NEAREST;
    sdesc.mipmap_filter = SG_FILTER_NEAREST;

    fixme->RegisterImage(image.id, &sdesc);

    return image.id;
}