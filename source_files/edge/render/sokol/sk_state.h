//----------------------------------------------------------------------------
//  EDGE Refresh internal state variables
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2024 The EDGE Team.
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//----------------------------------------------------------------------------
//
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#pragma once

#include "../r_state.h"
#include "sk_local.h"

#define SK_MAX_SAMPLERS 256

enum PipelineFlags
{
    kPipelineDepthWrite   = 1,
    kPipelineDepthGreater = 2,
    kPipelineAdditive     = 4,
    kPipelineAlpha        = 8
};

class SokolRenderState : public RenderState
{
  public:
    void Enable(GLenum cap, bool enabled = true)
    {
        switch (cap)
        {
        case GL_TEXTURE_2D:
            if (enable_texture_2d_[active_texture_ - GL_TEXTURE0] == enabled)
                return;
            enable_texture_2d_[active_texture_ - GL_TEXTURE0] = enabled;
            break;
        case GL_FOG:
            if (enable_fog_ == enabled)
                return;
            enable_fog_ = enabled;
            break;
        case GL_ALPHA_TEST:
            if (enable_alpha_test_ == enabled)
                return;
            enable_alpha_test_ = enabled;
            break;
        case GL_BLEND:
            if (enable_blend_ == enabled)
                return;
            enable_blend_ = enabled;
            break;
        case GL_CULL_FACE:
            if (enable_cull_face_ == enabled)
                return;
            enable_cull_face_ = enabled;
            break;
        case GL_SCISSOR_TEST:
            if (enable_scissor_test_ == enabled)
                return;
            enable_scissor_test_ = enabled;
            break;
        case GL_LIGHTING:
            if (enable_lighting_ == enabled)
                return;
            enable_lighting_ = enabled;
            break;
        case GL_COLOR_MATERIAL:
            if (enable_color_material_ == enabled)
                return;
            enable_color_material_ = enabled;
            break;
        case GL_DEPTH_TEST:
            if (enable_depth_test_ == enabled)
                return;
            enable_depth_test_ = enabled;
            break;
        case GL_STENCIL_TEST:
            if (enable_stencil_test_ == enabled)
                return;
            enable_stencil_test_ = enabled;
            break;
        case GL_LINE_SMOOTH:
            if (enable_line_smooth_ == enabled)
                return;
            enable_line_smooth_ = enabled;
            break;
        case GL_NORMALIZE:
            if (enable_normalize_ == enabled)
                return;
            enable_normalize_ = enabled;
            break;

        case GL_CLIP_PLANE0:
        case GL_CLIP_PLANE1:
        case GL_CLIP_PLANE2:
        case GL_CLIP_PLANE3:
        case GL_CLIP_PLANE4:
        case GL_CLIP_PLANE5:
            break;
            /*
            if (enable_clip_plane_[cap - GL_CLIP_PLANE0] == enabled)
                return;
            enable_clip_plane_[cap - GL_CLIP_PLANE0] = enabled;
            break;
            */

#ifndef EDGE_GL_ES2
        case GL_POLYGON_SMOOTH:
            if (enable_polygon_smooth_ == enabled)
                return;
            enable_polygon_smooth_ = enabled;
            break;
#endif
        default:
            FatalError("Unknown GL State %i", cap);
        }

        if (enabled)
        {
            // no multitexture
            if (cap == GL_TEXTURE_2D)
            {
                if ((active_texture_ - GL_TEXTURE0) == 0)
                {
                    // sgl_enable_texture();
                }
            }

            if (cap >= GL_CLIP_PLANE0 && cap <= GL_CLIP_PLANE5)
            {
                sgl_set_clipplane_enabled(cap - GL_CLIP_PLANE0, true);
            }

            // glEnable(cap);
        }
        else
        {
            if (cap == GL_TEXTURE_2D)
            {
                // no multitexture
                if ((active_texture_ - GL_TEXTURE0) == 0)
                {
                    // sgl_disable_texture();
                }
            }

            if (cap >= GL_CLIP_PLANE0 && cap <= GL_CLIP_PLANE5)
            {
                sgl_set_clipplane_enabled(cap - GL_CLIP_PLANE0, false);
            }

            // glDisable(cap);
        }

        ec_frame_stats.draw_state_change++;
    }

    void Disable(GLenum cap)
    {
        Enable(cap, false);
    }

    void DepthMask(bool enable)
    {
        if (depth_mask_ == enable)
        {
            return;
        }

        depth_mask_ = enable;
        // glDepthMask(enable ? GL_TRUE : GL_FALSE);
        ec_frame_stats.draw_state_change++;
    }

    void DepthFunction(GLenum func)
    {
        if (func == depth_function_)
        {
            return;
        }

        depth_function_ = func;

        // glDepthFunc(depth_function_);
        ec_frame_stats.draw_state_change++;
    }

