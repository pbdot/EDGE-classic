//------------------------------------------------------------------------
//  Adjustable border (variation of Fl_Tile)
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2012-2013 Andrew Apted
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
#include "smc_ui_tile.h"
#include "smc_ui_window.h"

namespace smc
{

//
// UI_Tile Constructor
//
#ifdef _FLTK_DISABLED
UI_Tile::UI_Tile(int X, int Y, int W, int H, const char *what, Fl_Widget *_left, Fl_Widget *_right)
    : Fl_Tile(X, Y, W, H, what), left(_left), right(_right)
{
    end();

    add(left);
    add(right);

    right_W = right->w();

    limiter = new Fl_Box(FL_NO_BOX, X + 32, Y, W - 32 - MIN_BROWSER_W, H, NULL);

    limiter->clear_visible(); // prevent this fucker from stealing events

    add(limiter);

    resizable(limiter);
}
#endif

//
// UI_Tile Destructor
//
UI_Tile::~UI_Tile()
{
}

void UI_Tile::resize(int X, int Y, int W, int H)
{
#ifdef _FLTK_DISABLED
    // resize ourself (skip the Fl_Group resize)
    Fl_Widget::resize(X, Y, W, H);

    // update limiter
    limiter->resize(X + 32, Y, W - 32 - MIN_BROWSER_W, H);

    if (find(right) >= children())
    {
        left->resize(X, Y, W, H);
        return;
    }

    // determine the width of the browser
    right_W = right->w();

    if (right_W > w() - 32)
        right_W = w() - 32;

    if (right_W < MIN_BROWSER_W)
        right_W = MIN_BROWSER_W;

    left->resize(X, Y, W - right_W, H);
    right->resize(X + W - right_W, Y, right_W, H);
#endif
}

void UI_Tile::ResizeBoth()
{
#ifdef _FLTK_DISABLED
    right->resize(x() + w() - right_W, y(), right_W, h());
    right->show();
    right->redraw();

    left->resize(x(), y(), w() - right_W, h());
    left->redraw();

    init_sizes();
#endif
}

void UI_Tile::ShowRight()
{
#ifdef _FLTK_DISABLED
    if (find(right) < children())
        return;

    // determine the width of the browser
    right_W = CLAMP(MIN_BROWSER_W, right_W, w() - 32);

    add(right);

    ResizeBoth();
#endif
}

void UI_Tile::HideRight()
{
#ifdef _FLTK_DISABLED
    if (find(right) >= children())
        return;

    // remember old width
    right_W = right->w();

    right->hide();

    remove(right);

    left->size(w(), h());
    left->redraw();

    // widgets in our group (the window) got rearranged, tell FLTK
    init_sizes();
#endif
}

void UI_Tile::MinimiseRight()
{
#ifdef _FLTK_DISABLED
    if (find(right) >= children())
        return;

    right_W = MIN_BROWSER_W;

    ResizeBoth();
#endif
}

void UI_Tile::MaximiseRight()
{
#ifdef _FLTK_DISABLED
    if (find(right) >= children())
        return;

    right_W = w() - 32;

    ResizeBoth();
#endif
}

bool UI_Tile::ParseUser(const char **tokens, int num_tok)
{
#ifdef _FLTK_DISABLED
    if (strcmp(tokens[0], "br_width") == 0 && num_tok >= 2)
    {
        bool was_visible = right->visible();

        if (was_visible)
            HideRight();

        right_W = atoi(tokens[1]);

        if (was_visible)
            ShowRight();

        return true;
    }
#endif
    return false;
}

void UI_Tile::WriteUser(FILE *fp)
{
#ifdef _FLTK_DISABLED
    fprintf(fp, "br_width %d\n", right->visible() ? right->w() : right_W);
#endif
}

} // namespace smc