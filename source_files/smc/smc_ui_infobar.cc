//------------------------------------------------------------------------
//  Information Bar (bottom of window)
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2019 Andrew Apted
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
#include "smc_ui_window.h"

#include "smc_e_main.h"
#include "smc_e_linedef.h"
#include "smc_m_config.h"
#include "smc_m_game.h"
#include "smc_r_grid.h"
#include "smc_r_render.h"

namespace smc
{

#define SNAP_COLOR (gui_scheme == 2 ? fl_rgb_color(255, 96, 0) : fl_rgb_color(255, 96, 0))
#define FREE_COLOR (gui_scheme == 2 ? fl_rgb_color(0, 192, 0) : fl_rgb_color(128, 255, 128))

#define RATIO_COLOR FL_YELLOW

const char *UI_InfoBar::scale_options_str = "  6%| 12%| 25%| 33%| 50%|100%|200%|400%|800%";

const double UI_InfoBar::scale_amounts[9] = {0.0625, 0.125, 0.25, 0.33333, 0.5, 1.0, 2.0, 4.0, 8.0};

const char *UI_InfoBar::grid_options_str = "1024|512|256|192|128| 64| 32| 16|  8|  4|  2|OFF";

const int UI_InfoBar::grid_amounts[12] = {
    1024, 512, 256, 192, 128, 64, 32, 16, 8, 4, 2, -1 /* OFF */
};

//
// UI_InfoBar Constructor
//
UI_InfoBar::UI_InfoBar(int X, int Y, int W, int H, const char *label)
#ifdef _FLTK_DISABLED
    : Fl_Group(X, Y, W, H, label)
#endif
{
#ifdef _FLTK_DISABLED
    box(FL_FLAT_BOX);

    // Fitts' law : keep buttons flush with bottom of window
    Y += 4;
    H -= 4;

    Fl_Box *mode_lab = new Fl_Box(FL_NO_BOX, X, Y, 56, H, "Mode:");
    mode_lab->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    mode_lab->labelsize(16);

    mode = new Fl_Menu_Button(X + 58, Y, 96, H, "Things");
    mode->align(FL_ALIGN_INSIDE);
    mode->add("Things|Linedefs|Sectors|Vertices");
    mode->callback(mode_callback, this);
    mode->labelsize(16);

    X = mode->x() + mode->w() + 10;

    Fl_Box *scale_lab = new Fl_Box(FL_NO_BOX, X, Y, 58, H, "Scale:");
    scale_lab->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    scale_lab->labelsize(16);

    scale = new Fl_Menu_Button(X + 60 + 26, Y, 78, H, "100%");
    scale->align(FL_ALIGN_INSIDE);
    scale->add(scale_options_str);
    scale->callback(scale_callback, this);
    scale->labelsize(16);

    Fl_Button *sc_minus, *sc_plus;

    sc_minus = new Fl_Button(X + 60, Y + 1, 24, H - 2, "-");
    sc_minus->callback(sc_minus_callback, this);
    sc_minus->labelfont(FL_HELVETICA_BOLD);
    sc_minus->labelsize(16);

    sc_plus = new Fl_Button(X + 60 + 26 + 80, Y + 1, 24, H - 2, "+");
    sc_plus->callback(sc_plus_callback, this);
    sc_plus->labelfont(FL_HELVETICA_BOLD);
    sc_plus->labelsize(16);

    X = sc_plus->x() + sc_plus->w() + 12;

    Fl_Box *gs_lab = new Fl_Box(FL_NO_BOX, X, Y, 42, H, "Grid:");
    gs_lab->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    gs_lab->labelsize(16);

    grid_size = new Fl_Menu_Button(X + 44, Y, 72, H, "OFF");

    grid_size->align(FL_ALIGN_INSIDE);
    grid_size->add(grid_options_str);
    grid_size->callback(grid_callback, this);
    grid_size->labelsize(16);

    X = grid_size->x() + grid_size->w() + 12;

    grid_snap = new Fl_Toggle_Button(X + 4, Y, 72, H);
    grid_snap->value(grid.snap ? 1 : 0);
    grid_snap->color(FREE_COLOR);
    grid_snap->selection_color(SNAP_COLOR);
    grid_snap->callback(snap_callback, this);
    grid_snap->labelsize(16);

    UpdateSnapText();

    X = grid_snap->x() + grid_snap->w() + 12;

    Fl_Box *ratio_lab = new Fl_Box(FL_NO_BOX, X, Y, 52, H, "Ratio:");
    ratio_lab->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    ratio_lab->labelsize(16);

    ratio_lock = new Fl_Menu_Button(X + 54, Y, 106, H, "UNLOCK");
    ratio_lock->align(FL_ALIGN_INSIDE);
    ratio_lock->add("UNLOCK|1:1|2:1|4:1|8:1|5:4|7:4|User Value");
    ratio_lock->callback(ratio_callback, this);
    ratio_lock->labelsize(16);

    X = ratio_lock->x() + ratio_lock->w() + 12;

    Fl_Box *rend_lab = new Fl_Box(FL_FLAT_BOX, X, Y, 56, H, "Rend:");
    rend_lab->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    rend_lab->labelsize(16);

    sec_rend = new Fl_Menu_Button(X + 58, Y, 96, H, "PLAIN");
    sec_rend->align(FL_ALIGN_INSIDE);
    sec_rend->add("PLAIN|Floor|Ceiling|Lighting|Floor Bright|Ceil Bright|Sound|3D VIEW");
    sec_rend->callback(rend_callback, this);
    sec_rend->labelsize(16);

    X = sec_rend->x() + rend_lab->w() + 10;

    resizable(NULL);

    end();
#endif
}

//
// UI_InfoBar Destructor
//
UI_InfoBar::~UI_InfoBar()
{
}

int UI_InfoBar::handle(int event)
{
#ifdef _FLTK_DISABLED
    return Fl_Group::handle(event);
#else
    return 0;
#endif
}

#ifdef _FLTK_DISABLED
void UI_InfoBar::mode_callback(Fl_Widget *w, void *data)
{
    Fl_Menu_Button *mode = (Fl_Menu_Button *)w;

    static const char *mode_keys = "tlsvr";

    Editor_ChangeMode(mode_keys[mode->value()]);
}

void UI_InfoBar::rend_callback(Fl_Widget *w, void *data)
{
    Fl_Menu_Button *sec_rend = (Fl_Menu_Button *)w;

    // last option is 3D mode
    if (sec_rend->value() > SREND_SoundProp)
    {
        Render3D_Enable(true);
        return;
    }

    switch (sec_rend->value())
    {
    case 1:
        edit.sector_render_mode = SREND_Floor;
        break;
    case 2:
        edit.sector_render_mode = SREND_Ceiling;
        break;
    case 3:
        edit.sector_render_mode = SREND_Lighting;
        break;
    case 4:
        edit.sector_render_mode = SREND_FloorBright;
        break;
    case 5:
        edit.sector_render_mode = SREND_CeilBright;
        break;
    case 6:
        edit.sector_render_mode = SREND_SoundProp;
        break;
    default:
        edit.sector_render_mode = SREND_Nothing;
        break;
    }

    if (edit.render3d)
        Render3D_Enable(false);

    // need sectors mode for sound propagation display
    if (edit.sector_render_mode == SREND_SoundProp && edit.mode != OBJ_SECTORS)
        Editor_ChangeMode('s');

    RedrawMap();
}

void UI_InfoBar::scale_callback(Fl_Widget *w, void *data)
{
    Fl_Menu_Button *scale = (Fl_Menu_Button *)w;

    double new_scale = scale_amounts[scale->value()];

    grid.NearestScale(new_scale);
}

void UI_InfoBar::sc_minus_callback(Fl_Widget *w, void *data)
{
    ExecuteCommand("Zoom", "-1", "/center");
}

void UI_InfoBar::sc_plus_callback(Fl_Widget *w, void *data)
{
    ExecuteCommand("Zoom", "+1", "/center");
}

void UI_InfoBar::grid_callback(Fl_Widget *w, void *data)
{
    Fl_Menu_Button *gsize = (Fl_Menu_Button *)w;

    int new_step = grid_amounts[gsize->value()];

    if (new_step < 0)
        grid.SetShown(false);
    else
        grid.ForceStep(new_step);
}

void UI_InfoBar::snap_callback(Fl_Widget *w, void *data)
{
    Fl_Toggle_Button *grid_snap = (Fl_Toggle_Button *)w;

    // update editor state
    grid.SetSnap(grid_snap->value() ? true : false);
}

void UI_InfoBar::ratio_callback(Fl_Widget *w, void *data)
{
    Fl_Menu_Button *ratio_lock = (Fl_Menu_Button *)w;

    grid.ratio = ratio_lock->value();
    main_win->info_bar->UpdateRatio();
}
#endif

//------------------------------------------------------------------------

void UI_InfoBar::NewEditMode(obj_type_e new_mode)
{
#ifdef _FLTK_DISABLED
    switch (new_mode)
    {
    case OBJ_THINGS:
        mode->value(0);
        break;
    case OBJ_LINEDEFS:
        mode->value(1);
        break;
    case OBJ_SECTORS:
        mode->value(2);
        break;
    case OBJ_VERTICES:
        mode->value(3);
        break;

    default:
        break;
    }
#endif

    UpdateModeColor();
}

void UI_InfoBar::SetMouse(double mx, double my)
{
    // TODO this method should go away

#ifdef _FLTK_DISABLED
    main_win->status_bar->redraw();
#endif
}

void UI_InfoBar::SetScale(double new_scale)
{
    double perc = new_scale * 100.0;

    char buffer[64];

    if (perc < 10.0)
        snprintf(buffer, sizeof(buffer), "%1.1f%%", perc);
    else
        snprintf(buffer, sizeof(buffer), "%3d%%", (int)perc);

#ifdef _FLTK_DISABLED
    scale->copy_label(buffer);
#endif
}

void UI_InfoBar::SetGrid(int new_step)
{
    if (new_step < 0)
    {
#ifdef _FLTK_DISABLED
        grid_size->label("OFF");
#endif
    }
    else
    {
#ifdef _FLTK_DISABLED
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%d", new_step);
        grid_size->copy_label(buffer);
#endif
    }
}

void UI_InfoBar::UpdateSnap()
{
#ifdef _FLTK_DISABLED
    grid_snap->value(grid.snap ? 1 : 0);
#endif

    UpdateSnapText();
}

void UI_InfoBar::UpdateSecRend()
{
    if (edit.render3d)
    {
#ifdef _FLTK_DISABLED
        sec_rend->label("3D VIEW");
#endif
        return;
    }

    switch (edit.sector_render_mode)
    {
#ifdef _FLTK_DISABLED
    case SREND_Floor:
        sec_rend->label("Floor");
        break;
    case SREND_Ceiling:
        sec_rend->label("Ceiling");
        break;
    case SREND_Lighting:
        sec_rend->label("Lighting");
        break;
    case SREND_FloorBright:
        sec_rend->label("Floor Brt");
        break;
    case SREND_CeilBright:
        sec_rend->label("Ceil Brt");
        break;
    case SREND_SoundProp:
        sec_rend->label("Sound");
        break;
    default:
        sec_rend->label("PLAIN");
        break;
#endif
    }
}

void UI_InfoBar::UpdateRatio()
{
#ifdef _FLTK_DISABLED
    if (grid.ratio == 0)
        ratio_lock->color(FL_BACKGROUND_COLOR);
    else
        ratio_lock->color(RATIO_COLOR);

    if (grid.ratio == 7)
    {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "Usr %d:%d", grid_ratio_high, grid_ratio_low);

        // drop the "Usr" part when overly long
        if (strlen(buffer) > 9)
            ratio_lock->copy_label(buffer + 4);
        else
            ratio_lock->copy_label(buffer);
    }
    else
    {
        ratio_lock->copy_label(ratio_lock->text(grid.ratio));
    }
#endif
}

