//------------------------------------------------------------------------
//  3D RENDERING : SOFTWARE MODE
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2019 Andrew Apted
//  Copyright (C) 1997-2003 Andr� Majorel et al
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//------------------------------------------------------------------------

#include "smc_main.h"

#include <map>
#include <algorithm>

#ifndef NO_OPENGL
#ifdef _FLTK_DISABLED
#include "FL/gl.h"
#endif
#endif

#include "smc_im_color.h"
#include "smc_im_img.h"
#include "smc_e_hover.h"
#include "smc_e_linedef.h"
#include "smc_e_main.h"
#include "smc_m_game.h"
#include "smc_w_rawdef.h"
#include "smc_w_texture.h"
#include "smc_r_render.h"
#include "smc_r_subdiv.h"

namespace smc
{

extern rgb_color_t transparent_col;

extern bool render_high_detail;
extern bool render_lock_gravity;
extern bool render_missing_bright;
extern bool render_unknown_bright;

static img_pixel_t DoomLightRemap(int light, float dist, img_pixel_t pixel)
{
    int map = R_DoomLightingEquation(light, dist);

    if (pixel & IS_RGB_PIXEL)
    {
        map = (map ^ 31) + 1;

        int r = IMG_PIXEL_RED(pixel);
        int g = IMG_PIXEL_GREEN(pixel);
        int b = IMG_PIXEL_BLUE(pixel);

        r = (r * map) >> 5;
        g = (g * map) >> 5;
        b = (b * map) >> 5;

        return IMG_PIXEL_MAKE_RGB(r, g, b);
    }
    else
    {
        return raw_colormap[map][pixel];
    }
}

struct DrawSurf
{
  public:
    enum
    {
        K_INVIS = 0,
        K_FLAT,
        K_TEXTURE
    };
    int kind;

    // heights for the surface (h1 is below h2)
    int h1, h2, tex_h;

    Img_c      *img;
    img_pixel_t col; /* used when no image */

    enum
    {
        SOLID_ABOVE = 1,
        SOLID_BELOW = 2
    };
    int y_clip;

    bool fullbright;

  public:
    DrawSurf() : kind(K_INVIS), h1(), h2(), tex_h(), img(NULL), col(), y_clip(), fullbright(false)
    {
    }

    ~DrawSurf()
    {
    }

    void FindFlat(const char *fname, Sector *sec)
    {
        fullbright = false;

        if (is_sky(fname))
        {
            col        = Misc_info.sky_color;
            fullbright = true;
            return;
        }

        if (r_view.texturing)
        {
            img = W_GetFlat(fname);

            if (!img)
            {
                img        = IM_UnknownFlat();
                fullbright = render_unknown_bright;
            }

            return;
        }

        // when lighting and no texturing, use a single color
        if (r_view.lighting)
            col = Misc_info.floor_colors[1];
        else
            col = HashedPalColor(fname, Misc_info.floor_colors);
    }

    void FindTex(const char *tname, LineDef *ld)
    {
        fullbright = false;

        if (r_view.texturing)
        {
            if (is_null_tex(tname))
            {
                img        = IM_MissingTex();
                fullbright = render_missing_bright;
                return;
            }
            else if (is_special_tex(tname))
            {
                img = IM_SpecialTex();
                return;
            }

            img = W_GetTexture(tname);

            if (!img)
            {
                img        = IM_UnknownTex();
                fullbright = render_unknown_bright;
            }

            return;
        }

        // when lighting and no texturing, use a single color
        if (r_view.lighting)
            col = Misc_info.wall_colors[1];
        else
            col = HashedPalColor(tname, Misc_info.wall_colors);
    }
};

struct DrawWall
{
  public:
    typedef std::vector<struct DrawWall *> vec_t;

    // when 'th' is >= 0, this is actually a sprite, and 'ld' and
    // 'sd' will be NULL.  Sprites use the info in the 'ceil' surface.
    int th;

    LineDef *ld;
    SideDef *sd;
    Sector  *sec;

    // which side this wall faces (SIDE_LEFT or SIDE_RIGHT)
    // for sprites: a copy of the thinginfo flags
    int side;

    // the linedef index
    int ld_index;

    // lighting for wall, adjusted for N/S and E/W walls
    int wall_light;

    // line constants
    float delta_ang;
    float dist, t_dist;
    float normal; // scale for things

    // distance values (inverted, so they can be lerped)
    double iz1, iz2;
    double diz, cur_iz;
    double mid_iz;

    // translate coord, for sprite
    float spr_tx1;

    // screen X coordinates
    int sx1, sx2;

    // for sprites, the remembered open space to clip to
    int oy1, oy2;

    /* surfaces */

    DrawSurf ceil;
    DrawSurf upper;
    DrawSurf lower;
    DrawSurf floor;
    DrawSurf rail;

    // IsCloser tests if THIS wall (wall A) is closer to the camera
    // than the given wall (wall B).
    //
    // Note that it is NOT suitable as a predicate for std::sort()
    // since it does not guarantee a linear order (total order) of
    // the elements.  Hence the need for our own sorting code.

    inline bool IsCloser(const DrawWall *const B) const
    {
        const DrawWall *const A = this;

        if (A == B)
            return false;

        if (A->ld && B->ld)
        {
            // handle cases where two linedefs share a vertex, since that
            // is where slime-trails would otherwise occur.

            // if they do share a vertex, we check if the other vertex of
            // wall A and the camera position are both on the same side of
            // wall B (extended to infinity).

            int A_other = -1;

            if (B->ld->TouchesVertex(A->ld->start))
                A_other = A->ld->end;
            else if (B->ld->TouchesVertex(A->ld->end))
                A_other = A->ld->start;

            if (A_other >= 0)
            {
                int ax = Vertices[A_other]->x();
                int ay = Vertices[A_other]->y();

                int bx1 = B->ld->Start()->x();
                int by1 = B->ld->Start()->y();
                int bx2 = B->ld->End()->x();
                int by2 = B->ld->End()->y();

                int cx = (int)r_view.x; // camera
                int cy = (int)r_view.y;

                int A_side = PointOnLineSide(ax, ay, bx1, by1, bx2, by2);
                int C_side = PointOnLineSide(cx, cy, bx1, by1, bx2, by2);

                return (A_side * C_side >= 0);
            }
        }
        else if (A->th >= 0 && B->th >= 0)
        {
            // prevent two things at same location from flickering
            const Thing *const TA = Things[A->th];
            const Thing *const TB = Things[B->th];

            if (TA->raw_x == TB->raw_x && TA->raw_y == TB->raw_y)
                return A->th > B->th;
        }

        return A->cur_iz > B->cur_iz;
    }

    /* PREDICATES */

    struct MidDistCmp
    {
        inline bool operator()(const DrawWall *A, const DrawWall *B) const
        {
            return A->mid_iz > B->mid_iz;
        }
    };

    struct SX1Cmp
    {
        inline bool operator()(const DrawWall *A, const DrawWall *B) const
        {
            return A->sx1 < B->sx1;
        }

