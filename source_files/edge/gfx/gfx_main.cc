

#include <unordered_map>
#include "gfx.h"
#include "sokol_gfx.h"
#include "HandmadeMath.h"

#include "i_defs.h"
#include "i_defs_gl.h"
#include "image_data.h"

#include "r_gldefs.h"
#include "r_texgl.h"
#include "SDL_video.h"

#include "shaders/screen.h"
#include "shaders/pp_ssao.h"

static struct
{
    sg_image    color_target;
    sg_sampler  color_sampler;
    sg_image    depth_target;
    sg_sampler  depth_sampler;
    sg_pass     world_pass;
    sg_buffer   quad_buffer;
    sg_pipeline screen_pipeline;

    // ssao
    sg_image    ssao_target;
    sg_pipeline ssao_pipeline;
    sg_pass     ssao_pass;

} state;

extern SDL_Window *my_vis;

static float quad_vertices_uvs[] = {-1.0f, -1.0f, 0.0f, 0, 0, 1.0f, -1.0f, 0.0f, 1, 0, 1.0f,  1.0f, 0.0f, 1, 1,
                                    -1.0f, -1.0f, 0.0f, 0, 0, 1.0f, 1.0f,  0.0f, 1, 1, -1.0f, 1.0f, 0.0f, 0, 1};

sg_buffer sokol_buffer_quad(void)
{
    sg_buffer_desc desc = {0};
    desc.data           = SG_RANGE(quad_vertices_uvs);
    desc.usage          = SG_USAGE_IMMUTABLE;
    return sg_make_buffer(&desc);
}

sg_image sokol_target_depth(int32_t width, int32_t height, int32_t sample_count)
{
    sg_image_desc img_desc = {0};
    img_desc.render_target = true;
    img_desc.width         = width;
    img_desc.height        = height;
    img_desc.pixel_format  = SG_PIXELFORMAT_DEPTH;
    img_desc.sample_count  = sample_count;
    img_desc.label         = "Depth target";

    return sg_make_image(&img_desc);
}

sg_image sokol_target_color(int32_t width, int32_t height, int32_t sample_count)
{
    sg_image_desc img_desc = {0};
    img_desc.render_target = true;
    img_desc.width         = width;
    img_desc.height        = height;
    img_desc.pixel_format  = SG_PIXELFORMAT_RGBA8;
    img_desc.sample_count  = sample_count;
    img_desc.label         = "Color target";

    return sg_make_image(&img_desc);
}

sg_image sokol_target_ssao(int32_t width, int32_t height, int32_t sample_count)
{
    sg_image_desc img_desc = {0};
    img_desc.render_target = true;
    img_desc.width         = width;
    img_desc.height        = height;
    img_desc.pixel_format  = SG_PIXELFORMAT_RGBA8;
    img_desc.sample_count  = sample_count;
    img_desc.label         = "SSAO target";

    return sg_make_image(&img_desc);
}

void GFX_Setup()
{
    int w, h;
    SDL_GL_GetDrawableSize(my_vis, &w, &h);

    sg_sampler_desc smp_desc = {0};
    smp_desc.min_filter      = SG_FILTER_LINEAR;
    smp_desc.mag_filter      = SG_FILTER_LINEAR;

    state.color_target  = sokol_target_color(w, h, 1);
    state.color_sampler = sg_make_sampler(&smp_desc);

    state.depth_target  = sokol_target_depth(w, h, 1);
    state.depth_sampler = sg_make_sampler(&smp_desc);

    sg_pass_desc world_pass                   = {0};
    world_pass.color_attachments[0].image     = state.color_target;
    world_pass.depth_stencil_attachment.image = state.depth_target;
    state.world_pass                          = sg_make_pass(&world_pass);

    state.quad_buffer = sokol_buffer_quad();

    sg_pipeline_desc pip_desc       = {0};
    pip_desc.shader                 = sg_make_shader(screen_shader_desc(sg_query_backend()));
    pip_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
    pip_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2;
    state.screen_pipeline           = sg_make_pipeline(&pip_desc);

    // ssao
    pip_desc = {0};

    state.ssao_target = sokol_target_ssao(w, h, 1);

    pip_desc.shader                 = sg_make_shader(ssao_shader_desc(sg_query_backend()));
    pip_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
    pip_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2;
    state.ssao_pipeline             = sg_make_pipeline(&pip_desc);

    sg_pass_desc ssao_pass_desc               = {0};
    ssao_pass_desc.color_attachments[0].image = state.ssao_target;
    state.ssao_pass                           = sg_make_pass(&ssao_pass_desc);
}

void GFX_DrawWorld()
{

    void GFX_DrawUnits();

    sg_pass_action pass_action        = {0};
    pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    pass_action.colors[0].clear_value = {0.0f, 0.3f, 0.0f, 1.0f};
    pass_action.depth.load_action     = SG_LOADACTION_CLEAR;
    pass_action.depth.clear_value     = 1.0f;

    sg_begin_pass(state.world_pass, pass_action);
    GFX_DrawUnits();
    sg_end_pass();
}

extern HMM_Mat4 frame_projection;

#pragma pack(push, 1)
SOKOL_SHDC_ALIGN(16) struct my_ssao_params_t
{
    float    u_near;
    float    u_far;
    float    u_target_size[2];
    HMM_Mat4 u_mat_p;
    HMM_Mat4 u_inv_mat_p;
};
#pragma pack(pop)

extern cvar_c r_nearclip;
extern cvar_c r_farclip;

void GFX_DrawPostProcess()
{
    int w, h;
    SDL_GL_GetDrawableSize(my_vis, &w, &h);

    my_ssao_params_t params = {0};

    params.u_mat_p     = frame_projection;
    params.u_inv_mat_p = HMM_InvPerspective_RH(frame_projection); // HMM_InvGeneralM4(frame_projection); <-- not sure if
                                                             // messes  up glFrustum derived matrix
    params.u_near           = r_nearclip.f;
    params.u_far            = r_farclip.f;
    params.u_target_size[0] = w;
    params.u_target_size[1] = h;

    sg_pass_action pass_action        = {0};
    pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    pass_action.colors[0].clear_value = {0.0f, 0.0f, 0.3f, 1.0f};
    pass_action.depth.load_action     = SG_LOADACTION_DONTCARE;

    sg_begin_pass(state.ssao_pass, pass_action);

    sg_apply_pipeline(state.ssao_pipeline);

    sg_range range = SG_RANGE(params);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_ssao_params, &range);

    sg_bindings bind = {0};

    bind.vertex_buffers[0] = state.quad_buffer;
    bind.fs.images[0]      = state.depth_target;
    bind.fs.samplers[0]    = state.depth_sampler;

    sg_apply_bindings(&bind);

    sg_draw(0, 6, 1);

    sg_end_pass();
}

void GFX_DrawWorldToScreen()
{
    sg_apply_pipeline(state.screen_pipeline);

    sg_bindings bind = {0};

    bind.vertex_buffers[0] = state.quad_buffer;
    bind.fs.images[0]      = state.color_target;
    bind.fs.samplers[0]    = state.color_sampler;

    sg_apply_bindings(&bind);

    sg_draw(0, 6, 1);
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

        sampler               = sg_make_sampler(&desc);
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

    gfx_image_t gimage = {image.id, sampler.id, 0};

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
