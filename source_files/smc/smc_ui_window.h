//------------------------------------------------------------------------
//  MAIN WINDOW
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2006-2016 Andrew Apted
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

#pragma once

#include "smc_ui_menu.h"
#include "smc_ui_canvas.h"
#include "smc_ui_infobar.h"
#include "smc_ui_hyper.h"
#include "smc_ui_nombre.h"

#include "smc_ui_pic.h"
#include "smc_ui_scroll.h"
#include "smc_ui_tile.h"
#include "smc_ui_browser.h"
#include "smc_ui_default.h"
#include "smc_ui_editor.h"
#include "smc_ui_replace.h"

#include "smc_ui_thing.h"
#include "smc_ui_sector.h"
#include "smc_ui_sidedef.h"
#include "smc_ui_linedef.h"
#include "smc_ui_vertex.h"

namespace smc
{

#define WINDOW_BG FL_DARK3

#define MIN_BROWSER_W 250

class Wad_file;

#ifdef _FLTK_DISABLED
class UI_MainWindow : public Fl_Double_Window
#else
class UI_MainWindow
#endif
{
  public:
    // main child widgets

#ifdef _FLTK_DISABLED
    Fl_Sys_Menu_Bar *menu_bar;
#endif

    int panel_W;

    UI_Tile *tile;

    UI_CanvasScroll *scroll;
    UI_Canvas       *canvas;

    UI_Browser *browser;

    UI_InfoBar   *info_bar;
    UI_StatusBar *status_bar;

    UI_ThingBox  *thing_box;
    UI_LineBox   *line_box;
    UI_SectorBox *sec_box;
    UI_VertexBox *vert_box;

    UI_DefaultProps   *props_box;
    UI_FindAndReplace *find_box;

  private:
#ifdef _FLTK_DISABLED
    // active cursor
    Fl_Cursor cursor_shape;
#endif

    // remember window size/position after going fullscreen.
    // the 'last_w' and 'last_h' fields are zero when not fullscreen
    int last_x, last_y, last_w, last_h;

  public:
    UI_MainWindow();
    virtual ~UI_MainWindow();

    // FLTK methods
    int  handle(int event);
    void draw();

  public:
    void SetTitle(const char *wad_name, const char *map_name, bool read_only);

    // add or remove the '*' (etc) in the title
    void UpdateTitle(char want_prefix = 0);

    void Maximize();

    void NewEditMode(obj_type_e mode);

    // this is a wrapper around the FLTK cursor() method which
    // prevents the possibly expensive call when the shape hasn't
    // changed.
#ifdef _FLTK_DISABLED
    void SetCursor(Fl_Cursor shape);
#endif

    // show or hide the Browser panel.
    // kind is NUL or '-' to hide, '/' to toggle, 'T' for textures, 'F' flats,
    //         'O' for thing types, 'L' line types, 'S' sector types.
    void BrowserMode(char kind);

    void ShowDefaultProps();
    void ShowFindAndReplace();

    void UpdateTotals();

    int GetPanelObjNum() const;

    void InvalidatePanelObj();
    void UpdatePanelObj();

    void UnselectPics();

    void HideSpecialPanel();

    bool isSpecialPanelShown()
    {
#ifdef _FLTK_DISABLED
        return props_box->visible() || find_box->visible();
#else
        return false;
#endif
    }

    void Delay(int steps); // each step is 1/10th second

    // see if one of the panels wants to perform a clipboard op,
    // because a texture is highlighted or selected (for example).
    // op == 'c' for copy, 'x' cut, 'v' paste, 'd' delete.
    bool ClipboardOp(char op);

    // this is used by the browser when user clicks on an entry.
    // kind == 'T' for textures (etc... as above)
    void BrowsedItem(char kind, int number, const char *name, int e_state);

    // this is called when game_info changes (in Main_LoadResources)
    // and can enable / disable stuff in the panels.
    void UpdateGameInfo();

  private:
#ifdef _FLTK_DISABLED
    static void quit_callback(Fl_Widget *w, void *data);
#endif
};

extern UI_MainWindow *main_win;

//------------------------------------------------------------------------

#ifdef _FLTK_DISABLED
class UI_Escapable_Window : public Fl_Double_Window
#else
class UI_Escapable_Window
#endif
{
  public:
    UI_Escapable_Window(int W, int H, const char *L = NULL);
    virtual ~UI_Escapable_Window();

  public:
    // FLTK event handling method
    int handle(int event);
};

//------------------------------------------------------------------------

class UI_LogViewer : public UI_Escapable_Window
{
  private:
#ifdef _FLTK_DISABLED
    Fl_Multi_Browser *browser;
    Fl_Button        *copy_but;
#endif

  public:
    UI_LogViewer();
    virtual ~UI_LogViewer();

    void Add(const char *line);

    void Deselect();

    // ensure the very last line is visible
    void JumpEnd();

  private:
    int CountSelectedLines() const;

    char *GetSelectedText() const;

#ifdef _FLTK_DISABLED
    static void ok_callback(Fl_Widget *, void *);
    static void clear_callback(Fl_Widget *, void *);
    static void save_callback(Fl_Widget *, void *);
    static void select_callback(Fl_Widget *, void *);
    static void copy_callback(Fl_Widget *, void *);
#endif
};

extern UI_LogViewer *log_viewer;

void LogViewer_Open();

} // namespace smc