void UI_InfoBar::UpdateModeColor()
{
#ifdef _FLTK_DISABLED
    switch (mode->value())
    {
    case 0:
        mode->label("Things");
        mode->color(THING_MODE_COL);
        break;
    case 1:
        mode->label("Linedefs");
        mode->color(LINE_MODE_COL);
        break;
    case 2:
        mode->label("Sectors");
        mode->color(SECTOR_MODE_COL);
        break;
    case 3:
        mode->label("Vertices");
        mode->color(VERTEX_MODE_COL);
        break;
    }
#endif
}

void UI_InfoBar::UpdateSnapText()
{
#ifdef _FLTK_DISABLED
    if (grid_snap->value())
    {
        grid_snap->label("SNAP");
    }
    else
    {
        grid_snap->label("Free");
    }

    grid_snap->redraw();
#endif
}

//------------------------------------------------------------------------

#define INFO_TEXT_COL fl_rgb_color(192, 192, 192)
#define INFO_DIM_COL  fl_rgb_color(128, 128, 128)

UI_StatusBar::UI_StatusBar(int X, int Y, int W, int H, const char *label)
    :
#ifdef _FLTK_DISABLED
      Fl_Widget(X, Y, W, H, label),
#endif
      status()
{
#ifdef _FLTK_DISABLED
    box(FL_NO_BOX);
#endif
}