        inline bool operator()(const DrawWall *A, int x) const
        {
            return A->sx1 < x;
        }

        inline bool operator()(int x, const DrawWall *A) const
        {
            return x < A->sx1;
        }
    };

    struct SX2Less
    {
        int x;

        SX2Less(int _x) : x(_x)
        {
        }

        inline bool operator()(const DrawWall *A) const
        {
            return A->sx2 < x;
        }
    };

    /* methods */

    Sector *Boom242Sector(Sector *real, Sector *temp, const Sector *dummy)
    {
        *temp = *real;

        temp->floorh = dummy->floorh;
        temp->ceilh  = dummy->ceilh;

        if (dummy->floorh > real->floorh && r_view.z < dummy->floorh)
        {
            // space C : underwater
            temp->floorh = real->floorh;
            temp->ceilh  = dummy->floorh;

            temp->floor_tex = dummy->floor_tex;
            temp->ceil_tex  = dummy->ceil_tex;
        }
        else if (dummy->ceilh < real->ceilh && r_view.z > dummy->ceilh)
        {
            // space A : head above ceiling
            temp->floorh = dummy->ceilh;
            temp->ceilh  = real->ceilh;

            temp->floor_tex = dummy->floor_tex;
            temp->ceil_tex  = dummy->ceil_tex;
        }
        else
        {
            // space B : normal
        }

        return temp;
    }

    void ComputeWallSurface()
    {
        Sector *front = sec;
        Sector *back  = NULL;

        SideDef *back_sd = (side == SIDE_LEFT) ? ld->Right() : ld->Left();
        if (back_sd)
            back = Sectors[back_sd->sector];

        // support for BOOM's 242 "transfer heights" line type
        Sector temp_front;
        Sector temp_back;

        sector_3dfloors_c *exfloor = Subdiv_3DFloorsForSector(sd->sector);
        if (exfloor->heightsec >= 0)
        {
            const Sector *dummy = Sectors[exfloor->heightsec];
            front               = Boom242Sector(front, &temp_front, dummy);
        }

        if (back != NULL)
        {
            exfloor = Subdiv_3DFloorsForSector(back_sd->sector);
            if (exfloor->heightsec >= 0)
            {
                const Sector *dummy = Sectors[exfloor->heightsec];
                back                = Boom242Sector(back, &temp_back, dummy);
            }
        }

        bool sky_upper = back && is_sky(front->CeilTex()) && is_sky(back->CeilTex());
        bool self_ref  = (front == back) ? true : false;

        if ((front->ceilh > r_view.z || is_sky(front->CeilTex())) && !sky_upper && !self_ref)
        {
            ceil.kind   = DrawSurf::K_FLAT;
            ceil.h1     = front->ceilh;
            ceil.h2     = +99999;
            ceil.tex_h  = ceil.h1;
            ceil.y_clip = DrawSurf::SOLID_ABOVE;

            ceil.FindFlat(front->CeilTex(), front);
        }

        if (front->floorh < r_view.z && !self_ref)
        {
            floor.kind   = DrawSurf::K_FLAT;
            floor.h1     = -99999;
            floor.h2     = front->floorh;
            floor.tex_h  = floor.h2;
            floor.y_clip = DrawSurf::SOLID_BELOW;

            floor.FindFlat(front->FloorTex(), front);
        }

        if (!back)
        {
            /* ONE-sided line */

            lower.kind   = DrawSurf::K_TEXTURE;
            lower.h1     = front->floorh;
            lower.h2     = front->ceilh;
            lower.y_clip = DrawSurf::SOLID_ABOVE | DrawSurf::SOLID_BELOW;

            lower.FindTex(sd->MidTex(), ld);

            if (lower.img && (ld->flags & MLF_LowerUnpegged))
                lower.tex_h = lower.h1 + lower.img->height();
            else
                lower.tex_h = lower.h2;

            lower.tex_h += sd->y_offset;
            return;
        }

        /* TWO-sided line */

        if (back->ceilh < front->ceilh && !sky_upper && !self_ref)
        {
            upper.kind   = DrawSurf::K_TEXTURE;
            upper.h1     = back->ceilh;
            upper.h2     = front->ceilh;
            upper.y_clip = DrawSurf::SOLID_ABOVE;

            upper.FindTex(sd->UpperTex(), ld);

            if (upper.img && !(ld->flags & MLF_UpperUnpegged))
                upper.tex_h = upper.h1 + upper.img->height();
            else
                upper.tex_h = upper.h2;

            upper.tex_h += sd->y_offset;
        }

        if (back->floorh > front->floorh && !self_ref)
        {
            lower.kind   = DrawSurf::K_TEXTURE;
            lower.h1     = front->floorh;
            lower.h2     = back->floorh;
            lower.y_clip = DrawSurf::SOLID_BELOW;

            lower.FindTex(sd->LowerTex(), ld);

            // note "sky_upper" here, needed to match original DOOM behavior
            if (ld->flags & MLF_LowerUnpegged)
                lower.tex_h = sky_upper ? back->ceilh : front->ceilh;
            else
                lower.tex_h = lower.h2;

            lower.tex_h += sd->y_offset;
        }

        /* Mid-Masked texture */

        if (!r_view.texturing)
            return;

        if (is_null_tex(sd->MidTex()))
            return;

        rail.FindTex(sd->MidTex(), ld);
        if (!rail.img)
            return;

        front = sec;
        back  = Sectors[back_sd->sector];

        int c_h = MIN(front->ceilh, back->ceilh);
        int f_h = MAX(front->floorh, back->floorh);
        int r_h = rail.img->height();

        if (f_h >= c_h)
            return;

        if (ld->flags & MLF_LowerUnpegged)
        {
            rail.h1 = f_h + sd->y_offset;
            rail.h2 = rail.h1 + r_h;
        }
        else
        {
            rail.h2 = c_h + sd->y_offset;
            rail.h1 = rail.h2 - r_h;
        }

        rail.kind   = DrawSurf::K_TEXTURE;
        rail.y_clip = 0;
        rail.tex_h  = rail.h2;

        // clip railing, unless sectors on both sides are identical or
        // we have a sky upper

        if (!(sky_upper ||
              (back->ceilh == front->ceilh && back->ceil_tex == front->ceil_tex && back->light == front->light)))
        {
            rail.h2 = MIN(c_h, rail.h2);
        }

        if (!(back->floorh == front->floorh && back->floor_tex == front->floor_tex && back->light == front->light))
        {
            rail.h1 = MAX(f_h, rail.h1);
        }
    }
};

struct RendInfo
{
  public:
    // complete set of walls/sprites to draw.
    DrawWall::vec_t walls;

    // the active list.  Pointers here are always duplicates of ones in
    // the walls list (no need to 'delete' any of them).
    DrawWall::vec_t active;

    // query state
    int query_mode; // 0 for normal render
    int query_sx;
    int query_sy;

    Objid query_result;
    float query_map_x;
    float query_map_y;
    float query_map_z;

    // inverse distances over X range, 0 when empty.
    std::vector<double> depth_x;

