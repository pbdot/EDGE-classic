

#include <unordered_map>
#include "gfx.h"
#include "sokol_gfx.h"
#include "HandmadeMath.h"

#include "i_defs.h"
#include "i_defs_gl.h"
#include "image_data.h"

#include "r_gldefs.h"
#include "r_texgl.h"

/*
static struct
{
    float          rx, ry;
    sg_pipeline    pip;
    sg_bindings    bind;
    sg_pass_action pass_action;
} state;
*/

void GFX_Setup()
{

    /*
        state.rx = state.ry = 0.0f;

        // a vertex buffer with 3 vertices
        float vertices[] = {// positions            // colors
                            0.0f, 0.5f, 0.5f, 1.0f,  0.0f,  0.0f, 1.0f, 0.5f, -0.5f, 0.5f, 0.0f,
                            1.0f, 0.0f, 1.0f, -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,  1.0f};

        sg_buffer_desc buffer_desc = {0};
        buffer_desc.data           = SG_RANGE(vertices);
        buffer_desc.label          = "triangle-vertices";

        state.bind.vertex_buffers[0] = sg_make_buffer(&buffer_desc);

        // create shader from code-generated sg_shader_desc
        sg_shader shd = sg_make_shader(triangle_shader_desc(sg_query_backend()));

        // create a pipeline object (default render states are fine for triangle)

        sg_pipeline_desc pipeline_desc                      = {0};
        pipeline_desc.shader                                = shd;
        pipeline_desc.layout.attrs[ATTR_vs_position].format = SG_VERTEXFORMAT_FLOAT3;
        pipeline_desc.layout.attrs[ATTR_vs_color0].format   = SG_VERTEXFORMAT_FLOAT4;
        pipeline_desc.label                                 = "triangle-pipeline";

        state.pip = sg_make_pipeline(&pipeline_desc);
    */
}

static std::unordered_map<uint32_t, sg_sampler> sampler_lookup;
static std::unordered_map<uint32_t, sg_image>   image_lookup;

std::unordered_map<uint32_t, gfx_image_t> gfx_image_lookup;

GLuint GFX_UploadTexture(epi::image_data_c *img, int flags, int max_pix)
{
    // tex_id the R_UploadTexture function so we can match it while prototyping

    // OPTIMIZE this and the memory copies below!
    if (img->bpp == 3)
    {
        img->SetAlpha(255);
    }

    SYS_ASSERT(img->bpp == 4);

    bool clamp  = (flags & UPL_Clamp) ? true : false;
    bool nomip  = (flags & UPL_MipMap) ? false : true;
    bool smooth = (flags & UPL_Smooth) ? true : false;
    bool thresh = (flags & UPL_Thresh) ? true : false;

    int total_w = img->width;
    int total_h = img->height;

    int new_w, new_h;

    // scale down, if necessary, to fix the maximum size
    for (new_w = total_w; new_w > glmax_tex_size; new_w /= 2)
    { /* nothing here */
    }

    for (new_h = total_h; new_h > glmax_tex_size; new_h /= 2)
    { /* nothing here */
    }

    while (new_w * new_h > max_pix)
    {
        if (new_h >= new_w)
            new_h /= 2;
        else
            new_w /= 2;
    }

    sg_sampler sampler = {0};

    auto value = sampler_lookup.find(flags);
    if (value != sampler_lookup.end())
    {
        sampler = value->second;
    }
    else
    {
        sg_sampler_desc desc = {0};

        // wrapping
        sg_wrap wrap = SG_WRAP_REPEAT;
        if (clamp)
        {
            wrap = SG_WRAP_CLAMP_TO_EDGE;
        }

        desc.wrap_u = wrap;
        desc.wrap_v = wrap;

        // filtering
        desc.mag_filter    = smooth ? SG_FILTER_LINEAR : SG_FILTER_NEAREST;
        desc.min_filter    = smooth ? SG_FILTER_LINEAR : SG_FILTER_NEAREST;
        desc.mipmap_filter = smooth ? SG_FILTER_LINEAR : SG_FILTER_NEAREST;

        sampler = sg_make_sampler(&desc);
        sampler_lookup[flags] = sampler;
    }

    // minification mode
    int mip_level = CLAMP(0, detail_level, 2);

    // special logic for mid-masked textures.  The UPL_Thresh flag
    // guarantees that each texture level has simple alpha (0 or 255),
    // but we must also disable Trilinear Mipmapping because it will
    // produce partial alpha values when interpolating between mips.
    if (thresh)
        mip_level = CLAMP(0, mip_level, 1);

    sg_image_data img_data = {0};

    std::vector<void *> mip_data;

    sg_image_desc img_desc = {0};
    img_desc.width         = new_w;
    img_desc.height        = new_h;

    img_desc.pixel_format = SG_PIXELFORMAT_RGBA8;

    for (int mip = 0;; mip++)
    {
        if (img->width != new_w || img->height != new_h)
        {
            img->ShrinkMasked(new_w, new_h);

            if (flags & UPL_Thresh)
                img->ThresholdAlpha((mip & 1) ? 96 : 144);
        }

        size_t   sz = new_w * new_h * 4;
        sg_range range;
        range.ptr  = malloc(sz);
        range.size = sz;
        memcpy((void *)range.ptr, (void *)img->PixelAt(0, 0), sz);
        img_data.subimage[0][mip] = range;

        mip_data.push_back((void *)range.ptr);

        // stop if mipmapping disabled or we have reached the end
        if (nomip || !detail_level || (new_w == 1 && new_h == 1))
            break;

        new_w = MAX(1, new_w / 2);
        new_h = MAX(1, new_h / 2);
    }

    //    glTexImage2D(GL_TEXTURE_2D, mip, (img->bpp == 3) ? GL_RGB : GL_RGBA, new_w, new_h, 0 /* border */,
    //             (img->bpp == 3) ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, img->PixelAt(0, 0));

    img_desc.num_mipmaps = (int)mip_data.size();
    img_desc.data        = img_data;

    sg_image image = sg_make_image(&img_desc);

    sg_gl_image_info info = sg_gl_query_image_info(image);

    for (void *mipmap : mip_data)
    {
        free(mipmap);
    }

    // Legacy rendering
    glBindTexture(GL_TEXTURE_2D, info.tex[0]);
    GLint tmode = GL_REPEAT;

    if (clamp)
        tmode = r_dumbclamp.d ? GL_CLAMP : GL_CLAMP_TO_EDGE;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, tmode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tmode);

    // magnification mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, smooth ? GL_LINEAR : GL_NEAREST);

    static GLuint minif_modes[2 * 3] = {GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR,

                                        GL_LINEAR,  GL_LINEAR_MIPMAP_NEAREST,  GL_LINEAR_MIPMAP_LINEAR};

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minif_modes[(smooth ? 3 : 0) + (nomip ? 0 : mip_level)]);

    gfx_image_t gimage = {image.id, sampler.id};

    gfx_image_lookup[info.tex[0]] = gimage;

    return info.tex[0];
}

/*
#pragma pack(push, 1)
SOKOL_SHDC_ALIGN(16) typedef struct vs_params_t
{
    HMM_Mat4 mvp;
} vs_params_t;
#pragma pack(pop)
*/

extern angle_t viewangle;
extern angle_t viewvertangle;
extern float   viewx;
extern float   viewy;
extern float   viewz;
