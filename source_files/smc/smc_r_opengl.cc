//------------------------------------------------------------------------
//  3D RENDERING : OPENGL
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2020 Andrew Apted
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

#ifndef NO_OPENGL

#include "smc_main.h"

#include <map>
#include <algorithm>

#ifdef _FLTK_DISABLED
#include "FL/gl.h"
#endif

#include "smc_e_main.h"
#include "smc_e_hover.h"   // PointOnLineSide
#include "smc_e_linedef.h" // LD_RailHeights
#include "smc_m_game.h"
#include "smc_m_bitvec.h"
#include "smc_w_rawdef.h"
#include "smc_w_texture.h"
#include "smc_r_render.h"
#include "smc_r_subdiv.h"

#include "smc_ui_window.h"

namespace smc
{

extern rgb_color_t transparent_col;

extern bool render_high_detail;
extern bool render_lock_gravity;
extern bool render_missing_bright;
extern bool render_unknown_bright;
extern int  render_pixel_aspect;
extern int  render_far_clip;

#ifdef _FLTK_DISABLED
// convert from our coordinate system (looking along +X)
// to OpenGL's coordinate system (looking down -Z).
static GLdouble flip_matrix[16] = {0, 0, -1, 0, -1, 0, 0, 0, 0, +1, 0, 0, 0, 0, 0, +1};
#endif

// The emulation of DOOM lighting here targets a very basic
// version of OpenGL: 1.2 or so.  It works by clipping walls
// and sector triangles against a small set of infinite lines
// at specific distances from the camera.  Once clipped, the
// light level of a primitive is constant, determined from
// the pair of clipping lines it ends up in.

#define LCLIP_NUM 14

static const float light_clip_dists[LCLIP_NUM] = {1280, 640, 428, 320, 256, 216, 176, 144, 120, 104, 88, 72, 60, 40};

static float DoomLightToFloat(int light, float dist)
{
    int map = R_DoomLightingEquation(light, dist);

    int level = (31 - map) * 8 + 7;

    // need to gamma-correct the light level
    if (usegamma > 0)
        level = gammatable[usegamma][level];

    return level / 255.0;
}

struct RendInfo3D
{
  public:
    // we don't test the visibility of sectors directly, instead we
    // mark sectors as visible when any linedef that touches the
    // sector is on-screen.
    bitvec_c seen_sectors;

  public:
    RendInfo3D() : seen_sectors(NumSectors + 1)
    {
    }

