//------------------------------------------------------------------------
//  Information Bar (bottom of window)
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2009 Andrew Apted
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
#include "smc_m_config.h"

#include "smc_ui_window.h"
#include "smc_ui_nombre.h"

namespace smc
{

#define NOMBRBACK_COL (gui_scheme == 2 ? FL_GRAY0 + 1 : FL_GRAY0 + 3)

//
// UI_Nombre Constructor
//
UI_Nombre::UI_Nombre(int X, int Y, int W, int H, const char *what)
    :
#ifdef _FLTK_DISABLED
      Fl_Box(FL_FLAT_BOX, X, Y, W, H, ""),
#endif
      index(-1), total(0), selected(0)
{
    type_name = StringDup(what);

#ifdef _FLTK_DISABLED
    align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
    color(NOMBRBACK_COL);

    labelfont(FL_COURIER_BOLD);
    labelsize(19);
#endif

    Update();
}

//
// UI_Nombre Destructor
//
UI_Nombre::~UI_Nombre()
{
    StringFree(type_name);
    type_name = NULL;
}

void UI_Nombre::Update()
{
    char buffer[256];

    if (index < 0)
        snprintf(buffer, sizeof(buffer), "No %s    / %d\n", type_name, total);
    else if (selected > 1)
        snprintf(buffer, sizeof(buffer), "%s #%-4d + %d more\n", type_name, index, selected - 1);
    else
        snprintf(buffer, sizeof(buffer), "%s #%-4d / %d\n", type_name, index, total);

#ifdef _FLTK_DISABLED
    if (index < 0 || total == 0)
        labelcolor(FL_DARK1);
    else
        labelcolor(FL_YELLOW);

    if (index < 0 || total == 0)
        color(NOMBRBACK_COL);
    else if (selected == 0)
        color(NOMBRBACK_COL); // same as above
    else if (selected == 1)
        color(FL_BLUE);
    else
        color(FL_RED);

    copy_label(buffer);
#endif
}

void UI_Nombre::SetIndex(int _idx)
{
    if (index != _idx)
    {
        index = _idx;
        Update();
    }
}

void UI_Nombre::SetTotal(int _tot)
{
    if (total != _tot)
    {
        total = _tot;
        Update();
    }
}

void UI_Nombre::SetSelected(int _sel)
{
    if (selected != _sel)
    {
        selected = _sel;
        Update();
    }
}

} // namespace smc