UI_StatusBar::~UI_StatusBar()
{
}

int UI_StatusBar::handle(int event)
{
    // this never handles any events
    return 0;
}

void UI_StatusBar::draw()
{
#ifdef _FLTK_DISABLED
    fl_color(fl_rgb_color(64, 64, 64));
    fl_rectf(x(), y(), w(), h());

    fl_color(fl_rgb_color(96, 96, 96));
    fl_rectf(x(), y() + h() - 1, w(), 1);

    fl_push_clip(x(), y(), w(), h());

    fl_font(FL_COURIER, 16);

    int cx = x() + 10;
    int cy = y() + 20;

    if (edit.render3d)
    {
        IB_Number(cx, cy, "x", I_ROUND(r_view.x), 5);
        IB_Number(cx, cy, "y", I_ROUND(r_view.y), 5);
        IB_Number(cx, cy, "z", I_ROUND(r_view.z) - Misc_info.view_height, 4);

        // use less space when an action is occurring
        if (edit.action == ACT_NOTHING)
        {
            int ang = I_ROUND(r_view.angle * 180 / M_PI);
            if (ang < 0)
                ang += 360;

            IB_Number(cx, cy, "ang", ang, 3);
            cx += 2;

            IB_Flag(cx, cy, r_view.gravity, "GRAV", "grav");
#if 0
			IB_Number(cx, cy, "gamma", usegamma, 1);
#endif
        }

        cx += 4;
    }
    else // 2D view
    {
        float mx = grid.SnapX(edit.map_x);
        float my = grid.SnapX(edit.map_y);

        mx = CLAMP(-32767, mx, 32767);
        my = CLAMP(-32767, my, 32767);

        IB_Coord(cx, cy, "x", mx);
        IB_Coord(cx, cy, "y", my);
        cx += 10;
#if 0
		IB_Number(cx, cy, "gamma", usegamma, 1);
		cx += 10;
#endif
    }

    /* status message */

    IB_Flag(cx, cy, true, "|", "|");

    fl_color(INFO_TEXT_COL);

    switch (edit.action)
    {
    case ACT_DRAG:
        IB_ShowDrag(cx, cy);
        break;

    case ACT_TRANSFORM:
        IB_ShowTransform(cx, cy);
        break;

    case ACT_ADJUST_OFS:
        IB_ShowOffsets(cx, cy);
        break;

    case ACT_DRAW_LINE:
        IB_ShowDrawLine(cx, cy);
        break;

    default:
        fl_draw(status.c_str(), cx, cy);
        break;
    }

    fl_pop_clip();
#endif
}