    ~RendInfo3D()
    {
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

    static inline int DeltaToX(double iz, float tx)
    {
        int x = int(r_view.aspect_sw * tx * iz);

        x = (x + r_view.screen_w) / 2;

        return x;
    }

    Img_c *FindFlat(const char *fname, byte &r, byte &g, byte &b, bool &fullbright)
    {
        fullbright = false;

        if (is_sky(fname))
        {
            fullbright = true;
#ifdef _FLTK_DISABLED
            glBindTexture(GL_TEXTURE_2D, 0);
#endif

            IM_DecodePixel(Misc_info.sky_color, r, g, b);
            return NULL;
        }

        if (!r_view.texturing)
        {
#ifdef _FLTK_DISABLED
            glBindTexture(GL_TEXTURE_2D, 0);
#endif

            int col;

            // when lighting and no texturing, use a single color
            if (r_view.lighting)
                col = Misc_info.floor_colors[1];
            else
                col = HashedPalColor(fname, Misc_info.floor_colors);

            IM_DecodePixel(col, r, g, b);
            return NULL;
        }

        Img_c *img = W_GetFlat(fname);
        if (!img)
        {
            img        = IM_UnknownFlat();
            fullbright = render_unknown_bright;
        }

        img->bind_gl();

        r = g = b = 255;
        return img;
    }

    Img_c *FindTexture(const char *tname, byte &r, byte &g, byte &b, bool &fullbright)
    {
        fullbright = false;

        if (!r_view.texturing)
        {
#ifdef _FLTK_DISABLED
            glBindTexture(GL_TEXTURE_2D, 0);
#endif

            int col;

            // when lighting and no texturing, use a single color
            if (r_view.lighting)
                col = Misc_info.wall_colors[1];
            else
                col = HashedPalColor(tname, Misc_info.wall_colors);

            IM_DecodePixel(col, r, g, b);
            return NULL;
        }

        Img_c *img;

        if (is_null_tex(tname))
        {
            img        = IM_MissingTex();
            fullbright = render_missing_bright;
        }
        else if (is_special_tex(tname))
        {
            img = IM_SpecialTex();
        }
        else
        {
            img = W_GetTexture(tname);

            if (!img)
            {
                img        = IM_UnknownTex();
                fullbright = render_unknown_bright;
            }
        }

        img->bind_gl();

        r = g = b = 255;
        return img;
    }

    void RawClippedTriangle(float ax, float ay, float az, float atx, float aty, float bx, float by, float bz, float btx,
                            float bty, float cx, float cy, float cz, float ctx, float cty, float r, float g, float b,
                            float level)
    {
#ifdef _FLTK_DISABLED
        glColor3f(level * r, level * g, level * b);

        glBegin(GL_POLYGON);

        glTexCoord2f(atx, aty);
        glVertex3f(ax, ay, az);
        glTexCoord2f(btx, bty);
        glVertex3f(bx, by, bz);
        glTexCoord2f(ctx, cty);
        glVertex3f(cx, cy, cz);

        glEnd();
#endif
    }

    void LightClippedTriangle(double ax, double ay, float az, float atx, float aty, double bx, double by, float bz,
                              float btx, float bty, double cx, double cy, float cz, float ctx, float cty, int clip,
                              float r, float g, float b, int light)
    {
        float level = DoomLightToFloat(light, light_clip_dists[LCLIP_NUM - 1] + 2.0);

        for (; clip < LCLIP_NUM; clip++)
        {
            float cdist = light_clip_dists[clip];

            level = DoomLightToFloat(light, cdist + 2.0);

            // coordinates of an infinite clipping line
            float c_x = r_view.x + r_view.Cos * cdist;
            float c_y = r_view.y + r_view.Sin * cdist;

            // vector of clipping line (if camera is north, this is east)
            float c_dx = r_view.Sin;
            float c_dy = -r_view.Cos;

            // check which side the triangle points are on
            double p1 = (ay - c_y) * c_dx - (ax - c_x) * c_dy;
            double p2 = (by - c_y) * c_dx - (bx - c_x) * c_dy;
            double p3 = (cy - c_y) * c_dx - (cx - c_x) * c_dy;

            int cat1 = (p1 < -0.1) ? -1 : (p1 > 0.1) ? +1 : 0;
            int cat2 = (p2 < -0.1) ? -1 : (p2 > 0.1) ? +1 : 0;
            int cat3 = (p3 < -0.1) ? -1 : (p3 > 0.1) ? +1 : 0;

            // completely on far side?
            if (cat1 >= 0 && cat2 >= 0 && cat3 >= 0)
                break;

            // completely on near side?
            if (cat1 <= 0 && cat2 <= 0 && cat3 <= 0)
                continue;

            // handle cases where partition goes through a vertex AND
            // cuts the opposite edge.
            if (cat1 == 0 || cat2 == 0 || cat3 == 0)
            {
                // rotate triangle so that C is on the partition,
                // and the edge AB is crossing the partition.
                if (cat1 == 0)
                {
                    float _x  = cx;
                    float _y  = cy;
                    float _z  = cz;
                    float _tx = ctx;
                    float _ty = cty;

                    cx  = ax;
                    cy  = ay;
                    cz  = az;
                    ctx = atx;
                    cty = aty;
                    ax  = bx;
                    ay  = by;
                    az  = bz;
                    atx = btx;
                    aty = bty;
                    bx  = _x;
                    by  = _y;
                    bz  = _z;
                    btx = _tx;
                    bty = _ty;

                    p1   = p2;
                    p2   = p3;
                    cat1 = cat2;
                }
                else if (cat2 == 0)
                {
                    float _x  = cx;
                    float _y  = cy;
                    float _z  = cz;
                    float _tx = ctx;
                    float _ty = cty;

                    cx  = bx;
                    cy  = by;
                    cz  = bz;
                    ctx = btx;
                    cty = bty;
                    bx  = ax;
                    by  = ay;
                    bz  = az;
                    btx = atx;
                    bty = aty;
                    ax  = _x;
                    ay  = _y;
                    az  = _z;
                    atx = _tx;
                    aty = _ty;

                    p2   = p1;
                    p1   = p3;
                    cat1 = cat3;
                }

                // compute intersection point
                double along = p1 / (p1 - p2);

                double ix  = ax + (bx - ax) * along;
                double iy  = ay + (by - ay) * along;
                double iz  = az + (bz - az) * along;
                float  itx = atx + (btx - atx) * along;
                float  ity = aty + (bty - aty) * along;

                // draw the piece on FAR side of the clipping line,
                // and keep going with the piece on the NEAR side.
                if (cat1 > 0)
                {
                    RawClippedTriangle(ix, iy, iz, itx, ity, cx, cy, cz, ctx, cty, ax, ay, az, atx, aty, r, g, b,
                                       level);

                    ax  = ix;
                    ay  = iy;
                    az  = iz;
                    atx = itx;
                    aty = ity;
                }
                else
                {
                    RawClippedTriangle(ix, iy, iz, itx, ity, cx, cy, cz, ctx, cty, bx, by, bz, btx, bty, r, g, b,
                                       level);

                    bx  = ix;
                    by  = iy;
                    bz  = iz;
                    btx = itx;
                    bty = ity;
                }

                continue;
            }

            // AT HERE, the partition definitely cuts two edges.
            // The cut produces a triangle piece and a quadrilateral
            // piece.

            int combo = ((cat3 > 0) ? 4 : 0) | ((cat2 > 0) ? 2 : 0) | ((cat1 > 0) ? 1 : 0);

            // rotate triangle so that C is the tip of the triangle piece.
            if (combo == 1 || combo == 6)
            {
                float  _x  = cx;
                float  _y  = cy;
                float  _z  = cz;
                float  _tx = ctx;
                float  _ty = cty;
                double _p  = p3;

                cx  = ax;
                cy  = ay;
                cz  = az;
                ctx = atx;
                cty = aty;
                p3  = p1;
                ax  = bx;
                ay  = by;
                az  = bz;
                atx = btx;
                aty = bty;
                p1  = p2;
                bx  = _x;
                by  = _y;
                bz  = _z;
                btx = _tx;
                bty = _ty;
                p2  = _p;
            }
            else if (combo == 2 || combo == 5)
            {
                float  _x  = cx;
                float  _y  = cy;
                float  _z  = cz;
                float  _tx = ctx;
                float  _ty = cty;
                double _p  = p3;

                cx  = bx;
                cy  = by;
                cz  = bz;
                ctx = btx;
                cty = bty;
                p3  = p2;
                bx  = ax;
                by  = ay;
                bz  = az;
                btx = atx;
                bty = aty;
                p2  = p1;
                ax  = _x;
                ay  = _y;
                az  = _z;
                atx = _tx;
                aty = _ty;
                p1  = _p;
            }

            // compute intersection points
            double ac_along = p1 / (p1 - p3);
            double bc_along = p2 / (p2 - p3);

            double ac_x = ax + (cx - ax) * ac_along;
            double ac_y = ay + (cy - ay) * ac_along;
            double ac_z = az + (cz - az) * ac_along;

            double bc_x = bx + (cx - bx) * bc_along;
            double bc_y = by + (cy - by) * bc_along;
            double bc_z = bz + (cz - bz) * bc_along;

            float ac_tx = atx + (ctx - atx) * ac_along;
            float ac_ty = aty + (cty - aty) * ac_along;
            float bc_tx = btx + (ctx - btx) * bc_along;
            float bc_ty = bty + (cty - bty) * bc_along;

            // handle cases where triangle piece is on NEAR side.
            if (combo == 3 || combo == 5 || combo == 6)
            {
                RawClippedTriangle(ax, ay, az, atx, aty, bx, by, bz, btx, bty, ac_x, ac_y, ac_z, ac_tx, ac_ty, r, g, b,
                                   level);

                RawClippedTriangle(bx, by, bz, btx, bty, bc_x, bc_y, bc_z, bc_tx, bc_ty, ac_x, ac_y, ac_z, ac_tx, ac_ty,
                                   r, g, b, level);

                ax  = ac_x;
                ay  = ac_y;
                az  = ac_z;
                atx = ac_tx;
                aty = ac_ty;
                bx  = bc_x;
                by  = bc_y;
                bz  = bc_z;
                btx = bc_tx;
                bty = bc_ty;

                continue;
            }

            // handle cases where triangle piece is on FAR side.
            // these cases require recursion to deal with the
            // quadrilateral on the near side.
            {
                RawClippedTriangle(cx, cy, cz, ctx, cty, ac_x, ac_y, ac_z, ac_tx, ac_ty, bc_x, bc_y, bc_z, bc_tx, bc_ty,
                                   r, g, b, level);

                // recurse!
                LightClippedTriangle(ax, ay, az, atx, aty, ac_x, ac_y, ac_z, ac_tx, ac_ty, bc_x, bc_y, bc_z, bc_tx,
                                     bc_ty, (clip + 1), r, g, b, light);

                cx  = bc_x;
                cy  = bc_y;
                cz  = bc_z;
                ctx = bc_tx;
                cty = bc_ty;
            }
        }

        RawClippedTriangle(ax, ay, az, atx, aty, bx, by, bz, btx, bty, cx, cy, cz, ctx, cty, r, g, b, level);
    }

    void RawClippedQuad(float x1, float y1, const slope_plane_c *p1, float x2, float y2, const slope_plane_c *p2,
                        float tx1, float tx2, float tex_top, float tex_scale, char where, float r, float g, float b,
                        float level)
    {
        float za1 = p1->SlopeZ(x1, y1);
        float za2 = p2->SlopeZ(x1, y1);
        float zb1 = p1->SlopeZ(x2, y2);
        float zb2 = p2->SlopeZ(x2, y2);

        // check heights [ for sides of slopes ]
        if (za2 <= za1 && zb2 <= zb1)
        {
            return;
        }
        else if (za2 < za1)
        {
            if (where == 'U')
                za2 = za1;
            else
                za1 = za2;
        }
        else if (zb2 < zb1)
        {
            if (where == 'U')
                zb2 = zb1;
            else
                zb1 = zb2;
        }

#ifdef _FLTK_DISABLED
        glColor3f(level * r, level * g, level * b);

        glBegin(GL_QUADS);

        glTexCoord2f(tx1, (za1 - tex_top) * tex_scale);
        glVertex3f(x1, y1, za1);
        glTexCoord2f(tx1, (za2 - tex_top) * tex_scale);
        glVertex3f(x1, y1, za2);
        glTexCoord2f(tx2, (zb2 - tex_top) * tex_scale);
        glVertex3f(x2, y2, zb2);
        glTexCoord2f(tx2, (zb1 - tex_top) * tex_scale);
        glVertex3f(x2, y2, zb1);

        glEnd();
#endif
    }

    void LightClippedQuad(double x1, double y1, const slope_plane_c *p1, double x2, double y2, const slope_plane_c *p2,
                          float tx1, float tx2, float tex_top, float tex_scale, char where, float r, float g, float b,
                          int light)
    {
        float level = DoomLightToFloat(light, light_clip_dists[LCLIP_NUM - 1] + 2.0);

        for (int clip = 0; clip < LCLIP_NUM; clip++)
        {
            float cdist = light_clip_dists[clip];

            level = DoomLightToFloat(light, cdist + 2.0);

            // coordinates of an infinite clipping line
            float c_x = r_view.x + r_view.Cos * cdist;
            float c_y = r_view.y + r_view.Sin * cdist;

            // vector of clipping line (if camera is north, this is east)
            float c_dx = r_view.Sin;
            float c_dy = -r_view.Cos;

            // check which side the start/end point is on
            double n1 = (y1 - c_y) * c_dx - (x1 - c_x) * c_dy;
            double n2 = (y2 - c_y) * c_dx - (x2 - c_x) * c_dy;

            int cat1 = (n1 < -0.1) ? -1 : (n1 > 0.1) ? +1 : 0;
            int cat2 = (n2 < -0.1) ? -1 : (n2 > 0.1) ? +1 : 0;

            // completely on far side?
            if (cat1 >= 0 && cat2 >= 0)
                break;

            // does it cross the partition?
            if ((cat1 < 0 && cat2 > 0) || (cat1 > 0 && cat2 < 0))
            {
                // compute intersection point
                double along = n1 / (n1 - n2);

                double ix = x1 + (x2 - x1) * along;
                double iy = y1 + (y2 - y1) * along;

                float itx = tx1 + (tx2 - tx1) * along;

                // draw the piece on FAR side of the clipping line,
                // and keep going with the piece on the NEAR side.
                if (cat2 > 0)
                {
                    RawClippedQuad(ix, iy, p1, x2, y2, p2, itx, tx2, tex_top, tex_scale, where, r, g, b, level);

                    x2  = ix;
                    y2  = iy;
                    tx2 = itx;
                }
                else
                {
                    RawClippedQuad(x1, y1, p1, ix, iy, p2, tx1, itx, tex_top, tex_scale, where, r, g, b, level);

                    x1  = ix;
                    y1  = iy;
                    tx1 = itx;
                }
            }
        }

        RawClippedQuad(x1, y1, p1, x2, y2, p2, tx1, tx2, tex_top, tex_scale, where, r, g, b, level);
    }

    inline bool IsPolygonClipped(const sector_polygon_t *poly)
    {
        int p;
        for (p = 0; p < poly->count; p++)
        {
            float px = poly->mx[p];
            float py = poly->my[p];

            double dist = (py - r_view.y) * r_view.Sin + (px - r_view.x) * r_view.Cos;

            if (dist < render_far_clip + 1)
                break;
        }

        // whole polygon was beyond the far clip?
        if (p == poly->count)
            return true;

        return false;
    }

    void DrawSectorPolygons(const Sector *sec, sector_subdivision_c *subdiv, const slope_plane_c *plane, int znormal,
                            float z, const char *fname)
    {
        bool is_slope = plane && plane->sloped;

        // check if camera is behind plane
        if (!is_slope)
        {
            if (znormal > 0 && r_view.z < z)
                return;
            if (znormal < 0 && r_view.z > z)
                return;
        }

        byte   r0, g0, b0;
        bool   fullbright;
        Img_c *img = FindFlat(fname, r0, g0, b0, fullbright);

        float r = r0 / 255.0;
        float g = g0 / 255.0;
        float b = b0 / 255.0;

        for (unsigned int i = 0; i < subdiv->polygons.size(); i++)
        {
            const sector_polygon_t *poly = &subdiv->polygons[i];

            // not sure this is worth it, just let OpenGL clip it
#if 0
			if (IsPolygonClipped(poly))
				continue;
#endif
            if (r_view.lighting && !fullbright)
            {
                float ax  = poly->mx[0];
                float ay  = poly->my[0];
                float az  = plane ? plane->SlopeZ(ax, ay) : z;
                float atx = ax / 64.0; // see note below
                float aty = ay / 64.0;

                float bx  = poly->mx[1];
                float by  = poly->my[1];
                float bz  = plane ? plane->SlopeZ(bx, by) : z;
                float btx = bx / 64.0; // see note below
                float bty = by / 64.0;

                float cx  = poly->mx[2];
                float cy  = poly->my[2];
                float cz  = plane ? plane->SlopeZ(cx, cy) : z;
                float ctx = cx / 64.0;
                float cty = cy / 64.0;

                LightClippedTriangle(ax, ay, az, atx, aty, bx, by, bz, btx, bty, cx, cy, cz, ctx, cty, 0, r, g, b,
                                     sec->light);

                if (poly->count == 4)
                {
                    float dx  = poly->mx[3];
                    float dy  = poly->my[3];
                    float dz  = plane ? plane->SlopeZ(dx, dy) : z;
                    float dtx = dx / 64.0;
                    float dty = dy / 64.0;

                    LightClippedTriangle(ax, ay, az, atx, aty, cx, cy, cz, ctx, cty, dx, dy, dz, dtx, dty, 0, r, g, b,
                                         sec->light);
                }
            }
            else
            {
#ifdef _FLTK_DISABLED
                glColor3f(r, g, b);
                glBegin(GL_POLYGON);

                for (int p = 0; p < poly->count; p++)
                {
                    float px = poly->mx[p];
                    float py = poly->my[p];
                    float pz = plane ? plane->SlopeZ(px, py) : z;

                    if (img)
                    {
                        // this logic follows ZDoom, which scales large flats to
                        // occupy a 64x64 unit area.  I presume wall textures
                        // used on floors or ceilings is the same....
                        glTexCoord2f(px / 64.0, py / 64.0);
                    }

                    glVertex3f(px, py, pz);
                }

                glEnd();
#endif
            }
        }
    }

    // the "where" parameter can be:
    //   - 'W' for one-sided wall
    //   - 'L' for lower
    //   - 'U' for upper
    //   - 'E' for extrafloor side
    void DrawSide(char where, const LineDef *ld, const SideDef *sd, const char *texname, const Sector *front,
                  const Sector *back, bool sky_upper, float ld_length, float x1, float y1, const slope_plane_c *p1,
                  float x2, float y2, const slope_plane_c *p2)
    {
        byte   r, g, b;
        bool   fullbright = true;
        Img_c *img        = NULL;

        if (sky_upper && where == 'U')
        {
#ifdef _FLTK_DISABLED
            glBindTexture(GL_TEXTURE_2D, 0);
#endif
            IM_DecodePixel(Misc_info.sky_color, r, g, b);
        }
        else
        {
            img = FindTexture(texname, r, g, b, fullbright);
        }

        // compute texture coords
        float tx1 = 0.0;
        float tx2 = 1.0;

        float tex_top   = 0;
        float tex_scale = 1.0 / 128.0;

        if (img)
        {
            float img_w  = img->width();
            float img_tw = RoundPOW2(img_w);

            float img_h  = img->height();
            float img_th = RoundPOW2(img_h);

            tx1 = 0;
            tx2 = tx1 + ld_length;

            tex_top = front->ceilh;

            if (where == 'W' && (ld->flags & MLF_LowerUnpegged))
            {
                tex_top = front->floorh + img_h;
            }

            if (where == 'L')
            {
                if (0 == (ld->flags & MLF_LowerUnpegged))
                {
                    tex_top = back->floorh;
                }
                else
                {
                    // an unpegged lower will align with a normal 1S wall,
                    // unless both front/back ceilings are sky....

                    tex_top = sky_upper ? back->ceilh : front->ceilh;
                }
            }

            if (where == 'U' && (0 == (ld->flags & MLF_UpperUnpegged)))
            {
                tex_top = back->ceilh + img_h;
            }

            tx1 = (tx1 + sd->x_offset) / img_tw;
            tx2 = (tx2 + sd->x_offset) / img_tw;

            tex_top += (img_th - img_h);
            tex_top += sd->y_offset;

            tex_scale = 1.0 / img_th;
        }

#ifdef _FLTK_DISABLED
        glDisable(GL_ALPHA_TEST);
#endif

        double r0 = (double)r / 255.0;
        double g0 = (double)g / 255.0;
        double b0 = (double)b / 255.0;

        if (r_view.lighting && !fullbright)
        {
            int light = front->light;

            // add "fake constrast" for axis-aligned walls
            if (ld->IsVertical())
                light += 16;
            else if (ld->IsHorizontal())
                light -= 16;

            LightClippedQuad(x1, y1, p1, x2, y2, p2, tx1, tx2, tex_top, tex_scale, where, r0, g0, b0, light);
        }
        else
        {
            RawClippedQuad(x1, y1, p1, x2, y2, p2, tx1, tx2, tex_top, tex_scale, where, r0, g0, b0, 1.0);
        }
    }

    void DrawMidMasker(const LineDef *ld, const SideDef *sd, const Sector *front, const Sector *back, bool sky_upper,
                       float ld_length, float x1, float y1, float x2, float y2)
    {
        byte   r, g, b;
        bool   fullbright;
        Img_c *img;

        img = FindTexture(sd->MidTex(), r, g, b, fullbright);
        if (img == NULL)
            return;

        float img_w  = img->width();
        float img_tw = RoundPOW2(img_w);

        float img_h  = img->height();
        float img_th = RoundPOW2(img_h);

        // compute Z coords and texture coords
        float z1 = MAX(front->floorh, back->floorh);
        float z2 = MIN(front->ceilh, back->ceilh);

        if (z2 <= z1)
            return;

        float tx1 = 0.0;
        float tx2 = tx1 + ld_length;

        tx1 = (tx1 + sd->x_offset) / img_tw;
        tx2 = (tx2 + sd->x_offset) / img_tw;

        if (ld->flags & MLF_LowerUnpegged)
        {
            z1 = z1 + sd->y_offset;
            z2 = z1 + img_h;
        }
        else
        {
            z2 = z2 + sd->y_offset;
            z1 = z2 - img_h;
        }

#ifdef _FLTK_DISABLED
        glEnable(GL_ALPHA_TEST);
#endif

        slope_plane_c p1;
        p1.Init(z1);
        slope_plane_c p2;
        p2.Init(z2);

        double r0 = (double)r / 255.0;
        double g0 = (double)g / 255.0;
        double b0 = (double)b / 255.0;

        float tex_scale = 1.0 / img_th;

        if (r_view.lighting && !fullbright)
        {
            int light = sd->SecRef()->light;

            // add "fake constrast" for axis-aligned walls
            if (ld->IsVertical())
                light += 16;
            else if (ld->IsHorizontal())
                light -= 16;

            LightClippedQuad(x1, y1, &p1, x2, y2, &p2, tx1, tx2, z1, tex_scale, 'R', r0, g0, b0, light);
        }
        else
        {
            RawClippedQuad(x1, y1, &p1, x2, y2, &p2, tx1, tx2, z1, tex_scale, 'R', r0, g0, b0, 1.0);
        }
    }

    void DrawLine(int ld_index)
    {
        const LineDef *ld = LineDefs[ld_index];

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

        // too far away?
        if (MIN(ty1, ty2) > render_far_clip)
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
        const SideDef *sd = (side == SIDE_LEFT) ? ld->Left() : ld->Right();

        if (!sd)
            return;

        if (side == SIDE_LEFT)
        {
            float tmp = angle1;
            angle1    = angle2;
            angle2    = tmp;
        }

        // clip angles to view volume

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
        int sx2 = AngleToX(angle2);

        if (sx1 > sx2)
            return;

        // compute distance from eye to wall
        float wdx = x2 - x1;
        float wdy = y2 - y1;

        float wlen = sqrt(wdx * wdx + wdy * wdy);
        float dist = fabs((y1 * wdx / wlen) - (x1 * wdy / wlen));

        if (dist < 0.01)
            return;

        bool self_ref = false;
        if (ld->Left() && ld->Right() && ld->Left()->sector == ld->Right()->sector)
            self_ref = true;

        // mark sectors to be drawn
        // [ this method means we don't need to check visibility of sectors ]
        if (!self_ref)
        {
            if (ld->Left() && is_sector(ld->Left()->sector))
                seen_sectors.set(ld->Left()->sector);

            if (ld->Right() && is_sector(ld->Right()->sector))
                seen_sectors.set(ld->Right()->sector);
        }

        /* actually draw it... */

        x1 = ld->Start()->x();
        y1 = ld->Start()->y();
        x2 = ld->End()->x();
        y2 = ld->End()->y();

        if (side == SIDE_LEFT)
        {
            std::swap(x1, x2);
            std::swap(y1, y2);
        }

        float ld_len = hypotf(x2 - x1, y2 - y1);

        const Sector *front = sd ? sd->SecRef() : NULL;

        bool sky_front = is_sky(front->CeilTex());
        bool sky_upper = false;

        if (ld->OneSided())
        {
            sector_3dfloors_c *ex = Subdiv_3DFloorsForSector(sd->sector);

            DrawSide('W', ld, sd, sd->MidTex(), front, NULL, false, ld_len, x1, y1, &ex->f_plane, x2, y2, &ex->c_plane);
        }
        else
        {
            const SideDef *sd_back = (side == SIDE_LEFT) ? ld->Right() : ld->Left();
            const Sector  *back    = sd_back ? sd_back->SecRef() : NULL;

            sky_upper = sky_front && is_sky(back->CeilTex());

            // check for BOOM 242 invisible platforms
            bool               invis_back = false;
            const Sector      *dummy      = NULL;
            sector_3dfloors_c *b_ex       = Subdiv_3DFloorsForSector(sd_back->sector);
            if (b_ex->heightsec >= 0)
            {
                dummy = Sectors[b_ex->heightsec];
                if (dummy->floorh < back->floorh)
                    invis_back = true;
            }

            sector_3dfloors_c *f_ex     = Subdiv_3DFloorsForSector(sd->sector);
            slope_plane_c     *f_floorp = &f_ex->f_plane;
            slope_plane_c      dummy_fp;
            if (f_ex->heightsec >= 0)
            {
                dummy = Sectors[f_ex->heightsec];
                if (dummy->floorh < front->floorh)
                {
                    dummy_fp.Init(dummy->floorh);
                    f_floorp = &dummy_fp;
                }
            }

            // we skip height check for slopes, but RawClippedQuad will handle it
            bool f_sloped = f_ex->f_plane.sloped || b_ex->f_plane.sloped;
            bool c_sloped = f_ex->c_plane.sloped || b_ex->c_plane.sloped;

            // lower part
            if ((back->floorh > front->floorh || f_sloped) && !self_ref && !invis_back)
                DrawSide('L', ld, sd, sd->LowerTex(), front, back, sky_upper, ld_len, x1, y1, f_floorp, x2, y2,
                         &b_ex->f_plane);

            // upper part
            if ((back->ceilh < front->ceilh || c_sloped) && !self_ref && !sky_upper)
                DrawSide('U', ld, sd, sd->UpperTex(), front, back, sky_upper, ld_len, x1, y1, &b_ex->c_plane, x2, y2,
                         &f_ex->c_plane);

            // railing tex
            if (!is_null_tex(sd->MidTex()) && r_view.texturing)
                DrawMidMasker(ld, sd, front, back, sky_upper, ld_len, x1, y1, x2, y2);

            // draw sides of extrafloors
            if (front->tag != back->tag)
            {
                for (size_t k = 0; k < b_ex->floors.size(); k++)
                {
                    const extrafloor_c &EF    = b_ex->floors[k];
                    const SideDef      *ef_sd = SideDefs[EF.sd];
                    const Sector       *dummy = Sectors[ef_sd->sector];

                    if (EF.flags & (EXFL_TOP | EXFL_BOTTOM))
                        continue;

                    int top_h    = dummy->ceilh;
                    int bottom_h = dummy->floorh;

                    if (EF.flags & EXFL_VAVOOM)
                        std::swap(top_h, bottom_h);

                    if (top_h <= bottom_h)
                        continue;

                    const char *tex = "-";
                    if (EF.flags & EXFL_UPPER)
                        tex = sd->UpperTex();
                    else if (EF.flags & EXFL_LOWER)
                        tex = sd->LowerTex();
                    else
                        tex = ef_sd->MidTex();

                    slope_plane_c p1;
                    p1.Init(bottom_h);
                    slope_plane_c p2;
                    p2.Init(top_h);

                    DrawSide('E', ld, sd, tex, front, back, false, ld_len, x1, y1, &p1, x2, y2, &p2);
                }
            }
        }

        /* EMULATE VANILLA SKIES */

        // we don't draw the skies as polygons, nor do we draw the upper
        // of a linedef which has sky on both sides.  instead we draw a
        // very tall quad (like a wall part) above a solid wall where it
        // meets a sky sector.

        if (sky_front && !sky_upper)
        {
            slope_plane_c p1;
            p1.Init(front->ceilh);
            slope_plane_c p2;
            p2.Init(front->ceilh + 16384.0);

            DrawSide('U', ld, sd, "-", front, NULL, true /* sky_upper */, ld_len, x1, y1, &p1, x2, y2, &p2);
        }
    }

    void DrawSector(int sec_index)
    {
        sector_subdivision_c *subdiv = Subdiv_PolygonsForSector(sec_index);

        if (!subdiv)
            return;

        const Sector *sec   = Sectors[sec_index];
        const Sector *dummy = NULL;

        sector_3dfloors_c *exfloor = Subdiv_3DFloorsForSector(sec_index);

#ifdef _FLTK_DISABLED
        glColor3f(1, 1, 1);
#endif

        // support for BOOM's 242 "transfer heights" line type
        if (exfloor->heightsec >= 0)
        {
            dummy = Sectors[exfloor->heightsec];

            if (dummy->floorh > sec->floorh && r_view.z < dummy->floorh)
            {
                // space C : underwater
                DrawSectorPolygons(sec, subdiv, NULL, -1, dummy->floorh, dummy->CeilTex());
                DrawSectorPolygons(sec, subdiv, NULL, +1, sec->floorh, dummy->FloorTex());

                // this helps the view to not look weird when clipping around
                if (dummy->ceilh > sec->floorh)
                    DrawSectorPolygons(sec, subdiv, NULL, -1, dummy->ceilh, sec->CeilTex());
            }
            else if (dummy->ceilh < sec->ceilh && r_view.z > dummy->ceilh)
            {
                // space A : head over ceiling
                DrawSectorPolygons(sec, subdiv, NULL, -1, dummy->ceilh, dummy->FloorTex());
                DrawSectorPolygons(sec, subdiv, NULL, -1, sec->ceilh, dummy->CeilTex());

                if (dummy->floorh < sec->ceilh)
                    DrawSectorPolygons(sec, subdiv, NULL, +1, dummy->floorh, sec->FloorTex());
            }
            else if (dummy->floorh < sec->floorh)
            {
                // invisible platform
                DrawSectorPolygons(sec, subdiv, NULL, +1, dummy->floorh, sec->FloorTex());

                if (!is_sky(sec->CeilTex()))
                    DrawSectorPolygons(sec, subdiv, NULL, -1, dummy->ceilh, sec->CeilTex());
            }
            else
            {
                // space B : normal
                DrawSectorPolygons(sec, subdiv, NULL, +1, dummy->floorh, sec->FloorTex());

                if (!is_sky(sec->CeilTex()))
                    DrawSectorPolygons(sec, subdiv, NULL, -1, dummy->ceilh, sec->CeilTex());
            }
        }
        else
        {

            // normal sector
            DrawSectorPolygons(sec, subdiv, &exfloor->f_plane, +1, sec->floorh, sec->FloorTex());

            if (!is_sky(sec->CeilTex()))
                DrawSectorPolygons(sec, subdiv, &exfloor->c_plane, -1, sec->ceilh, sec->CeilTex());
        }

        // draw planes of 3D floors
        for (size_t k = 0; k < exfloor->floors.size(); k++)
        {
            const extrafloor_c &EF    = exfloor->floors[k];
            const Sector       *dummy = Sectors[SideDefs[EF.sd]->sector];

            // TODO: supporting translucent surfaces is non-trivial and needs
            //       to be done in separate pass with a depth sort.
            bool is_trans = (EF.flags & EXFL_TRANSLUC) != 0;
            (void)is_trans;

            int top_h    = dummy->ceilh;
            int bottom_h = dummy->floorh;

            const char *top_tex    = dummy->CeilTex();
            const char *bottom_tex = dummy->FloorTex();

            if (EF.flags & EXFL_TOP)
                bottom_h = top_h;
            else if (EF.flags & EXFL_BOTTOM)
                top_h = bottom_h;
            else if (EF.flags & EXFL_VAVOOM)
            {
                std::swap(top_h, bottom_h);
                std::swap(top_tex, bottom_tex);
            }

            DrawSectorPolygons(sec, subdiv, NULL, +1, top_h, top_tex);
            DrawSectorPolygons(sec, subdiv, NULL, -1, bottom_h, bottom_tex);
        }
    }

    void DrawThing(int th_index)
    {
        Thing *th = Things[th_index];

        const thingtype_t *info = M_GetThingType(th->type);

        // project sprite to check if it is off-screen

        float x = th->x() - r_view.x;
        float y = th->y() - r_view.y;

        float tx = x * r_view.Sin - y * r_view.Cos;
        float ty = x * r_view.Cos + y * r_view.Sin;

        // sprite is complete behind viewplane?
        if (ty < 4)
            return;

        if (render_far_clip > 0 && ty > render_far_clip)
            return;

        bool fullbright = false;
        if (info->flags & THINGDEF_LIT)
            fullbright = true;

        float scale = info->scale;

        Img_c *img = W_GetSprite(th->type);
        if (!img)
        {
            img        = IM_UnknownSprite();
            fullbright = true;
            scale      = 0.33;
        }

        float scale_w = img->width() * scale;
        float scale_h = img->height() * scale;

        float tx1 = tx - scale_w * 0.5;
        float tx2 = tx + scale_w * 0.5;
        float ty1, ty2;

        double iz = 1 / ty;

        int sx1 = DeltaToX(iz, tx1);
        int sx2 = DeltaToX(iz, tx2);

        if (sx2 < 0 || sx1 > r_view.screen_w)
            return;

        // sprite is potentially visible, so draw it

        // choose X/Y coordinates so quad faces the camera
        float x1 = th->x() - r_view.Sin * scale_w * 0.5;
        float y1 = th->y() + r_view.Cos * scale_w * 0.5;

        float x2 = th->x() + r_view.Sin * scale_w * 0.5;
        float y2 = th->y() - r_view.Cos * scale_w * 0.5;

        int sec_num = r_view.thing_sectors[th_index];

        float z1, z2;

        if (info->flags & THINGDEF_CEIL)
        {
            // IOANCH 9/2015: add thing z (for Hexen format)
            z2 = (is_sector(sec_num) ? Sectors[sec_num]->ceilh : 192) - th->h();
            z1 = z2 - scale_h;
        }
        else
        {
            z1 = (is_sector(sec_num) ? Sectors[sec_num]->floorh : 0) + th->h();
            z2 = z1 + scale_h;
        }

        // bind the sprite image (upload it to OpenGL if needed)
        img->bind_gl();

        // choose texture coords based on image size
        tx1 = 0.0;
        ty1 = 0.0;
        tx2 = (float)img->width() / (float)RoundPOW2(img->width());
        ty2 = (float)img->height() / (float)RoundPOW2(img->height());

        // lighting
        float L = 1.0;

        if (r_view.lighting && !fullbright)
        {
            int light = is_sector(sec_num) ? Sectors[sec_num]->light : 255;

            L = DoomLightToFloat(light, ty /* dist */);
        }

#ifdef _FLTK_DISABLED
        glColor3f(L, L, L);

        glBegin(GL_QUADS);

        glTexCoord2f(tx1, ty1);
        glVertex3f(x1, y1, z1);
        glTexCoord2f(tx1, ty2);
        glVertex3f(x1, y1, z2);
        glTexCoord2f(tx2, ty2);
        glVertex3f(x2, y2, z2);
        glTexCoord2f(tx2, ty1);
        glVertex3f(x2, y2, z1);

        glEnd();
#endif
    }

    void HighlightLine(int ld_index, int part)
    {
        const LineDef *L = LineDefs[ld_index];

        int side = (part & PART_LF_ALL) ? SIDE_LEFT : SIDE_RIGHT;

        const SideDef *sd = (side < 0) ? L->Left() : L->Right();
        if (sd == NULL)
            return;

        float x1 = L->Start()->x();
        float y1 = L->Start()->y();
        float x2 = L->End()->x();
        float y2 = L->End()->y();

        // check that this side is facing the camera
        int cam_side = PointOnLineSide(r_view.x, r_view.y, x1, y1, x2, y2);
        if (cam_side != side)
            return;

        const SideDef *sd_back = (side < 0) ? L->Right() : L->Left();

        const Sector *front = sd->SecRef();
        const Sector *back  = sd_back ? sd_back->SecRef() : NULL;

        float z1, z2;

        if (L->TwoSided())
        {
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
                int zi1, zi2;

                if (!LD_RailHeights(zi1, zi2, L, sd, front, back))
                    return;

                z1 = zi1;
                z2 = zi2;
            }
        }
        else // one-sided line
        {
            if (0 == (part & (PART_RT_LOWER | PART_LF_LOWER)))
                return;

            z1 = front->floorh;
            z2 = front->ceilh;
        }

#ifdef _FLTK_DISABLED
        glBegin(GL_LINE_LOOP);

        glVertex3f(x1, y1, z1);
        glVertex3f(x1, y1, z2);
        glVertex3f(x2, y2, z2);
        glVertex3f(x2, y2, z1);

        glEnd();
#endif
    }