    // vertical clip window, an inclusive range
    int open_y1;
    int open_y2;

    // these used by Highlight()
    int hl_ox, hl_oy;
    int hl_thick;
#ifdef _FLTK_DISABLED
    Fl_Color hl_color;
#endif

  private:
    static void DeleteWall(DrawWall *P)
    {
        delete P;
    }

  public:
    RendInfo() : walls(), active(), query_mode(0), query_sx(), query_sy(), depth_x(), open_y1(), open_y2()
    {
    }

    ~RendInfo()
    {
        std::for_each(walls.begin(), walls.end(), DeleteWall);

        walls.clear();
        active.clear();
    }

    void InitDepthBuf(int width)
    {
        depth_x.resize(width);

        std::fill_n(depth_x.begin(), width, 0);
    }

    void DrawHighlightLine(int sx1, int sy1, int sx2, int sy2)
    {
        if (!render_high_detail)
        {
            sx1 *= 2;
            sy1 *= 2;
            sx2 *= 2;
            sy2 *= 2;
        }

#ifdef _FLTK_DISABLED
        fl_color(hl_color);

        if (hl_thick)
            fl_line_style(FL_SOLID, 2);

        fl_line(hl_ox + sx1, hl_oy + sy1, hl_ox + sx2, hl_oy + sy2);

        if (hl_thick)
            fl_line_style(0);
#endif
    }

    static inline float PointToAngle(float x, float y)
    {
        if (-0.01 < x && x < 0.01)
            return (y > 0) ? M_PI / 2 : (3 * M_PI / 2);

        float angle = atan2(y, x);

        if (angle < 0)
            angle += 2 * M_PI;

        return angle;
    }

    static inline int AngleToX(float ang)
    {
        float t = tan(M_PI / 2 - ang);

        int x = int(r_view.aspect_sw * t);

        x = (r_view.screen_w + x) / 2;

        if (x < 0)
            x = 0;
        else if (x > r_view.screen_w)
            x = r_view.screen_w;

        return x;
    }

    static inline float XToAngle(int x)
    {
        x = x * 2 - r_view.screen_w;

        float ang = M_PI / 2 + atan(x / r_view.aspect_sw);

        if (ang < 0)
            ang = 0;
        else if (ang > M_PI)
            ang = M_PI;

        return ang;
    }

    static inline int DeltaToX(double iz, float tx)
    {
        int x = int(r_view.aspect_sw * tx * iz);

        x = (x + r_view.screen_w) / 2;

        return x;
    }

    static inline float XToDelta(int x, double iz)
    {
        x = x * 2 - r_view.screen_w;

        float tx = x / iz / r_view.aspect_sw;

        return tx;
    }

    static inline int DistToY(double iz, int sec_h)
    {
        if (sec_h > 32770)
            return -9999;

        if (sec_h < -32770)
            return +9999;

        int y = int(r_view.aspect_sh * (sec_h - r_view.z) * iz);

        return (r_view.screen_h - y) / 2;
    }

    static inline float YToDist(int y, int sec_h)
    {
        y = r_view.screen_h - y * 2;

        if (y == 0)
            return 999999;

        return r_view.aspect_sh * (sec_h - r_view.z) / y;
    }

    static inline float YToSecH(int y, double iz)
    {
        y = y * 2 - r_view.screen_h;

        return r_view.z - (float(y) / r_view.aspect_sh / iz);
    }

    void AddLine(int ld_index)
    {
        LineDef *ld = LineDefs[ld_index];

        if (!is_vertex(ld->start) || !is_vertex(ld->end))
            return;

        if (!ld->Right())
            return;

        float x1 = ld->Start()->x() - r_view.x;
        float y1 = ld->Start()->y() - r_view.y;
        float x2 = ld->End()->x() - r_view.x;
        float y2 = ld->End()->y() - r_view.y;

        float tx1 = x1 * r_view.Sin - y1 * r_view.Cos;
        float ty1 = x1 * r_view.Cos + y1 * r_view.Sin;
        float tx2 = x2 * r_view.Sin - y2 * r_view.Cos;
        float ty2 = x2 * r_view.Cos + y2 * r_view.Sin;

        // reject line if complete behind viewplane
        if (ty1 <= 0 && ty2 <= 0)
            return;

        float angle1 = PointToAngle(tx1, ty1);
        float angle2 = PointToAngle(tx2, ty2);
        float span   = angle1 - angle2;

        if (span < 0)
            span += 2 * M_PI;

        int side = SIDE_RIGHT;

        if (span >= M_PI)
            side = SIDE_LEFT;

        // ignore the line when there is no facing sidedef
        SideDef *sd = (side == SIDE_LEFT) ? ld->Left() : ld->Right();

        if (!sd)
            return;

        if (side == SIDE_LEFT)
        {
            float tmp = angle1;
            angle1    = angle2;
            angle2    = tmp;
        }

        // clip angles to view volume

        float base_ang = angle1;

        float leftclip  = (3 * M_PI / 4);
        float rightclip = M_PI / 4;

        float tspan1 = angle1 - rightclip;
        float tspan2 = leftclip - angle2;

        if (tspan1 < 0)
            tspan1 += 2 * M_PI;
        if (tspan2 < 0)
            tspan2 += 2 * M_PI;

        if (tspan1 > M_PI / 2)
        {
            // Totally off the left edge?
            if (tspan2 >= M_PI)
                return;

            angle1 = leftclip;
        }

        if (tspan2 > M_PI / 2)
        {
            // Totally off the left edge?
            if (tspan1 >= M_PI)
                return;

            angle2 = rightclip;
        }

        // convert angles to on-screen X positions
        int sx1 = AngleToX(angle1);
        int sx2 = AngleToX(angle2) - 1;

        if (sx1 > sx2)
            return;

        // optimisation for query mode
        if (query_mode && (sx2 < query_sx || sx1 > query_sx))
            return;

        // compute distance from eye to wall
        float wdx = x2 - x1;
        float wdy = y2 - y1;

        float wlen = sqrt(wdx * wdx + wdy * wdy);
        float dist = fabs((y1 * wdx / wlen) - (x1 * wdy / wlen));

        if (dist < 0.01)
            return;

        // compute normal of wall (translated coords)
        float normal;

        if (side == SIDE_LEFT)
            normal = PointToAngle(ty2 - ty1, tx1 - tx2);
        else
            normal = PointToAngle(ty1 - ty2, tx2 - tx1);

        // compute inverse distances
        double iz1 = cos(normal - angle1) / dist / cos(M_PI / 2 - angle1);
        double iz2 = cos(normal - angle2) / dist / cos(M_PI / 2 - angle2);

        double diz = (iz2 - iz1) / MAX(1, sx2 - sx1);

        // create drawwall structure

        DrawWall *dw = new DrawWall;

        dw->th       = -1;
        dw->ld       = ld;
        dw->ld_index = ld_index;

        dw->sd   = sd;
        dw->sec  = sd->SecRef();
        dw->side = side;

        dw->wall_light = dw->sec->light;

        // add "fake constrast" for axis-aligned walls
        if (ld->IsVertical())
            dw->wall_light += 16;
        else if (ld->IsHorizontal())
            dw->wall_light -= 16;

        dw->delta_ang = angle1 + XToAngle(sx1) - normal;

        dw->dist   = dist;
        dw->normal = normal;
        dw->t_dist = tan(base_ang - normal) * dist;

        dw->iz1    = iz1;
        dw->iz2    = iz2;
        dw->diz    = diz;
        dw->mid_iz = iz1 + (sx2 - sx1 + 1) * diz / 2;

        dw->sx1 = sx1;
        dw->sx2 = sx2;

        walls.push_back(dw);
    }

