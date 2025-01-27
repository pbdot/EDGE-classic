//------------------------------------------------------------------------
//  SIDEDEF INFORMATION
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2016 Andrew Apted
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

#include "smc_e_hover.h" // OppositeSector
#include "smc_e_linedef.h"
#include "smc_e_main.h"
#include "smc_m_game.h"
#include "smc_r_render.h"
#include "smc_w_rawdef.h"
#include "smc_w_texture.h"

namespace smc
{

// config item
bool swap_sidedefs           = false;
bool show_full_one_sided     = false;
bool sidedef_add_del_buttons = false;

//
// Constructor
//
UI_SideBox::UI_SideBox(int X, int Y, int W, int H, int _side)
    :
#ifdef _FLTK_DISABLED
      Fl_Group(X, Y, W, H),
#endif
      obj(SETOBJ_NO_LINE), is_front(_side == 0), on_2S_line(false)
{
#ifdef _FLTK_DISABLED
    box(FL_FLAT_BOX); // FL_UP_BOX

    align(FL_ALIGN_INSIDE | FL_ALIGN_TOP | FL_ALIGN_LEFT);

    if (is_front)
        labelcolor(FL_BLUE);
    else
        labelcolor(fl_rgb_color(224, 64, 0));

    add_button = new Fl_Button(X + W - 120, Y, 50, 20, "ADD");
    add_button->labelcolor(labelcolor());
    add_button->callback(add_callback, this);

    del_button = new Fl_Button(X + W - 65, Y, 50, 20, "DEL");
    del_button->labelcolor(labelcolor());
    del_button->callback(delete_callback, this);

    X += 6;
    Y += 6 + 16; // space for label

    W -= 12;
    H -= 12;

    int MX = X + W / 2;

    x_ofs = new Fl_Int_Input(X + 28, Y, 52, 24, "x:");
    y_ofs = new Fl_Int_Input(MX - 20, Y, 52, 24, "y:");
    sec   = new Fl_Int_Input(X + W - 59, Y, 52, 24, "sec:");

    x_ofs->align(FL_ALIGN_LEFT);
    y_ofs->align(FL_ALIGN_LEFT);
    sec->align(FL_ALIGN_LEFT);

    x_ofs->callback(offset_callback, this);
    y_ofs->callback(offset_callback, this);
    sec->callback(sector_callback, this);

    x_ofs->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
    y_ofs->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
    sec->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

    Y += x_ofs->h() + 6;

    int LX = X + 16;
    int UX = X + W - 64 - 16;
    MX     = MX - 32;

    if (swap_sidedefs)
    {
        std::swap(UX, LX);
    }

    l_pic = new UI_Pic(LX, Y, 64, 64, "Lower");
    u_pic = new UI_Pic(UX, Y, 64, 64, "Upper");
    r_pic = new UI_Pic(MX, Y, 64, 64, "Rail");

    l_pic->callback(tex_callback, this);
    u_pic->callback(tex_callback, this);
    r_pic->callback(tex_callback, this);

    Y += 65;

    l_tex = new UI_DynInput(LX - 8, Y, 80, 20);
    u_tex = new UI_DynInput(UX - 8, Y, 80, 20);
    r_tex = new UI_DynInput(MX - 8, Y, 80, 20);

    l_tex->textsize(12);
    u_tex->textsize(12);
    r_tex->textsize(12);

    l_tex->callback(tex_callback, this);
    u_tex->callback(tex_callback, this);
    r_tex->callback(tex_callback, this);

    l_tex->callback2(dyntex_callback, this);
    u_tex->callback2(dyntex_callback, this);
    r_tex->callback2(dyntex_callback, this);

    l_tex->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
    u_tex->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);
    r_tex->when(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY);

    end();

    UpdateHiding();
    UpdateLabel();
    UpdateAddDel();
#endif
}

//
// Destructor
//
UI_SideBox::~UI_SideBox()
{
}