    void HighlightSector(int sec_index, int part)
    {
        const Sector *sec = Sectors[sec_index];

        float z = (part == PART_CEIL) ? sec->ceilh : sec->floorh;

        // are we dragging this surface?
        if (edit.action == ACT_DRAG &&
            (!edit.dragged.valid() ||
             (edit.dragged.num == sec_index && (edit.dragged.parts == 0 || (edit.dragged.parts & part)))))
        {
            z = z + edit.drag_sector_dz;
        }
        else
        {
            // check that plane faces the camera
            if (part == PART_FLOOR && (r_view.z < z + 0.2))
                return;
            if (part == PART_CEIL && (r_view.z > z - 0.2))
                return;
        }

        for (int n = 0; n < NumLineDefs; n++)
        {
            const LineDef *L = LineDefs[n];

            if (L->TouchesSector(sec_index))
            {
                float x1 = L->Start()->x();
                float y1 = L->Start()->y();
                float x2 = L->End()->x();
                float y2 = L->End()->y();

#ifdef _FLTK_DISABLED
                glBegin(GL_LINE_STRIP);
                glVertex3f(x1, y1, z);
                glVertex3f(x2, y2, z);
                glEnd();
#endif
            }
        }
    }

    void HighlightThing(int th_index)
    {
        Thing *th = Things[th_index];
        float  tx = th->x();
        float  ty = th->y();

        float drag_dz = 0;

        if (edit.action == ACT_DRAG && (!edit.dragged.valid() || edit.dragged.num == th_index))
        {
            tx += (edit.drag_cur_x - edit.drag_start_x);
            ty += (edit.drag_cur_y - edit.drag_start_y);

            drag_dz = edit.drag_cur_z - edit.drag_start_z;
        }

        const thingtype_t *info = M_GetThingType(th->type);

        float scale = info->scale;

        Img_c *img = W_GetSprite(th->type);
        if (!img)
        {
            img   = IM_UnknownSprite();
            scale = 0.33;
        }

        float scale_w = img->width() * scale;
        float scale_h = img->height() * scale;

        // choose X/Y coordinates so quad faces the camera
        float x1 = tx - r_view.Sin * scale_w * 0.5;
        float y1 = ty + r_view.Cos * scale_w * 0.5;
        float x2 = tx + r_view.Sin * scale_w * 0.5;
        float y2 = ty - r_view.Cos * scale_w * 0.5;

        int sec_num = r_view.thing_sectors[th_index];

        float z1, z2;

        if (info->flags & THINGDEF_CEIL)
        {
            // IOANCH 9/2015: add thing z (for Hexen format)
            z2 = (is_sector(sec_num) ? Sectors[sec_num]->ceilh : 192) - th->h();
            z1 = z2 - scale_h;
        }
        else
        {
            z1 = (is_sector(sec_num) ? Sectors[sec_num]->floorh : 0) + th->h();
            z2 = z1 + scale_h;
        }

        z1 += drag_dz;
        z2 += drag_dz;
#ifdef _FLTK_DISABLED
        glBegin(GL_LINE_LOOP);

        glVertex3f(x1, y1, z1);
        glVertex3f(x1, y1, z2);
        glVertex3f(x2, y2, z2);
        glVertex3f(x2, y2, z1);

        glEnd();
#endif
    }