void UI_StatusBar::IB_ShowDrag(int cx, int cy)
{
    if (edit.render3d && edit.mode == OBJ_SECTORS)
    {
        IB_Number(cx, cy, "raise delta", I_ROUND(edit.drag_sector_dz), 4);
        return;
    }
    if (edit.render3d && edit.mode == OBJ_THINGS && edit.drag_thing_up_down)
    {
        double dz = edit.drag_cur_z - edit.drag_start_z;
        IB_Number(cx, cy, "raise delta", I_ROUND(dz), 4);
        return;
    }

    double dx, dy;

    if (edit.render3d)
    {
        dx = edit.drag_cur_x - edit.drag_start_x;
        dy = edit.drag_cur_y - edit.drag_start_y;
    }
    else
    {
        main_win->canvas->DragDelta(&dx, &dy);
    }

    IB_Coord(cx, cy, "dragging delta x", dx);
    IB_Coord(cx, cy, "y", dy);
}

void UI_StatusBar::IB_ShowTransform(int cx, int cy)
{
    int rot_degrees;

    switch (edit.trans_mode)
    {
    case TRANS_K_Scale:
        IB_Coord(cx, cy, "scale by", edit.trans_param.scale_x);
        break;

    case TRANS_K_Stretch:
        IB_Coord(cx, cy, "stretch x", edit.trans_param.scale_x);
        IB_Coord(cx, cy, "y", edit.trans_param.scale_y);
        break;

    case TRANS_K_Rotate:
    case TRANS_K_RotScale:
        rot_degrees = edit.trans_param.rotate * 90 / 16384;
        IB_Number(cx, cy, "rotate by", rot_degrees, 3);
        break;

    case TRANS_K_Skew:
        IB_Coord(cx, cy, "skew x", edit.trans_param.skew_x);
        IB_Coord(cx, cy, "y", edit.trans_param.skew_y);
        break;
    }

    if (edit.trans_mode == TRANS_K_RotScale)
        IB_Coord(cx, cy, "scale", edit.trans_param.scale_x);
}