    void AddThing(int th_index)
    {
        Thing *th = Things[th_index];

        const thingtype_t *info = M_GetThingType(th->type);

        float x = th->x() - r_view.x;
        float y = th->y() - r_view.y;

        float tx = x * r_view.Sin - y * r_view.Cos;
        float ty = x * r_view.Cos + y * r_view.Sin;

        // reject sprite if complete behind viewplane
        if (ty < 4)
            return;

        bool is_unknown = false;

        float scale = info->scale;

        Img_c *sprite = W_GetSprite(th->type);
        if (!sprite)
        {
            sprite     = IM_UnknownSprite();
            is_unknown = true;
            scale      = 0.33;
        }

        float tx1 = tx - sprite->width() * scale / 2.0;
        float tx2 = tx + sprite->width() * scale / 2.0;

        double iz = 1 / ty;

        int sx1 = DeltaToX(iz, tx1);
        int sx2 = DeltaToX(iz, tx2) - 1;

        if (sx1 < 0)
            sx1 = 0;

        if (sx2 >= r_view.screen_w)
            sx2 = r_view.screen_w - 1;

        if (sx1 > sx2)
            return;

        // optimisation for query mode
        if (query_mode && (sx2 < query_sx || sx1 > query_sx))
            return;

        int thsec = r_view.thing_sectors[th_index];

        // check if thing is hidden by BOOM deep water
        if (is_sector(thsec))
        {
            sector_3dfloors_c *exfloor = Subdiv_3DFloorsForSector(thsec);
            if (is_sector(exfloor->heightsec))
            {
                const Sector *real  = Sectors[thsec];
                const Sector *dummy = Sectors[exfloor->heightsec];

                if (dummy->floorh > real->floorh && r_view.z > dummy->floorh &&
                    !(info && (info->flags & THINGDEF_CEIL)))
                {
                    return;
                }
            }
        }

        int h1, h2;

        if (info && (info->flags & THINGDEF_CEIL))
        {
            // IOANCH 9/2015: also add z
            h2 = (is_sector(thsec) ? Sectors[thsec]->ceilh : 192) - th->h();
            h1 = h2 - sprite->height() * scale;
        }
        else
        {
            h1 = (is_sector(thsec) ? Sectors[thsec]->floorh : 0) + th->h();
            h2 = h1 + sprite->height() * scale;
        }

        // create drawwall structure

        DrawWall *dw = new DrawWall;

        dw->th       = th_index;
        dw->ld_index = -1;
        dw->ld       = NULL;
        dw->sd       = NULL;
        dw->sec      = NULL;

        dw->side = info ? info->flags : 0;

        if (is_unknown && render_unknown_bright)
            dw->side |= THINGDEF_LIT;

        dw->spr_tx1 = tx1;

        dw->normal = scale;

        dw->iz1 = dw->mid_iz = iz;
        dw->diz              = 0;

        dw->sx1 = sx1;
        dw->sx2 = sx2;

        dw->ceil.img = sprite;
        dw->ceil.h1  = h1;
        dw->ceil.h2  = h2;

        walls.push_back(dw);
    }

    void ComputeSurfaces()
    {
        DrawWall::vec_t::iterator S;

        for (S = walls.begin(); S != walls.end(); S++)
        {
            if ((*S)->ld)
                (*S)->ComputeWallSurface();
        }
    }

    void QueryCalcCoord(const DrawWall *dw, obj_type_e what, int part)
    {
        float dist = 1.0 / dw->cur_iz;

        if (what == OBJ_SECTORS)
        {
            // sky surfaces require a check on Z height
            if (part == PART_CEIL && dw->sec->ceilh > r_view.z + 1)
                dist = YToDist(query_sy, dw->sec->ceilh);
            else if (part == PART_FLOOR && dw->sec->floorh < r_view.z - 1)
                dist = YToDist(query_sy, dw->sec->floorh);
        }

        if (dist < 4.0)
            dist = 4.0;

        float ang  = XToAngle(query_sx);
        float modv = cos(ang - M_PI / 2);

        float t_cos = cos(M_PI + -r_view.angle + ang) / modv;
        float t_sin = sin(M_PI + -r_view.angle + ang) / modv;

        query_map_x = r_view.x - t_sin * dist;
        query_map_y = r_view.y - t_cos * dist;
        query_map_z = YToSecH(query_sy, 1.0 / dist);

        // ensure we never produce X == 0
        if (query_map_x == 0)
            query_map_x = 0.01;
    }

    void HighlightWallBit(const DrawWall *dw, int ld_index, int part)
    {
        // check the part is on the side facing the camera
        int p_side = (part & PART_LF_ALL) ? SIDE_LEFT : SIDE_RIGHT;
        if (dw->side != p_side)
            return;

        int z1, z2;

        if (dw->ld->TwoSided())
        {
            const Sector *front = dw->ld->Right()->SecRef();
            const Sector *back  = dw->ld->Left()->SecRef();

            if (part & (PART_RT_LOWER | PART_LF_LOWER))
            {
                z1 = MIN(front->floorh, back->floorh);
                z2 = MAX(front->floorh, back->floorh);
            }
            else if (part & (PART_RT_UPPER | PART_LF_UPPER))
            {
                z1 = MIN(front->ceilh, back->ceilh);
                z2 = MAX(front->ceilh, back->ceilh);
            }
            else
            {
                if (!LD_RailHeights(z1, z2, dw->ld, dw->sd, front, back))
                    return;
            }
        }
        else
        {
            if (0 == (part & (PART_RT_LOWER | PART_LF_LOWER)))
                return;

            z1 = dw->sd->SecRef()->floorh;
            z2 = dw->sd->SecRef()->ceilh;
        }

        int x1 = dw->sx1;
        int x2 = dw->sx2 + 1;

        int ly1 = DistToY(dw->iz1, z2);
        int ly2 = DistToY(dw->iz1, z1);

        int ry1 = DistToY(dw->iz2, z2);
        int ry2 = DistToY(dw->iz2, z1);

        // workaround for crappy line clipping in X windows
        if (ly1 < -5000 || ly2 < -5000 || ly1 > 5000 || ly2 > 5000 || ry1 < -5000 || ry2 < -5000 || ry1 > 5000 ||
            ry2 > 5000)
            return;

        DrawHighlightLine(x1, ly1, x1, ly2);
        DrawHighlightLine(x2, ry1, x2, ry2);
        DrawHighlightLine(x1, ly1, x2, ry1);
        DrawHighlightLine(x1, ly2, x2, ry2);
    }