    void HighlightObject(Objid &obj)
    {
        if (!obj.valid())
            return;

        if (obj.type == OBJ_THINGS)
        {
            HighlightThing(obj.num);
        }
        else if (obj.type == OBJ_SECTORS)
        {
            if (obj.parts == 0 || (obj.parts & PART_FLOOR))
                HighlightSector(obj.num, PART_FLOOR);

            if (obj.parts == 0 || (obj.parts & PART_CEIL))
                HighlightSector(obj.num, PART_CEIL);
        }
        else if (obj.type == OBJ_LINEDEFS)
        {
            /* right side */
            if (obj.parts == 0 || (obj.parts & PART_RT_LOWER))
                HighlightLine(obj.num, PART_RT_LOWER);

            if (obj.parts == 0 || (obj.parts & PART_RT_UPPER))
                HighlightLine(obj.num, PART_RT_UPPER);

            if (obj.parts & PART_RT_RAIL)
                HighlightLine(obj.num, PART_RT_RAIL);

            /* left side */
            if (obj.parts == 0 || (obj.parts & PART_LF_LOWER))
                HighlightLine(obj.num, PART_LF_LOWER);

            if (obj.parts == 0 || (obj.parts & PART_LF_UPPER))
                HighlightLine(obj.num, PART_LF_UPPER);

            if (obj.parts & PART_LF_RAIL)
                HighlightLine(obj.num, PART_LF_RAIL);
        }
    }