void UI_StatusBar::IB_ShowOffsets(int cx, int cy)
{
    int dx = I_ROUND(edit.adjust_dx);
    int dy = I_ROUND(edit.adjust_dy);

    Objid hl = edit.highlight;

    if (!edit.Selected->empty())
    {
        if (edit.Selected->count_obj() == 1)
        {
            int first = edit.Selected->find_first();
            int parts = edit.Selected->get_ext(first);

            hl = Objid(edit.mode, first, parts);
        }
        else
        {
            hl.clear();
        }
    }

    if (hl.valid() && hl.parts >= 2)
    {
        const LineDef *L = LineDefs[edit.highlight.num];

        int x_offset = 0;
        int y_offset = 0;

        const SideDef *SD = NULL;

        if (hl.parts & PART_LF_ALL)
            SD = L->Left();
        else
            SD = L->Right();

        if (SD != NULL)
        {
            x_offset = SD->x_offset;
            y_offset = SD->y_offset;

            IB_Number(cx, cy, "new ofs x", x_offset + dx, 4);
            IB_Number(cx, cy, "y", y_offset + dy, 4);
        }
    }

    IB_Number(cx, cy, "delta x", dx, 4);
    IB_Number(cx, cy, "y", dy, 4);
}

void UI_StatusBar::IB_ShowDrawLine(int cx, int cy)
{
    if (!edit.draw_from.valid())
        return;

    const Vertex *V = Vertices[edit.draw_from.num];

    double dx = edit.draw_to_x - V->x();
    double dy = edit.draw_to_y - V->y();

    // show a ratio value
    fixcoord_t fdx = TO_COORD(dx);
    fixcoord_t fdy = TO_COORD(dy);

    std::string ratio_name = LD_RatioName(fdx, fdy, false);

    int old_cx = cx;
    IB_String(cx, cy, ratio_name.c_str());

    cx = MAX(cx + 12, old_cx + 170);

    IB_Coord(cx, cy, "delta x", dx);
    IB_Coord(cx, cy, "y", dy);
}

void UI_StatusBar::IB_Number(int &cx, int &cy, const char *label, int value, int size)
{
#ifdef _FLTK_DISABLED
    char buffer[256];

    // negative size means we require a sign
    if (size < 0)
        snprintf(buffer, sizeof(buffer), "%s:%-+*d ", label, -size + 1, value);
    else
        snprintf(buffer, sizeof(buffer), "%s:%-*d ", label, size, value);

    fl_color(INFO_TEXT_COL);
    fl_draw(buffer, cx, cy);

    cx = cx + fl_width(buffer);
#endif
}

void UI_StatusBar::IB_Coord(int &cx, int &cy, const char *label, float value)
{
#ifdef _FLTK_DISABLED
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s:%-8.2f ", label, value);

    fl_color(INFO_TEXT_COL);
    fl_draw(buffer, cx, cy);

    cx = cx + fl_width(buffer);
#endif
}

void UI_StatusBar::IB_String(int &cx, int &cy, const char *str)
{
#ifdef _FLTK_DISABLED
    fl_draw(str, cx, cy);

    cx = cx + fl_width(str);
#endif
}

void UI_StatusBar::IB_Flag(int &cx, int &cy, bool value, const char *label_on, const char *label_off)
{
#ifdef _FLTK_DISABLED
    const char *label = value ? label_on : label_off;

    fl_color(value ? INFO_TEXT_COL : INFO_DIM_COL);

    fl_draw(label, cx, cy);

    cx = cx + fl_width(label) + 20;
#endif
}

void UI_StatusBar::SetStatus(const char *str)
{
    if (status == str)
        return;

    status = str;

#ifdef _FLTK_DISABLED
    redraw();
#endif
}

void Status_Set(const char *fmt, ...)
{
    if (!main_win)
        return;

    va_list arg_ptr;

    static char buffer[MSG_BUF_LEN];

    va_start(arg_ptr, fmt);
    vsnprintf(buffer, MSG_BUF_LEN - 1, fmt, arg_ptr);
    va_end(arg_ptr);

    buffer[MSG_BUF_LEN - 1] = 0;

    main_win->status_bar->SetStatus(buffer);
}

void Status_Clear()
{
    if (!main_win)
        return;

    main_win->status_bar->SetStatus("");
}

} // namespace smc