//------------------------------------------------------------------------
//  A decent scrolling widget
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2012-2016 Andrew Apted
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

#define SBAR_W 16

// _FLTK_DISABLED
class UI_Scroll /* : public Fl_Group */
{
  private:
#ifdef _FLTK_DISABLED
    Fl_Scrollbar *scrollbar;
#endif

    int bar_side;

    bool resize_horiz_;

    int top_y, bottom_y;

  public:
    UI_Scroll(int X, int Y, int W, int H, int _bar_side = -1);

    virtual ~UI_Scroll();

    /* FLTK methods */
    void resize(int X, int Y, int W, int H);
    int  handle(int event);

  public:
    void resize_horiz(bool r)
    {
        resize_horiz_ = r;
    }

#ifdef _FLTK_DISABLED
    void Add(Fl_Widget *w);
    void Remove(Fl_Widget *w);
#endif
    void Remove_first();
    void Remove_all();

    int Children() const;
#ifdef _FLTK_DISABLED
    Fl_Widget *Child(int i) const;
#endif

    void Init_sizes();

    void Line_size(int pixels);

    // delta is positive to move further down the list, negative to
    // move further up.  Its absolute value can be:
    //   1 : scroll by a small amount
    //   2 : scroll by one "line"
    //   3 : scroll to next/previous page
    //   4 : scroll to end
    void Scroll(int delta);

    // scroll so that a certain child widget can be seen.
    // currently it is placed at top of scroll area, or as close
    // as possible.
    void JumpToChild(int i);

  private:
    void ScrollByPixels(int pixels);

    void do_scroll();
    void calc_extents();
    void reposition_all(int start_y);

#ifdef _FLTK_DISABLED
    static void bar_callback(Fl_Widget *, void *);
#endif
};

//------------------------------------------------------------------------

class UI_Canvas;

// _FLTK_DISABLED
class UI_CanvasScroll /*: public Fl_Group*/
{
  public:
    UI_Canvas *canvas;

    UI_StatusBar *status;

  private:
#ifdef _FLTK_DISABLED
    Fl_Scrollbar *horiz;
    Fl_Scrollbar *vert;
#endif

    bool enable_bars;

    int bound_x1, bound_x2;
    int bound_y1, bound_y2;

    int last_bounds[4];

  public:
    UI_CanvasScroll(int X, int Y, int W, int H);

    virtual ~UI_CanvasScroll();

  public:
    void UpdateRenderMode();
    void UpdateBounds();

    void AdjustPos();

  private:
    void Adjust_X();
    void Adjust_Y();

    void UpdateBounds_X();
    void UpdateBounds_Y();

    void Scroll_X(); // callback helpers
    void Scroll_Y();

#ifdef _FLTK_DISABLED
    static void bar_callback(Fl_Widget *, void *);
#endif
};

} // namespace smc