#ifdef _FLTK_DISABLED
void UI_SideBox::tex_callback(Fl_Widget *w, void *data)
{
    UI_SideBox *box = (UI_SideBox *)data;

    if (box->obj < 0)
        return;

    if (Fl::event_button() != 3 && (w == box->l_pic || w == box->u_pic || w == box->r_pic))
    {
        UI_Pic *pic = (UI_Pic *)w;

        pic->Selected(!pic->Selected());

        if (pic->Selected())
            main_win->BrowserMode('T');
        return;
    }

    int new_tex;

    // right click sets to default value, "-" for rail
    if (Fl::event_button() == 3)
    {
        if (w == box->r_pic)
            new_tex = BA_InternaliseString("-");
        else
            new_tex = BA_InternaliseString(default_wall_tex);
    }
    else
    {
        if (w == box->l_tex)
            new_tex = BA_InternaliseString(NormalizeTex(box->l_tex->value()));
        else if (w == box->u_tex)
            new_tex = BA_InternaliseString(NormalizeTex(box->u_tex->value()));
        else
            new_tex = BA_InternaliseString(NormalizeTex(box->r_tex->value()));
    }

    // iterate over selected linedefs
    if (!edit.Selected->empty())
    {
        BA_Begin();
        BA_MessageForSel("edited texture on", edit.Selected);

        for (sel_iter_c it(edit.Selected); !it.done(); it.next())
        {
            const LineDef *L = LineDefs[*it];

            int sd = box->is_front ? L->right : L->left;

            if (is_sidedef(sd))
            {
                bool lower = (w == box->l_tex || w == box->l_pic);
                bool upper = (w == box->u_tex || w == box->u_pic);
                bool rail  = (w == box->r_tex || w == box->r_pic);

                if (L->OneSided())
                    std::swap(lower, rail);

                if (lower)
                {
                    BA_ChangeSD(sd, SideDef::F_LOWER_TEX, new_tex);
                }
                else if (upper)
                {
                    BA_ChangeSD(sd, SideDef::F_UPPER_TEX, new_tex);
                }
                else if (rail)
                {
                    BA_ChangeSD(sd, SideDef::F_MID_TEX, new_tex);
                }
            }
        }

        BA_End();

        box->UpdateField();
    }
}

void UI_SideBox::dyntex_callback(Fl_Widget *w, void *data)
{
    // change picture to match the input, BUT does not change the map

    UI_SideBox *box = (UI_SideBox *)data;

    if (box->obj < 0)
        return;

    if (w == box->l_tex)
    {
        box->l_pic->GetTex(box->l_tex->value());
    }
    else if (w == box->u_tex)
    {
        box->u_pic->GetTex(box->u_tex->value());
    }
    else if (w == box->r_tex)
    {
        box->r_pic->GetTex(box->r_tex->value());
    }
}

void UI_SideBox::add_callback(Fl_Widget *w, void *data)
{
    UI_SideBox *box = (UI_SideBox *)data;

    if (box->obj >= 0)
        return;

    if (edit.Selected->empty())
        return;

    // iterate over selected linedefs

    int field = box->is_front ? LineDef::F_RIGHT : LineDef::F_LEFT;

    BA_Begin();

    // make sure we have a fallback sector to use
    if (NumSectors == 0)
    {
        int new_sec = BA_New(OBJ_SECTORS);

        Sectors[new_sec]->SetDefaults();
    }

    for (sel_iter_c it(edit.Selected); !it.done(); it.next())
    {
        const LineDef *L = LineDefs[*it];

        int sd    = box->is_front ? L->right : L->left;
        int other = box->is_front ? L->left : L->right;

        // skip lines which already have this sidedef
        if (is_sidedef(sd))
            continue;

        // determine what sector to use
        int new_sec = OppositeSector(*it, box->is_front ? SIDE_RIGHT : SIDE_LEFT);

        if (new_sec < 0)
            new_sec = NumSectors - 1;

        // create the new sidedef
        sd = BA_New(OBJ_SIDEDEFS);

        SideDefs[sd]->SetDefaults(other >= 0);
        SideDefs[sd]->sector = new_sec;

        BA_ChangeLD(*it, field, sd);

        if (other >= 0)
            LD_AddSecondSideDef(*it, sd, other);
    }

    BA_MessageForSel("added sidedef to", edit.Selected);
    BA_End();

    main_win->line_box->UpdateField();
    main_win->line_box->UpdateSides();
}