    void Highlight()
    {
#ifdef _FLTK_DISABLED
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_ALPHA_TEST);

        glLineWidth(2);
#endif

        /* do the selection */

        bool saw_hl = false;

        for (sel_iter_c it(edit.Selected); !it.done(); it.next())
        {
            if (edit.highlight.valid() && *it == edit.highlight.num)
            {
                saw_hl = true;

                // can skip drawing twice for things, but not other stuff
                if (edit.mode == OBJ_THINGS)
                    continue;
            }

            byte parts = edit.Selected->get_ext(*it);

            if (parts > 1)
            {
#ifdef _FLTK_DISABLED
                gl_color(SEL3D_COL);
#endif
            }
            else
            {
#ifdef _FLTK_DISABLED
                gl_color(SEL_COL);
#endif
                parts = 0;
            }

            Objid obj(edit.mode, *it, parts & ~1);

            HighlightObject(obj);
        }

        /* do the highlight */
#ifdef _FLTK_DISABLED
        gl_color(saw_hl ? HI_AND_SEL_COL : HI_COL);
#endif

        if (edit.action == ACT_DRAG && edit.dragged.valid())
            HighlightObject(edit.dragged);
        else
            HighlightObject(edit.highlight);

#ifdef _FLTK_DISABLED
        glLineWidth(1);
#endif
    }