    void HighlightLines(int ld_index, int parts)
    {
        DrawWall::vec_t::iterator S;

        for (S = walls.begin(); S != walls.end(); S++)
        {
            const DrawWall *dw = (*S);
            if (!dw->ld)
                continue;

            int line2  = ld_index;
            int parts2 = parts;

            if (ld_index >= 0)
            {
                if (dw->ld_index != ld_index)
                    continue;
            }
            else
            {
                line2  = dw->ld_index;
                parts2 = edit.Selected->get_ext(line2);

                if (parts2 == 0)
                    continue;

                if (parts2 == 1)
                {
                    parts2 = 0;
#ifdef _FLTK_DISABLED
                    hl_color = SEL_COL;
#endif
                }
                else
                {
#ifdef _FLTK_DISABLED
                    hl_color = SEL3D_COL;
#endif
                }
            }

            /* right side */
            if (parts2 == 0 || (parts2 & PART_RT_LOWER))
                HighlightWallBit(dw, line2, PART_RT_LOWER);

            if (parts2 == 0 || (parts2 & PART_RT_UPPER))
                HighlightWallBit(dw, line2, PART_RT_UPPER);

            if (parts2 & PART_RT_RAIL)
                HighlightWallBit(dw, line2, PART_RT_RAIL);

            /* left side */
            if (parts2 == 0 || (parts2 & PART_LF_LOWER))
                HighlightWallBit(dw, line2, PART_LF_LOWER);

            if (parts2 == 0 || (parts2 & PART_LF_UPPER))
                HighlightWallBit(dw, line2, PART_LF_UPPER);

            if (parts2 & PART_LF_RAIL)
                HighlightWallBit(dw, line2, PART_LF_RAIL);
        }
    }

    void HighlightSectorBit(const DrawWall *dw, int sec_index, int part)
    {
        const Sector *S = Sectors[sec_index];

        int z = (part == PART_CEIL) ? S->ceilh : S->floorh;

        // are we dragging this surface?
        if (edit.action == ACT_DRAG &&
            (!edit.dragged.valid() ||
             (edit.dragged.num == sec_index && (edit.dragged.parts == 0 || (edit.dragged.parts & part)))))
        {
            z = z + (int)edit.drag_sector_dz;
        }
        else
        {
            // check that plane faces the camera
            if (part == PART_FLOOR && (r_view.z < z + 0.2))
                return;
            if (part == PART_CEIL && (r_view.z > z - 0.2))
                return;
        }

        int sy1 = DistToY(dw->iz1, z);
        int sy2 = DistToY(dw->iz2, z);

        // workaround for crappy line clipping in X windows
        if (sy1 < -5000 || sy2 < -5000 || sy1 > 5000 || sy2 > 5000)
            return;

        DrawHighlightLine(dw->sx1, sy1, dw->sx2, sy2);
    }

    void HighlightSectors(int sec_index, int parts)
    {
        DrawWall::vec_t::iterator S;

        for (S = walls.begin(); S != walls.end(); S++)
        {
            const DrawWall *dw = (*S);
            if (!dw->ld)
                continue;

            if (sec_index >= 0)
            {
                if (!dw->ld->TouchesSector(sec_index))
                    continue;

                // Note: hl_color already set by caller

                if (parts == 0 || (parts & PART_FLOOR))
                    HighlightSectorBit(dw, sec_index, PART_FLOOR);

                if (parts == 0 || (parts & PART_CEIL))
                    HighlightSectorBit(dw, sec_index, PART_CEIL);

                continue;
            }

            /* doing the selection */

            for (int what_side = 0; what_side < 2; what_side++)
            {
                const SideDef *sd_front = dw->ld->Right();
                const SideDef *sd_back  = dw->ld->Left();

                if (sd_front && sd_back && sd_front == sd_back)
                    break;

                if (what_side == 1)
                    std::swap(sd_front, sd_back);

                if (sd_front == NULL)
                    continue;

                int sec2 = sd_front->sector;

                parts = edit.Selected->get_ext(sec2);
                if (parts == 0)
                    continue;

                if (parts == 1)
                {
                    parts = PART_FLOOR | PART_CEIL;
#ifdef _FLTK_DISABLED
                    hl_color = SEL_COL;
#endif
                }
                else
                {
#ifdef _FLTK_DISABLED
                    hl_color = SEL3D_COL;
#endif
                }

                if (parts & PART_FLOOR)
                    HighlightSectorBit(dw, sec2, PART_FLOOR);

                if (parts & PART_CEIL)
                    HighlightSectorBit(dw, sec2, PART_CEIL);
            }
        }
    }

    void HighlightThings(int th_index)
    {
        DrawWall::vec_t::iterator S;

        for (S = walls.begin(); S != walls.end(); S++)
        {
            const DrawWall *dw = (*S);
            if (dw->th < 0)
                continue;

            if (th_index >= 0)
            {
                if (dw->th != th_index)
                    continue;
            }
            else
            {
                if (!edit.Selected->get(dw->th))
                    continue;
            }

            int x1 = dw->sx1 - 1;
            int x2 = dw->sx2 + 1;

            int h1 = dw->ceil.h1 - 1;
            int h2 = dw->ceil.h2 + 1;

            int y1 = DistToY(dw->iz1, h2);
            int y2 = DistToY(dw->iz1, h1);

            if (edit.action == ACT_DRAG && (!edit.dragged.valid() || edit.dragged.num == th_index))
            {
                // re-project thing onto the viewplane
                float dx = edit.drag_cur_x - edit.drag_start_x;
                float dy = edit.drag_cur_y - edit.drag_start_y;
                float dz = edit.drag_cur_z - edit.drag_start_z;

                const Thing *T = Things[dw->th];

                float x = T->x() + dx - r_view.x;
                float y = T->y() + dy - r_view.y;

                float tx = x * r_view.Sin - y * r_view.Cos;
                float ty = x * r_view.Cos + y * r_view.Sin;

                if (ty < 1)
                    ty = 1;

                float  scale  = dw->normal;
                Img_c *sprite = dw->ceil.img;

                float tx1 = tx - sprite->width() * scale / 2.0;
                float tx2 = tx + sprite->width() * scale / 2.0;

                double iz = 1 / ty;

                x1 = DeltaToX(iz, tx1) - 1;
                x2 = DeltaToX(iz, tx2) + 1;

                int thsec = r_view.thing_sectors[th_index];

                if (dw->side & THINGDEF_CEIL)
                {
                    h2 = (is_sector(thsec) ? Sectors[thsec]->ceilh : 192) - T->h();
                    h1 = h2 - sprite->height() * scale;
                }
                else
                {
                    h1 = (is_sector(thsec) ? Sectors[thsec]->floorh : 0) + T->h();
                    h2 = h1 + sprite->height() * scale;
                }

                h1 = h1 + dz - 1;
                h2 = h2 + dz + 1;

                y1 = DistToY(iz, h2);
                y2 = DistToY(iz, h1);
            }

            DrawHighlightLine(x1, y1, x1, y2);
            DrawHighlightLine(x2, y1, x2, y2);
            DrawHighlightLine(x1, y1, x2, y1);
            DrawHighlightLine(x1, y2, x2, y2);
        }
    }