void UI_SideBox::delete_callback(Fl_Widget *w, void *data)
{
    UI_SideBox *box = (UI_SideBox *)data;

    if (box->obj < 0)
        return;

    if (edit.Selected->empty())
        return;

    // iterate over selected linedefs
    BA_Begin();

    for (sel_iter_c it(edit.Selected); !it.done(); it.next())
    {
        const LineDef *L = LineDefs[*it];

        int sd = box->is_front ? L->right : L->left;

        if (sd < 0)
            continue;

        // NOTE WELL: the actual sidedef is not deleted (it might be shared)

        LD_RemoveSideDef(*it, box->is_front ? SIDE_RIGHT : SIDE_LEFT);
    }

    BA_MessageForSel("deleted sidedef from", edit.Selected);
    BA_End();

    main_win->line_box->UpdateField();
    main_win->line_box->UpdateSides();
}

void UI_SideBox::offset_callback(Fl_Widget *w, void *data)
{
    UI_SideBox *box = (UI_SideBox *)data;

    int new_x_ofs = atoi(box->x_ofs->value());
    int new_y_ofs = atoi(box->y_ofs->value());

    // iterate over selected linedefs
    if (!edit.Selected->empty())
    {
        BA_Begin();

        if (w == box->x_ofs)
            BA_MessageForSel("edited X offset on", edit.Selected);
        else
            BA_MessageForSel("edited Y offset on", edit.Selected);

        for (sel_iter_c it(edit.Selected); !it.done(); it.next())
        {
            const LineDef *L = LineDefs[*it];

            int sd = box->is_front ? L->right : L->left;

            if (is_sidedef(sd))
            {
                if (w == box->x_ofs)
                    BA_ChangeSD(sd, SideDef::F_X_OFFSET, new_x_ofs);
                else
                    BA_ChangeSD(sd, SideDef::F_Y_OFFSET, new_y_ofs);
            }
        }

        BA_End();
    }
}

void UI_SideBox::sector_callback(Fl_Widget *w, void *data)
{
    UI_SideBox *box = (UI_SideBox *)data;

    int new_sec = atoi(box->sec->value());

    new_sec = CLAMP(0, new_sec, NumSectors - 1);

    // iterate over selected linedefs
    if (!edit.Selected->empty())
    {
        BA_Begin();
        BA_MessageForSel("edited sector-ref on", edit.Selected);

        for (sel_iter_c it(edit.Selected); !it.done(); it.next())
        {
            const LineDef *L = LineDefs[*it];

            int sd = box->is_front ? L->right : L->left;

            if (is_sidedef(sd))
                BA_ChangeSD(sd, SideDef::F_SECTOR, new_sec);
        }

        BA_End();
    }
}
#endif

//------------------------------------------------------------------------

void UI_SideBox::SetObj(int index, int solid_mask, bool two_sided)
{
    if (obj == index && what_is_solid == solid_mask && on_2S_line == two_sided)
        return;

    bool hide_change = !(obj == index && on_2S_line == two_sided);

    obj = index;

    what_is_solid = solid_mask;
    on_2S_line    = two_sided;

    if (hide_change)
        UpdateHiding();

    UpdateLabel();
    UpdateAddDel();
    UpdateField();

    if (obj < 0)
        UnselectPics();

#ifdef _FLTK_DISABLED
    redraw();
#endif
}

