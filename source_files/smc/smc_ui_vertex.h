//------------------------------------------------------------------------
//  VERTEX PANEL
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2015 Andrew Apted
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

namespace smc
{

#ifdef _FLTK_DISABLED
class UI_VertexBox : public Fl_Group
#else
class UI_VertexBox
#endif
{
  private:
    int obj;
    int count;

  public:
    UI_Nombre *which;

#ifdef _FLTK_DISABLED
    Fl_Int_Input *pos_x;
    Fl_Int_Input *pos_y;

    Fl_Button *move_left;
    Fl_Button *move_right;
    Fl_Button *move_up;
    Fl_Button *move_down;
#endif

  public:
    UI_VertexBox(int X, int Y, int W, int H, const char *label = NULL);
    virtual ~UI_VertexBox();

  public:
    int handle(int event);
    // FLTK virtual method for handling input events.

  public:
    void SetObj(int _index, int _count);

    int GetObj() const
    {
        return obj;
    }

    // call this if the vertex was externally changed.
    void UpdateField();

    void UpdateTotal();

  private:
#ifdef _FLTK_DISABLED
    static void x_callback(Fl_Widget *, void *);
    static void y_callback(Fl_Widget *, void *);
    static void button_callback(Fl_Widget *, void *);
#endif
};

} // namespace smc