    void Highlight(int ox, int oy)
    {
        hl_ox = ox;
        hl_oy = oy;

        hl_thick = 2;

        switch (edit.mode)
        {
        case OBJ_THINGS:
#ifdef _FLTK_DISABLED
            hl_color = SEL_COL;
            HighlightThings(-1);

            hl_color = HI_COL;
            if (edit.action == ACT_DRAG && edit.dragged.valid())
            {
                HighlightThings(edit.dragged.num);
            }
            else if (edit.highlight.valid())
            {
                if (edit.Selected->get(edit.highlight.num))
                    hl_color = HI_AND_SEL_COL;

                HighlightThings(edit.highlight.num);
            }
#endif
            break;

        case OBJ_SECTORS:
#ifdef _FLTK_DISABLED
            HighlightSectors(-1, -1);

            hl_color = HI_COL;
            if (edit.action == ACT_DRAG && edit.dragged.valid())
            {
                HighlightSectors(edit.dragged.num, edit.dragged.parts);
            }
            else if (edit.highlight.valid())
            {
                if (edit.Selected->get(edit.highlight.num))
                    hl_color = HI_AND_SEL_COL;

                HighlightSectors(edit.highlight.num, edit.highlight.parts);
            }
#endif
            break;

        case OBJ_LINEDEFS:
#ifdef _FLTK_DISABLED
            HighlightLines(-1, -1);

            hl_color = HI_COL;
            if (edit.action == ACT_DRAG && edit.dragged.valid())
            {
                if (edit.Selected->get(edit.highlight.num))
                    hl_color = HI_AND_SEL_COL;

                HighlightLines(edit.highlight.num, edit.highlight.parts);
            }
#endif
            break;

        default:
            break;
        }
    }

    void ClipSolids()
    {
        // perform a rough depth sort of the walls and sprites.

        std::sort(walls.begin(), walls.end(), DrawWall::MidDistCmp());

        // go forwards, from closest to furthest away

        DrawWall::vec_t::iterator S;

        for (S = walls.begin(); S != walls.end(); S++)
        {
            DrawWall *dw = (*S);

            if (!dw)
                continue;

            int one_sided = dw->ld && !dw->ld->Left();
            int vis_count = dw->sx2 - dw->sx1 + 1;

            for (int x = dw->sx1; x <= dw->sx2; x++)
            {
                double iz = dw->iz1 + (dw->diz * (x - dw->sx1));

                if (iz < depth_x[x])
                    vis_count--;
                else if (one_sided)
                    depth_x[x] = iz;
            }

            if (vis_count == 0)
            {
                delete dw;
                (*S) = NULL;
            }
        }

        // remove null pointers

        S = std::remove(walls.begin(), walls.end(), (DrawWall *)NULL);

        walls.erase(S, walls.end());
    }

    void RenderFlatColumn(DrawWall *dw, DrawSurf &surf, int x, int y1, int y2)
    {
        img_pixel_t *dest = r_view.screen;

        const img_pixel_t *src = surf.img->buf();

        int tw = surf.img->width();
        int th = surf.img->height();

        float ang  = XToAngle(x);
        float modv = cos(ang - M_PI / 2);

        float t_cos = cos(M_PI + -r_view.angle + ang) / modv;
        float t_sin = sin(M_PI + -r_view.angle + ang) / modv;

        dest += x + y1 * r_view.screen_w;

        int light = dw->sec->light;

        for (; y1 <= y2; y1++, dest += r_view.screen_w)
        {
            float dist = YToDist(y1, surf.tex_h);

            int tx = int(r_view.x - t_sin * dist) & (tw - 1);
            int ty = int(-r_view.y + t_cos * dist) & (th - 1);

            *dest = src[ty * tw + tx];

            if (r_view.lighting && !surf.fullbright)
                *dest = DoomLightRemap(light, dist, *dest);
        }
    }

    void RenderTexColumn(DrawWall *dw, DrawSurf &surf, int x, int y1, int y2)
    {
        img_pixel_t *dest = r_view.screen;

        const img_pixel_t *src = surf.img->buf();

        int tw = surf.img->width();
        int th = surf.img->height();

        int   light = dw->wall_light;
        float dist  = 1.0 / dw->cur_iz;

        /* compute texture X coord */

        float cur_ang = dw->delta_ang - XToAngle(x);

        int tx = int(dw->t_dist - tan(cur_ang) * dw->dist);

        tx = (dw->sd->x_offset + tx) & (tw - 1);

        /* compute texture Y coords */

        float hh = surf.tex_h - YToSecH(y1, dw->cur_iz);
        float dh = surf.tex_h - YToSecH(y2, dw->cur_iz);

        dh = (dh - hh) / MAX(1, y2 - y1);
        hh += 0.2;

        src += tx;
        dest += x + y1 * r_view.screen_w;

        for (; y1 <= y2; y1++, hh += dh, dest += r_view.screen_w)
        {
            int ty = int(floor(hh)) % th;

            // handle negative values (use % twice)
            ty = (ty + th) % th;

            img_pixel_t pix = src[ty * tw];

            if (pix == TRANS_PIXEL)
                continue;

            if (r_view.lighting && !surf.fullbright)
                *dest = DoomLightRemap(light, dist, pix);
            else
                *dest = pix;
        }
    }

    void SolidFlatColumn(DrawWall *dw, DrawSurf &surf, int x, int y1, int y2)
    {
        img_pixel_t *dest = r_view.screen;

        dest += x + y1 * r_view.screen_w;

        int light = dw->sec->light;

        for (; y1 <= y2; y1++, dest += r_view.screen_w)
        {
            float dist = YToDist(y1, surf.tex_h);

            if (r_view.lighting && !surf.fullbright)
                *dest = DoomLightRemap(light, dist, surf.col);
            else
                *dest = surf.col;
        }
    }

    void SolidTexColumn(DrawWall *dw, DrawSurf &surf, int x, int y1, int y2)
    {
        int   light = dw->wall_light;
        float dist  = 1.0 / dw->cur_iz;

        img_pixel_t *dest = r_view.screen;

        dest += x + y1 * r_view.screen_w;

        for (; y1 <= y2; y1++, dest += r_view.screen_w)
        {
            if (r_view.lighting && !surf.fullbright)
                *dest = DoomLightRemap(light, dist, surf.col);
            else
                *dest = surf.col;
        }
    }

