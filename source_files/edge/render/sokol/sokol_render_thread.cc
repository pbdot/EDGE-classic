
/*
SOKOL_API_IMPL void sgl_context_draw_layer(sgl_context ctx_id, int layer_id) {
    SOKOL_ASSERT(_SGL_INIT_COOKIE == _sgl.init_cookie);
    _sgl_context_t* ctx = _sgl_lookup_context(ctx_id.id);
    if (ctx) {
        _sgl_draw(ctx, layer_id);
    }
}

static void _sgl_draw(_sgl_context_t* ctx, int layer_id) {
    SOKOL_ASSERT(ctx);
    if ((ctx->vertices.next > 0) && (ctx->commands.next > 0)) {
        sg_push_debug_group("sokol-gl");

        uint32_t cur_pip_id = SG_INVALID_ID;
        uint32_t cur_img0_id = SG_INVALID_ID;
        uint32_t cur_smp0_id = SG_INVALID_ID;
        uint32_t cur_img1_id = SG_INVALID_ID;
        uint32_t cur_smp1_id = SG_INVALID_ID;

        int cur_vertex_uniform_index = -1;
        int cur_fragment_uniform_index = -1;

        if (ctx->update_frame_id != ctx->frame_id) {
            ctx->update_frame_id = ctx->frame_id;
            const sg_range range = { ctx->vertices.ptr, (size_t)ctx->vertices.next * sizeof(_sgl_vertex_t) };
            sg_update_buffer(ctx->vbuf, &range);
        }

        // render all successfully recorded commands (this may be less than the
        // issued commands if we're in an error state)
        for (int i = 0; i < ctx->commands.next; i++) {
            const _sgl_command_t* cmd = &ctx->commands.ptr[i];
            if (cmd->layer_id != layer_id) {
                continue;
            }
            switch (cmd->cmd) {
                case SGL_COMMAND_VIEWPORT:
                    {
                        const _sgl_viewport_args_t* args = &cmd->args.viewport;
                        sg_apply_viewport(args->x, args->y, args->w, args->h, args->origin_top_left);
                    }
                    break;
                case SGL_COMMAND_SCISSOR_RECT:
                    {
                        const _sgl_scissor_rect_args_t* args = &cmd->args.scissor_rect;
                        sg_apply_scissor_rect(args->x, args->y, args->w, args->h, args->origin_top_left);
                    }
                    break;
                case SGL_COMMAND_CLEAR_DEPTH:
                    {
                        const _sgl_clear_depth_args_t* args = &cmd->args.clear_depth;

#ifdef SOKOL_GLCORE
                        sg_gl_clear_depth(args->value);
#endif

#ifdef SOKOL_D3D11
                        sg_d3d11_clear_depth(args->value);
#endif
                    }
                    break;
                case SGL_COMMAND_DRAW:
                    {
                        const _sgl_draw_args_t* args = &cmd->args.draw;
                        if (args->pip.id != cur_pip_id) {
                            sg_apply_pipeline(args->pip);
                            cur_pip_id = args->pip.id;
                            // when pipeline changes, also need to re-apply uniforms and bindings
                            cur_img0_id = SG_INVALID_ID;
                            cur_smp0_id = SG_INVALID_ID;
                            cur_img1_id = SG_INVALID_ID;
                            cur_smp1_id = SG_INVALID_ID;
                            cur_vertex_uniform_index = -1;
                            cur_fragment_uniform_index = -1;
                        }
                        if ((cur_img0_id != args->img0.id) || (cur_smp0_id != args->smp0.id) ||
                            (cur_img1_id != args->img1.id) || (cur_smp1_id != args->smp1.id)) {
                            ctx->bind.images[0] = args->img0;
                            ctx->bind.samplers[0] = args->smp0;
                            ctx->bind.images[1] = args->img1;
                            ctx->bind.samplers[1] = args->smp1;
                            sg_apply_bindings(&ctx->bind);
                            cur_img0_id = args->img0.id;
                            cur_smp0_id = args->smp0.id;
                            cur_img1_id = args->img1.id;
                            cur_smp1_id = args->smp1.id;
                        }
                        if (cur_vertex_uniform_index != args->vertex_uniform_index) {
                            const sg_range ub_range = { &ctx->vertex_uniforms.ptr[args->vertex_uniform_index],
sizeof(_sgl_vertex_uniform_t) }; sg_apply_uniforms(0, &ub_range); cur_vertex_uniform_index = args->vertex_uniform_index;
                        }
                        if (cur_fragment_uniform_index != args->fragment_uniform_index) {
                            const sg_range ub_range = { &ctx->fragment_uniforms.ptr[args->fragment_uniform_index],
sizeof(_sgl_fragment_uniform_t) }; sg_apply_uniforms(1, &ub_range); cur_fragment_uniform_index =
args->fragment_uniform_index;
                        }
                        // FIXME: what if number of vertices doesn't match the primitive type?
                        if (args->num_vertices > 0) {
                            sg_draw(args->base_vertex, args->num_vertices, 1);
                        }
                    }
                    break;
            }
        }
        sg_pop_debug_group();
    }
}
*/