    void MarkCameraSector()
    {
        Objid obj;
        GetNearObject(obj, OBJ_SECTORS, r_view.x, r_view.y);

        if (obj.valid())
            seen_sectors.set(obj.num);
    }

    void Render()
    {
        // always draw the sector the camera is in
        MarkCameraSector();

        for (int i = 0; i < NumLineDefs; i++)
            DrawLine(i);

#ifdef _FLTK_DISABLED
        glDisable(GL_ALPHA_TEST);
#endif
        for (int s = 0; s < NumSectors; s++)
            if (seen_sectors.get(s))
                DrawSector(s);

#ifdef _FLTK_DISABLED
        glEnable(GL_ALPHA_TEST);
#endif

        if (r_view.sprites)
            for (int t = 0; t < NumThings; t++)
                DrawThing(t);
    }

    void Begin(int ow, int oh)
    {
#ifdef _FLTK_DISABLED
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_TEXTURE_2D);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_ALPHA_TEST);

        glDisable(GL_CULL_FACE);

        glAlphaFunc(GL_GREATER, 0.5);

        // setup projection

        // Note: this crud is a workaround for retina displays on MacOS
        Fl::use_high_res_GL(true);
        int pix = I_ROUND(main_win->canvas->pixels_per_unit());
        Fl::use_high_res_GL(false);

        glViewport(0, 0, ow * pix, oh * pix);

        GLdouble angle = r_view.angle * 180.0 / M_PI;

        // the model-view matrix does three things:
        //   1. translates X/Y/Z by view position
        //   2. rotates around Y axis by view angle
        //   3. flips coordinates so camera looks down -Z
        //
        // due to how matrix multiplication works, these things
        // must be specified in the reverse order as above.
        glMatrixMode(GL_MODELVIEW);

        glLoadMatrixd(flip_matrix);
        glRotated(-angle, 0, 0, +1);
        glTranslated(-r_view.x, -r_view.y, -r_view.z);

        // the projection matrix creates the 3D perspective
        glMatrixMode(GL_PROJECTION);

        float x_slope = 100.0 / render_pixel_aspect;
        float y_slope = (float)oh / (float)ow;

        // this matches behavior of S/W renderer.
        // [ currently it is important since we use the S/W path
        //   for querying what the mouse is pointing at ]
        float z_near = x_slope;
        float z_far  = render_far_clip - 8.0;

        glLoadIdentity();
        glFrustum(-x_slope, +x_slope, -y_slope, +y_slope, z_near, z_far);
#endif
    }

    void Finish()
    {
#ifdef _FLTK_DISABLED
        // reset state
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_ALPHA_TEST);

        // reset matrices
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
#endif
    }
};

void RGL_RenderWorld(int ox, int oy, int ow, int oh)
{
    RendInfo3D rend;

    rend.Begin(ow, oh);
    rend.Render();
    rend.Highlight();
    rend.Finish();
}

#endif /* NO_OPENGL */
}