    inline void RenderWallSurface(DrawWall *dw, DrawSurf &surf, int x, obj_type_e what, int part)
    {
        if (surf.kind == DrawSurf::K_INVIS)
            return;

        int y1 = DistToY(dw->cur_iz, surf.h2);
        int y2 = DistToY(dw->cur_iz, surf.h1) - 1;

        // clip to the open region
        if (y1 < open_y1)
            y1 = open_y1;

        if (y2 > open_y2)
            y2 = open_y2;

        // update open region based on ends which are "solid"
        if (surf.y_clip & DrawSurf::SOLID_ABOVE)
            open_y1 = MAX(open_y1, y2 + 1);

        if (surf.y_clip & DrawSurf::SOLID_BELOW)
            open_y2 = MIN(open_y2, y1 - 1);

        if (y1 > y2)
            return;

        /* query mode : is mouse over this wall part? */

        if (query_mode)
        {
            if (y1 <= query_sy && query_sy <= y2)
            {
                if (what == OBJ_LINEDEFS)
                {
                    if (dw->side < 0)
                        part <<= 4;

                    query_result = Objid(what, dw->ld_index, part);
                }
                else if (dw->sd != NULL)
                {
                    query_result = Objid(what, dw->sd->sector, part);
                }

                QueryCalcCoord(dw, what, part);
            }
            return;
        }

        /* fill pixels */

        if (!surf.img)
        {
            if (surf.kind == DrawSurf::K_FLAT)
                SolidFlatColumn(dw, surf, x, y1, y2);
            else
                SolidTexColumn(dw, surf, x, y1, y2);
        }
        else
            switch (surf.kind)
            {
            case DrawSurf::K_FLAT:
                RenderFlatColumn(dw, surf, x, y1, y2);
                break;

            case DrawSurf::K_TEXTURE:
                RenderTexColumn(dw, surf, x, y1, y2);
                break;
            }
    }

    inline void RenderSprite(DrawWall *dw, int x)
    {
        int y1 = DistToY(dw->cur_iz, dw->ceil.h2);
        int y2 = DistToY(dw->cur_iz, dw->ceil.h1) - 1;

        if (y1 < dw->oy1)
            y1 = dw->oy1;

        if (y2 > dw->oy2)
            y2 = dw->oy2;

        if (y1 > y2)
            return;

        if (query_mode)
        {
            if (y1 <= query_sy && query_sy <= y2 && edit.mode == OBJ_THINGS)
            {
                query_result = Objid(OBJ_THINGS, dw->th);
            }
            return;
        }

        int tw = dw->ceil.img->width();
        int th = dw->ceil.img->height();

        float scale = dw->normal;

        int tx = int((XToDelta(x, dw->cur_iz) - dw->spr_tx1) / scale);

        if (tx < 0 || tx >= tw)
            return;

        float hh = dw->ceil.h2 - YToSecH(y1, dw->cur_iz);
        float dh = dw->ceil.h2 - YToSecH(y2, dw->cur_iz);

        dh = (dh - hh) / MAX(1, y2 - y1);

        int   thsec = r_view.thing_sectors[dw->th];
        int   light = is_sector(thsec) ? Sectors[thsec]->light : 255;
        float dist  = 1.0 / dw->cur_iz;

        /* fill pixels */

        img_pixel_t *dest = r_view.screen;
        dest += x + y1 * r_view.screen_w;

        const img_pixel_t *src = dw->ceil.img->buf();
        src += tx;

        for (; y1 <= y2; y1++, hh += dh, dest += r_view.screen_w)
        {
            int ty = int(hh / scale);

            if (ty < 0 || ty >= th)
                continue;

            img_pixel_t pix = src[ty * tw];

            if (pix == TRANS_PIXEL)
                continue;

            if (dw->side & THINGDEF_INVIS)
            {
                if (*dest & IS_RGB_PIXEL)
                    *dest = IS_RGB_PIXEL | ((*dest & 0x7bde) >> 1);
                else
                    *dest = raw_colormap[14][*dest];
                continue;
            }

            *dest = pix;

            if (r_view.lighting && !(dw->side & THINGDEF_LIT))
                *dest = DoomLightRemap(light, dist, *dest);
        }
    }

    inline void RenderMidMasker(DrawWall *dw, DrawSurf &surf, int x)
    {
        if (surf.kind == DrawSurf::K_INVIS)
            return;

        if (!surf.img)
            return;

        int y1 = DistToY(dw->cur_iz, surf.h2);
        int y2 = DistToY(dw->cur_iz, surf.h1) - 1;

        if (y1 < dw->oy1)
            y1 = dw->oy1;

        if (y2 > dw->oy2)
            y2 = dw->oy2;

        if (y1 > y2)
            return;

        if (query_mode)
        {
            if (y1 <= query_sy && query_sy <= y2 && edit.mode == OBJ_LINEDEFS)
            {
                int part     = (dw->side < 0) ? PART_LF_RAIL : PART_RT_RAIL;
                query_result = Objid(OBJ_LINEDEFS, dw->ld_index, part);
            }
            return;
        }

        /* fill pixels */

        RenderTexColumn(dw, surf, x, y1, y2);
    }

    inline void Sort_Swap(int i, int k)
    {
        DrawWall *A = active[i];
        DrawWall *B = active[k];

        active[k] = A;
        active[i] = B;
    }

    int Sort_Partition(int lo, int hi, int pivot_idx)
    {
        /* this is Hoare's algorithm */

        const DrawWall *pivot = active[pivot_idx];

        int s = lo;
        int e = hi;

        for (;;)
        {
            while (s <= e && active[s]->IsCloser(pivot))
                s++;

            if (s > hi)
            {
                // all values were < pivot, including the pivot itself!

                if (pivot_idx != hi)
                    Sort_Swap(pivot_idx, hi);

                return hi - 1;
            }

            while (e >= s && !active[e]->IsCloser(pivot))
                e--;

            if (e < lo)
            {
                // all values were >= pivot

                if (pivot_idx != lo)
                    Sort_Swap(pivot_idx, lo);

                return lo;
            }

            if (s < e)
            {
                Sort_Swap(s, e);

                s++;
                e--;

                continue;
            }

            /* NOT NEEDED (it seems)
            if (s == e && active[s]->IsCloser(pivot))
                    s++;
            */

            return s - 1;
        }
    }

    void Sort_Range(int s, int e)
    {
        SYS_ASSERT(s <= e);

        while (s < e)
        {
            // previously there was a bubble sort here, but timing
            // tests showed that it was overkill.  This is enough.
            if (s == e - 1)
            {
                const DrawWall *const A = active[s];
                const DrawWall *const B = active[e];

                if (B->IsCloser(A))
                    Sort_Swap(s, e);

                return;
            }

            // since we are usually sorting a mostly-sorted list, the
            // wall in the middle is highly likely to be a good pivot.
            int pivot_idx = (s + e) >> 1;

            int mid = Sort_Partition(s, e, pivot_idx);

            // handle degenerate cases
            if (mid <= s)
            {
                s++;
                continue;
            }
            else if (mid + 1 >= e)
            {
                e--;
                continue;
            }

            // only use recursion on the smallest group
            // [ it helps to limit stack usage ]
            if ((mid - s) < (e - mid))
            {
                Sort_Range(s, mid);
                s = mid + 1;
            }
            else
            {
                Sort_Range(mid + 1, e);
                e = mid;
            }
        }
    }