    void CullFace(GLenum mode)
    {
        if (cull_face_ == mode)
        {
            return;
        }

        cull_face_ = mode;
        // glCullFace(mode);
        ec_frame_stats.draw_state_change++;
    }

    void AlphaFunction(GLenum func, GLfloat ref)
    {
        if (func == alpha_function_ && AlmostEquals(ref, alpha_function_reference_))
        {
            return;
        }

        alpha_function_           = func;
        alpha_function_reference_ = ref;

        // glAlphaFunc(alpha_function_, alpha_function_reference_);
        ec_frame_stats.draw_state_change++;
    }

    void ActiveTexture(GLenum activeTexture)
    {
        if (activeTexture == active_texture_)
        {
            return;
        }

        active_texture_ = activeTexture;
        // glActiveTexture(active_texture_);
        ec_frame_stats.draw_state_change++;
    }

    void BindTexture(GLuint textureid)
    {
        GLuint index = active_texture_ - GL_TEXTURE0;
        if (bind_texture_2d_[index] == textureid)
        {
            return;
        }

        bind_texture_2d_[index] = textureid;

        // no multitexture
        if (index != 0)
        {
            return;
        }

        // glBindTexture(GL_TEXTURE_2D, textureid);
        sg_image img;
        img.id = textureid;
        // sgl_texture(img, default_sampler);

        ec_frame_stats.draw_texture_change++;
        ec_frame_stats.draw_state_change++;
    }

    void PolygonOffset(GLfloat factor, GLfloat units)
    {
        if (factor == polygon_offset_factor_ && units == polygon_offset_units_)
        {
            return;
        }

        polygon_offset_factor_ = factor;
        polygon_offset_units_  = units;
        // glPolygonOffset(polygon_offset_factor_, polygon_offset_units_);
        ec_frame_stats.draw_state_change++;
    }

    void ClearColor(RGBAColor color)
    {
        if (color == clear_color_)
        {
            return;
        }

        clear_color_ = color;
        // glClearColor(epi::GetRGBARed(clear_color_) / 255.0f, epi::GetRGBAGreen(clear_color_) / 255.0f,
        //              epi::GetRGBABlue(clear_color_) / 255.0f, epi::GetRGBAAlpha(clear_color_) / 255.0f);
        ec_frame_stats.draw_state_change++;
    }

    void FogMode(GLint fogMode)
    {
        if (fog_mode_ == fogMode)
        {
            return;
        }

        fog_mode_ = fogMode;
        // glFogi(GL_FOG_MODE, fog_mode_);
        ec_frame_stats.draw_state_change++;
    }

    void FogColor(RGBAColor color)
    {
        if (fog_color_ == color)
        {
            return;
        }

        fog_color_ = color;

        float gl_fc[4] = {epi::GetRGBARed(fog_color_) / 255.0f, epi::GetRGBAGreen(fog_color_) / 255.0f,
                          epi::GetRGBABlue(fog_color_) / 255.0f, epi::GetRGBAAlpha(fog_color_) / 255.0f};
        // glFogfv(GL_FOG_COLOR, gl_fc);
        ec_frame_stats.draw_state_change++;
    }

    void FogStart(GLfloat start)
    {
        if (fog_start_ == start)
        {
            return;
        }

        fog_start_ = start;
        // glFogf(GL_FOG_START, fog_start_);
        ec_frame_stats.draw_state_change++;
    }

    void FogEnd(GLfloat end)
    {
        if (fog_end_ == end)
        {
            return;
        }

        fog_end_ = end;
        // glFogf(GL_FOG_END, fog_end_);
        ec_frame_stats.draw_state_change++;
    }

    void FogDensity(GLfloat density)
    {
        if (fog_density_ == density)
        {
            return;
        }

        fog_density_ = density;
        // glFogf(GL_FOG_DENSITY, fog_density_);
        ec_frame_stats.draw_state_change++;
    }

    void GLColor(RGBAColor color)
    {
        if (color == gl_color_)
        {
            return;
        }

        gl_color_ = color;
        // glColor4ub(epi::GetRGBARed(color), epi::GetRGBAGreen(color), epi::GetRGBABlue(color),
        // epi::GetRGBAAlpha(color));
        ec_frame_stats.draw_state_change++;
    }

    void BlendFunction(GLenum sfactor, GLenum dfactor)
    {
        if (blend_source_factor_ == sfactor && blend_destination_factor_ == dfactor)
        {
            return;
        }

        blend_source_factor_      = sfactor;
        blend_destination_factor_ = dfactor;
        // glBlendFunc(blend_source_factor_, blend_destination_factor_);
        ec_frame_stats.draw_state_change++;
    }