void UI_SideBox::UpdateField()
{
#ifdef _FLTK_DISABLED
    if (is_sidedef(obj))
    {
        const SideDef *sd = SideDefs[obj];

        x_ofs->value(Int_TmpStr(sd->x_offset));
        y_ofs->value(Int_TmpStr(sd->y_offset));
        sec->value(Int_TmpStr(sd->sector));

        const char *lower = sd->LowerTex();
        const char *rail  = sd->MidTex();
        const char *upper = sd->UpperTex();

        if (what_is_solid & SOLID_MID)
            std::swap(lower, rail);

        l_tex->value(lower);
        u_tex->value(upper);
        r_tex->value(rail);

        l_pic->GetTex(lower);
        u_pic->GetTex(upper);
        r_pic->GetTex(rail);

        if ((what_is_solid & (SOLID_LOWER | SOLID_MID)) && is_null_tex(lower))
            l_pic->MarkMissing();

        if ((what_is_solid & SOLID_UPPER) && is_null_tex(upper))
            u_pic->MarkMissing();

        if (is_special_tex(lower))
            l_pic->MarkSpecial();
        if (is_special_tex(upper))
            u_pic->MarkSpecial();
        if (is_special_tex(rail))
            r_pic->MarkSpecial();

        l_pic->AllowHighlight(true);
        u_pic->AllowHighlight(true);
        r_pic->AllowHighlight(true);
    }
    else
    {
        x_ofs->value("");
        y_ofs->value("");
        sec->value("");

        l_tex->value("");
        u_tex->value("");
        r_tex->value("");

        l_pic->Clear();
        u_pic->Clear();
        r_pic->Clear();

        l_pic->AllowHighlight(false);
        u_pic->AllowHighlight(false);
        r_pic->AllowHighlight(false);
    }
#endif
}

void UI_SideBox::UpdateLabel()
{
#ifdef _FLTK_DISABLED
    if (!is_sidedef(obj))
    {
        label(is_front ? "   No Front Sidedef" : "   No Back Sidedef");
        return;
    }

    char buffer[200];

    snprintf(buffer, sizeof(buffer), "   %s Sidedef: #%d\n", is_front ? "Front" : "Back", obj);

    copy_label(buffer);
#endif
}

void UI_SideBox::UpdateAddDel()
{
#ifdef _FLTK_DISABLED
    if (obj == SETOBJ_NO_LINE || !sidedef_add_del_buttons)
    {
        add_button->hide();
        del_button->hide();
    }
    else if (!is_sidedef(obj))
    {
        add_button->show();
        del_button->hide();
    }
    else
    {
        add_button->hide();
        del_button->show();
    }
#endif
}

void UI_SideBox::UpdateHiding()
{
#ifdef _FLTK_DISABLED
    if (obj < 0)
    {
        x_ofs->hide();
        y_ofs->hide();
        sec->hide();

        l_tex->hide();
        u_tex->hide();
        r_tex->hide();

        l_pic->hide();
        u_pic->hide();
        r_pic->hide();
    }
    else
    {
        x_ofs->show();
        y_ofs->show();
        sec->show();

        l_tex->show();
        l_pic->show();

        if (on_2S_line || show_full_one_sided)
        {
            u_tex->show();
            r_tex->show();

            u_pic->show();
            r_pic->show();
        }
        else
        {
            u_tex->hide();
            r_tex->hide();

            u_pic->hide();
            r_pic->hide();

            u_pic->Unhighlight();
            r_pic->Unhighlight();

            u_pic->Selected(false);
            r_pic->Selected(false);
        }
    }
#endif
}

int UI_SideBox::GetSelectedPics() const
{
    if (obj < 0)
        return 0;

    return (l_pic->Selected() ? PART_RT_LOWER : 0) | (u_pic->Selected() ? PART_RT_UPPER : 0) |
           (r_pic->Selected() ? PART_RT_RAIL : 0);
}

int UI_SideBox::GetHighlightedPics() const
{
    if (obj < 0)
        return 0;

    return (l_pic->Highlighted() ? PART_RT_LOWER : 0) | (u_pic->Highlighted() ? PART_RT_UPPER : 0) |
           (r_pic->Highlighted() ? PART_RT_RAIL : 0);
}

void UI_SideBox::UnselectPics()
{
    l_pic->Unhighlight();
    u_pic->Unhighlight();
    r_pic->Unhighlight();

    l_pic->Selected(false);
    u_pic->Selected(false);
    r_pic->Selected(false);
}

} // namespace smc