    void SortActiveList()
    {
        // this uses the Quicksort algorithm to sort the active list.
        //
        // Note that this sorting code has been written assuming some
        // limitations of the DrawWall::IsCloser() method -- see the
        // description of that method for more details.

        if (active.size() < 2)
            return;

        Sort_Range(0, (int)active.size() - 1);
    }

#define IZ_EPSILON 1e-5

    void UpdateActiveList(int x)
    {
        DrawWall::vec_t::iterator S, E, P;

        bool changes = false;

        // remove walls that have finished.

        S = active.begin();
        E = active.end();

        S = std::remove_if(S, E, DrawWall::SX2Less(x));

        if (S != E)
        {
            active.erase(S, E);
            changes = true;
        }

        // add new walls that start in this column.

        S = walls.begin();
        E = walls.end();

        S = std::lower_bound(S, E, x, DrawWall::SX1Cmp());
        E = std::upper_bound(S, E, x, DrawWall::SX1Cmp());

        if (S != E)
            changes = true;

        for (; S != E; S++)
        {
            active.push_back(*S);
        }

        // calculate new depth values

        S = active.begin();
        E = active.end();

        for (P = S; (P != E); P++)
        {
            DrawWall *dw = (*P);

            dw->cur_iz = dw->iz1 + dw->diz * (x - dw->sx1);

            if (P != S && (*(P - 1))->cur_iz < dw->cur_iz + IZ_EPSILON)
                changes = true;
        }

        // if there are changes, re-sort the active list...

        if (changes && active.size() > 0)
        {
            SortActiveList();
        }
    }

    void RenderWalls()
    {
        // sort walls by their starting column, to allow binary search.

        std::sort(walls.begin(), walls.end(), DrawWall::SX1Cmp());

        active.clear();

        for (int x = 0; x < r_view.screen_w; x++)
        {
            // clear vertical depth buffer

            open_y1 = 0;
            open_y2 = r_view.screen_h - 1;

            UpdateActiveList(x);

            // in query mode, only care about a single column
            if (query_mode && x != query_sx)
                continue;

            // render, front to back

            DrawWall::vec_t::iterator S, E, P;

            S = active.begin();
            E = active.end();

            for (P = S; P != E; P++)
            {
                DrawWall *dw = (*P);

                // for things, just remember the open space
                {
                    dw->oy1 = open_y1;
                    dw->oy2 = open_y2;
                }
                if (dw->th >= 0)
                    continue;

                RenderWallSurface(dw, dw->ceil, x, OBJ_SECTORS, PART_CEIL);
                RenderWallSurface(dw, dw->floor, x, OBJ_SECTORS, PART_FLOOR);

                RenderWallSurface(dw, dw->upper, x, OBJ_LINEDEFS, PART_RT_UPPER);
                RenderWallSurface(dw, dw->lower, x, OBJ_LINEDEFS, PART_RT_LOWER);

                if (open_y1 > open_y2)
                    break;
            }

            // now render things, back to front
            // (mid-masked textures are done here too)

            if (P == E)
                P--;

            for (; P != (S - 1); P--)
            {
                DrawWall *dw = (*P);

                if (dw->th >= 0)
                    RenderSprite(dw, x);
                else
                    RenderMidMasker(dw, dw->rail, x);
            }
        }
    }

    void ClearScreen()
    {
        // color #0 is black (DOOM, Heretic, Hexen)
        // [ other colors won't work here, since img_pixel_t is 16 bits ]
        byte COLOR = 0;

        size_t total = r_view.screen_w * r_view.screen_h;

        memset(r_view.screen, COLOR, total * sizeof(r_view.screen[0]));
    }

    void Render()
    {
        if (!query_mode)
            ClearScreen();

        InitDepthBuf(r_view.screen_w);

        for (int i = 0; i < NumLineDefs; i++)
            AddLine(i);

        if (r_view.sprites)
            for (int k = 0; k < NumThings; k++)
                AddThing(k);

        ClipSolids();

        ComputeSurfaces();

        RenderWalls();
    }

    void Query(int qx, int qy)
    {
        query_mode = 1;
        query_sx   = qx;
        query_sy   = qy;
        query_result.clear();
        query_map_x = 0;
        query_map_y = 0;
        query_map_z = 0;

        Render();

        query_mode = 0;
    }
};

static void BlitHires(int ox, int oy, int ow, int oh)
{
#ifdef _FLTK_DISABLED
    u8_t line_rgb[r_view.screen_w * 3];

    for (int ry = 0; ry < r_view.screen_h; ry++)
    {
        u8_t *dest     = line_rgb;
        u8_t *dest_end = line_rgb + r_view.screen_w * 3;

        const img_pixel_t *src = r_view.screen + ry * r_view.screen_w;

        for (; dest < dest_end; dest += 3, src++)
        {
            IM_DecodePixel(*src, dest[0], dest[1], dest[2]);
        }

        fl_draw_image(line_rgb, ox, oy + ry, r_view.screen_w, 1);
    }
#endif
}

static void BlitLores(int ox, int oy, int ow, int oh)
{
#ifdef _FLTK_DISABLED
    // if destination width is odd, we store an extra pixel here
    u8_t line_rgb[(ow + 1) * 3];

    for (int ry = 0; ry < r_view.screen_h; ry++)
    {
        const img_pixel_t *src = r_view.screen + ry * r_view.screen_w;

        u8_t *dest     = line_rgb;
        u8_t *dest_end = line_rgb + ow * 3;

        for (; dest < dest_end; dest += 6, src++)
        {
            IM_DecodePixel(*src, dest[0], dest[1], dest[2]);
            IM_DecodePixel(*src, dest[3], dest[4], dest[5]);
        }

        fl_draw_image(line_rgb, ox, oy + ry * 2, ow, 1);

        if (ry * 2 + 1 < oh)
        {
            fl_draw_image(line_rgb, ox, oy + ry * 2 + 1, ow, 1);
        }
    }
#endif
}

void SW_RenderWorld(int ox, int oy, int ow, int oh)
{
#ifdef _FLTK_DISABLED
    RendInfo rend;

    fl_push_clip(ox, oy, ow, oh);

    rend.Render();

    if (render_high_detail)
        BlitHires(ox, oy, ow, oh);
    else
        BlitLores(ox, oy, ow, oh);

    rend.Highlight(ox, oy);

    fl_pop_clip();
#endif
}

bool SW_QueryPoint(Objid &hl, int qx, int qy)
{
    if (!render_high_detail)
    {
        qx = qx / 2;
        qy = qy / 2;
    }

    RendInfo rend;

    // this runs the renderer, but *no* drawing is done
    rend.Query(qx, qy);

    if (rend.query_map_x != 0)
    {
        edit.map_x = rend.query_map_x;
        edit.map_y = rend.query_map_y;
        edit.map_z = rend.query_map_z;
    }

    if (!rend.query_result.valid())
    {
        // nothing was hit
        return false;
    }

    hl = rend.query_result;
    return true;
}

} // namespace smc