    void TextureEnvironmentMode(GLint param)
    {
        GLuint index = active_texture_ - GL_TEXTURE0;

        if (texture_environment_mode_[index] == param)
        {
            return;
        }

        texture_environment_mode_[index] = param;
        // glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, texture_environment_mode_[index]);
        ec_frame_stats.draw_state_change++;
    }

    void TextureEnvironmentCombineRGB(GLint param)
    {
        GLuint index = active_texture_ - GL_TEXTURE0;

        if (texture_environment_combine_rgb_[index] == param)
        {
            return;
        }

        texture_environment_combine_rgb_[index] = param;
        // glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, texture_environment_combine_rgb_[index]);
        ec_frame_stats.draw_state_change++;
    }

    void TextureEnvironmentSource0RGB(GLint param)
    {
        GLuint index = active_texture_ - GL_TEXTURE0;

        if (texture_environment_source_0_rgb_[index] == param)
        {
            return;
        }

        texture_environment_source_0_rgb_[index] = param;
        // glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, texture_environment_source_0_rgb_[index]);
        ec_frame_stats.draw_state_change++;
    }

    void TextureMinFilter(GLint param)
    {
        GLuint index = active_texture_ - GL_TEXTURE0;

        texture_min_filter_[index] = param;
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texture_min_filter_[index]);
        ec_frame_stats.draw_state_change++;
    }

    void TextureMagFilter(GLint param)
    {
        GLuint index = active_texture_ - GL_TEXTURE0;

        texture_mag_filter_[index] = param;
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texture_mag_filter_[index]);
        ec_frame_stats.draw_state_change++;
    }

    void TextureWrapS(GLint param)
    {
        GLuint index = active_texture_ - GL_TEXTURE0;

        // We do it regardless of the cached value; functions should check
        // texture environments against the appropriate unordered_map and
        // know if a change needs to occur
        texture_wrap_s_[index] = param;
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texture_wrap_s_[index]);
        ec_frame_stats.draw_state_change++;
    }

    void TextureWrapT(GLint param)
    {
        GLuint index = active_texture_ - GL_TEXTURE0;

        // We do it regardless of the cached value; functions should check
        // texture environments against the appropriate unordered_map and
        // know if a change needs to occur
        texture_wrap_t_[index] = param;
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texture_wrap_t_[index]);
        ec_frame_stats.draw_state_change++;
    }

    void MultiTexCoord(GLuint tex, const HMM_Vec2 *coords)
    {
        if (enable_texture_2d_[tex - GL_TEXTURE0] == false)
            return;
        /*
        if (tex == GL_TEXTURE0 && enable_texture_2d_[1] == false)
            glTexCoord2fv((GLfloat *)coords);
        else
            glMultiTexCoord2fv(tex, (GLfloat *)coords);
        */
        ec_frame_stats.draw_state_change++;
    }

    void Hint(GLenum target, GLenum mode)
    {
        // glHint(target, mode);
        ec_frame_stats.draw_state_change++;
    }

    void LineWidth(float width)
    {
        if (AlmostEquals(width, line_width_))
        {
            return;
        }
        line_width_ = width;
        // glLineWidth(line_width_);
        ec_frame_stats.draw_state_change++;
    }

    void DeleteTexture(const GLuint *tex_id)
    {
        if (tex_id && *tex_id > 0)
        {
            texture_clamp_s.erase(*tex_id);
            texture_clamp_t.erase(*tex_id);
            // glDeleteTextures(1, tex_id);
            //  We don't need to actually perform a texture bind,
            //  but these should be cleared out to ensure
            //  we aren't mistakenly using a tex_id that does not
            //  correlate to the same texture anymore
            bind_texture_2d_[0] = 0;
            bind_texture_2d_[1] = 0;
        }
    }

    void FrontFace(GLenum wind)
    {
        if (front_face_ == wind)
        {
            return;
        }

        front_face_ = wind;
        // glFrontFace(wind);
        ec_frame_stats.draw_state_change++;
    }

    void ShadeModel(GLenum model)
    {
        if (shade_model_ == model)
        {
            return;
        }

        shade_model_ = model;
        // glShadeModel(model);
        ec_frame_stats.draw_state_change++;
    }

    void ColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a)
    {
        // glColorMask(r, g, b, a);
    }

    void Clear(GLbitfield mask)
    {
        // glClear(mask);
    }

    void Flush()
    {
        // glFlush();
    }

    void ReadScreen(int x, int y, int w, int h, uint8_t *rgb_buffer)
    {
        /*
        glPixelZoom(1.0f, 1.0f);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        for (; h > 0; h--, y++)
        {
            glReadPixels(x, y, w, 1, GL_RGB, GL_UNSIGNED_BYTE, rgb_buffer);

            rgb_buffer += w * 3;
        }
        */
    }

    void ClipPlane(GLenum plane, const GLdouble *equation)
    {
        sgl_set_clipplane(int(plane) - int(GL_CLIP_PLANE0), float(equation[0]), float(equation[1]), float(equation[2]),
                          float(equation[3]));
    }

    void Scissor(GLint x, GLint y, GLsizei width, GLsizei height)
    {
        // can't currently disable
        sgl_scissor_rect(x, y, width, height, true);
    }

    void GenTextures(GLsizei n, GLuint *textures)
    {
        // glGenTextures(n, textures);
        *textures = 0;
    }

    void TexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border,
                    GLenum format, GLenum type, const void *data)
    {
        // glTexImage2D(target, level, internalformat, width, height, border, format, type, data);
    }

    void Initialize();

    void StartFrame(void);

    void SwapBuffers(void);

    void FinishFrame(void);

    void SetRenderMode(RenderMode mode)
    {
        if (mode == kRenderMode2D)
        {
            sgl_set_context(context_2d_);
        }
        else if (mode == kRenderMode3D)
        {
            sgl_set_context(context_3d_);
        }
        else
        {
            FatalError("Unknown Render Mode");
        }
    }

    sgl_pipeline GetPipeline(uint32_t pipeline_flags)
    {
        std::unordered_map<uint32_t, uint32_t>::const_iterator pipeline_itr = pipelines_.find(pipeline_flags);
        uint32_t                                               pipeline_id  = 0xFFFFFFFF;
        if (pipeline_itr == pipelines_.end())
        {
            sg_pipeline_desc pipeline_desc = {0};
            if (pipeline_flags & kPipelineDepthGreater)
                pipeline_desc.depth.compare = SG_COMPAREFUNC_GREATER;
            else
                pipeline_desc.depth.compare = SG_COMPAREFUNC_LESS_EQUAL;

            if (pipeline_flags & kPipelineDepthWrite)
                pipeline_desc.depth.write_enabled = true;

            if (pipeline_flags & kPipelineAlpha)
            {
                pipeline_desc.colors[0].blend.enabled        = true;
                pipeline_desc.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
                pipeline_desc.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            }
            if (pipeline_flags & kPipelineAdditive)
            {
                pipeline_desc.colors[0].blend.enabled        = true;
                pipeline_desc.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
                pipeline_desc.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE;
            }

            pipeline_id = sgl_context_make_pipeline(context_3d_, &pipeline_desc).id;

            pipelines_[pipeline_flags] = pipeline_id;
        }
        else
        {
            pipeline_id = pipeline_itr->second;
        }

        sgl_pipeline pipeline = {pipeline_id};
        return pipeline;
    }

    void RegisterImage(uint32_t imageId, sg_sampler_desc *desc);

    sg_sampler default_sampler;

    sg_sampler                             samplers_[SK_MAX_SAMPLERS];
    sg_sampler_desc                        sampler_descs_[SK_MAX_SAMPLERS];
    std::unordered_map<uint32_t, uint32_t> image_samplers_;

    // flags => pipeline
    std::unordered_map<uint32_t, uint32_t> pipelines_;

    int num_samplers_;

    int frameStateChanges_ = 0;

    bool   enable_blend_;
    GLenum blend_source_factor_;
    GLenum blend_destination_factor_;

    bool   enable_cull_face_;
    GLenum cull_face_;

    GLenum front_face_;

    GLenum shade_model_;

    bool enable_scissor_test_;

    bool enable_clip_plane_[6];

    RGBAColor clear_color_;

    // texture
    bool enable_texture_2d_[2];

    GLint texture_environment_mode_[2];
    GLint texture_environment_combine_rgb_[2];
    GLint texture_environment_source_0_rgb_[2];
    GLint texture_min_filter_[2];
    GLint texture_mag_filter_[2];
    GLint texture_wrap_s_[2];
    GLint texture_wrap_t_[2];

    GLuint bind_texture_2d_[2];
    GLenum active_texture_ = GL_TEXTURE0;

    bool   enable_depth_test_;
    bool   depth_mask_;
    GLenum depth_function_;

    GLfloat polygon_offset_factor_;
    GLfloat polygon_offset_units_;

    bool    enable_alpha_test_;
    GLenum  alpha_function_;
    GLfloat alpha_function_reference_;

    bool enable_lighting_;

    bool enable_color_material_;

    bool enable_stencil_test_;

    bool  enable_line_smooth_;
    float line_width_;

    bool enable_normalize_;

#ifndef EDGE_GL_ES2
    bool enable_polygon_smooth_;
#endif

    bool      enable_fog_;
    GLint     fog_mode_;
    GLfloat   fog_start_;
    GLfloat   fog_end_;
    GLfloat   fog_density_;
    RGBAColor fog_color_;

    RGBAColor gl_color_;

    // 2D
    sgl_context context_2d_;

    // 3D
    sgl_context context_3d_;

    // imgui
    sgimgui_t sgimgui_;
};

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
