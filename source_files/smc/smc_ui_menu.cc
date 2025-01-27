//------------------------------------------------------------------------
//  MENUS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2018 Andrew Apted
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
#include "smc_m_files.h"
#include "smc_m_loadsave.h"

namespace smc
{

//------------------------------------------------------------------------
//  FILE MENU
//------------------------------------------------------------------------
#ifdef _FLTK_DISABLED

static void file_do_new_project(Fl_Widget *w, void *data)
{
    ExecuteCommand("NewProject");
}

static void file_do_manage_project(Fl_Widget *w, void *data)
{
    ExecuteCommand("ManageProject");
}

static void file_do_open(Fl_Widget *w, void *data)
{
    ExecuteCommand("OpenMap");
}

static void file_do_save(Fl_Widget *w, void *data)
{
    ExecuteCommand("SaveMap");
}

static void file_do_export(Fl_Widget *w, void *data)
{
    ExecuteCommand("ExportMap");
}

static void file_do_fresh_map(Fl_Widget *w, void *data)
{
    ExecuteCommand("FreshMap");
}

static void file_do_copy_map(Fl_Widget *w, void *data)
{
    ExecuteCommand("CopyMap");
}

static void file_do_rename(Fl_Widget *w, void *data)
{
    ExecuteCommand("RenameMap");
}

static void file_do_delete(Fl_Widget *w, void *data)
{
    ExecuteCommand("DeleteMap");
}

static void file_do_load_given(Fl_Widget *w, void *data)
{
    const char *filename = (const char *)data;

    int given_idx = M_FindGivenFile(filename);

    if (given_idx >= 0)
        last_given_file = given_idx;

    OpenFileMap(filename);
}

static void file_do_load_recent(Fl_Widget *w, void *data)
{
    M_OpenRecentFromMenu(data);
}

static void file_do_quit(Fl_Widget *w, void *data)
{
    ExecuteCommand("Quit");
}

//------------------------------------------------------------------------
//  EDIT MENU
//------------------------------------------------------------------------

static void edit_do_undo(Fl_Widget *w, void *data)
{
    ExecuteCommand("Undo");
}

static void edit_do_redo(Fl_Widget *w, void *data)
{
    ExecuteCommand("Redo");
}

static void edit_do_cut(Fl_Widget *w, void *data)
{
    ExecuteCommand("Clipboard_Cut");
}

static void edit_do_copy(Fl_Widget *w, void *data)
{
    ExecuteCommand("Clipboard_Copy");
}

static void edit_do_paste(Fl_Widget *w, void *data)
{
    ExecuteCommand("Clipboard_Paste");
}

static void edit_do_delete(Fl_Widget *w, void *data)
{
    ExecuteCommand("Delete");
}

static void edit_do_select_all(Fl_Widget *w, void *data)
{
    ExecuteCommand("SelectAll");
}

static void edit_do_unselect_all(Fl_Widget *w, void *data)
{
    ExecuteCommand("UnselectAll");
}

static void edit_do_invert_sel(Fl_Widget *w, void *data)
{
    ExecuteCommand("InvertSelection");
}

static void edit_do_last_sel(Fl_Widget *w, void *data)
{
    ExecuteCommand("LastSelection");
}

static void edit_do_op_menu(Fl_Widget *w, void *data)
{
    ExecuteCommand("OpMenu");
}

static void edit_do_move(Fl_Widget *w, void *data)
{
    ExecuteCommand("MoveObjectsDialog");
}

static void edit_do_scale(Fl_Widget *w, void *data)
{
    ExecuteCommand("ScaleObjectsDialog");
}

static void edit_do_rotate(Fl_Widget *w, void *data)
{
    ExecuteCommand("RotateObjectsDialog");
}

static void edit_do_mirror_horiz(Fl_Widget *w, void *data)
{
    ExecuteCommand("Mirror", "horiz");
}

static void edit_do_mirror_vert(Fl_Widget *w, void *data)
{
    ExecuteCommand("Mirror", "vert");
}

//------------------------------------------------------------------------
//  VIEW MENU
//------------------------------------------------------------------------

static void view_do_zoom_in(Fl_Widget *w, void *data)
{
    ExecuteCommand("Zoom", "+1", "/center");
}

static void view_do_zoom_out(Fl_Widget *w, void *data)
{
    ExecuteCommand("Zoom", "-1", "/center");
}

static void view_do_whole_map(Fl_Widget *w, void *data)
{
    ExecuteCommand("ZoomWholeMap");
}

static void view_do_whole_selection(Fl_Widget *w, void *data)
{
    ExecuteCommand("ZoomSelection");
}

static void view_do_camera_pos(Fl_Widget *w, void *data)
{
    ExecuteCommand("GoToCamera");
}

static void view_do_toggle_3d(Fl_Widget *w, void *data)
{
    ExecuteCommand("Toggle", "3d");
}

static void view_do_object_nums(Fl_Widget *w, void *data)
{
    ExecuteCommand("Toggle", "obj_nums");
}

static void view_do_sprites(Fl_Widget *w, void *data)
{
    ExecuteCommand("Toggle", "sprites");
}

static void view_do_gamma(Fl_Widget *w, void *data)
{
    ExecuteCommand("Toggle", "gamma");
}

static void view_do_default_props(Fl_Widget *w, void *data)
{
    ExecuteCommand("DefaultProps");
}

static void view_do_find(Fl_Widget *w, void *data)
{
    ExecuteCommand("FindDialog");
}

static void view_do_next(Fl_Widget *w, void *data)
{
    ExecuteCommand("FindNext");
}

static void view_do_jump(Fl_Widget *w, void *data)
{
    ExecuteCommand("JumpToObject");
}

//------------------------------------------------------------------------
//  BROWSER MENU
//------------------------------------------------------------------------

static void browser_do_textures(Fl_Widget *w, void *data)
{
    ExecuteCommand("BrowserMode", "T");
}

static void browser_do_flats(Fl_Widget *w, void *data)
{
    ExecuteCommand("BrowserMode", "F");
}

static void browser_do_things(Fl_Widget *w, void *data)
{
    ExecuteCommand("BrowserMode", "O");
}

static void browser_do_lines(Fl_Widget *w, void *data)
{
    ExecuteCommand("BrowserMode", "L");
}

static void browser_do_sectors(Fl_Widget *w, void *data)
{
    ExecuteCommand("BrowserMode", "S");
}

static void browser_do_gen_types(Fl_Widget *w, void *data)
{
    ExecuteCommand("BrowserMode", "G");
}

static void browser_do_recent_tex(Fl_Widget *w, void *data)
{
    ExecuteCommand("BrowserMode", "T", "/recent");
}

static void browser_do_recent_flats(Fl_Widget *w, void *data)
{
    ExecuteCommand("BrowserMode", "F", "/recent");
}

static void browser_do_recent_things(Fl_Widget *w, void *data)
{
    ExecuteCommand("BrowserMode", "O", "/recent");
}

static void browser_hide(Fl_Widget *w, void *data)
{
    ExecuteCommand("Set", "browser", "0");
}

//------------------------------------------------------------------------
//  CHECK MENU
//------------------------------------------------------------------------

static void checks_do_all(Fl_Widget *w, void *data)
{
    ExecuteCommand("MapCheck", "all");
}

static void checks_do_major(Fl_Widget *w, void *data)
{
    ExecuteCommand("MapCheck", "major");
}

static void checks_do_vertices(Fl_Widget *w, void *data)
{
    ExecuteCommand("MapCheck", "vertices");
}

static void checks_do_sectors(Fl_Widget *w, void *data)
{
    ExecuteCommand("MapCheck", "sectors");
}

static void checks_do_linedefs(Fl_Widget *w, void *data)
{
    ExecuteCommand("MapCheck", "linedefs");
}

static void checks_do_things(Fl_Widget *w, void *data)
{
    ExecuteCommand("MapCheck", "things");
}

static void checks_do_textures(Fl_Widget *w, void *data)
{
    ExecuteCommand("MapCheck", "textures");
}

static void checks_do_tags(Fl_Widget *w, void *data)
{
    ExecuteCommand("MapCheck", "tags");
}

//------------------------------------------------------------------------
//  TOOLS MENU
//------------------------------------------------------------------------

static void tools_do_preferences(Fl_Widget *w, void *data)
{
    ExecuteCommand("PreferenceDialog");
}

static void tools_do_build_nodes(Fl_Widget *w, void *data)
{
    ExecuteCommand("BuildAllNodes");
}

static void tools_do_test_map(Fl_Widget *w, void *data)
{
    ExecuteCommand("TestMap");
}

static void tools_do_lump_editor(Fl_Widget *w, void *data)
{
    ExecuteCommand("EditLump");
}

static void tools_do_add_behavior(Fl_Widget *w, void *data)
{
    ExecuteCommand("AddBehavior");
}

static void tools_do_view_logs(Fl_Widget *w, void *data)
{
    ExecuteCommand("LogViewer");
}

static void tools_do_recalc_sectors(Fl_Widget *w, void *data)
{
    ExecuteCommand("RecalcSectors");
}

//------------------------------------------------------------------------
//  HELP MENU
//------------------------------------------------------------------------

static void help_do_online_docs(Fl_Widget *w, void *data)
{
    ExecuteCommand("OnlineDocs");
}

static void help_do_about(Fl_Widget *w, void *data)
{
    ExecuteCommand("AboutDialog");
}

//------------------------------------------------------------------------

#define M_GIVEN_FILES  "&Given Files"
#define M_RECENT_FILES "&Recent Files"

#undef FCAL
#define FCAL (Fl_Callback *)

// Note that the spaces at end of some menu strings are there to
// prevent FLTK drawing the key binding hard against the item's
// text.

static Fl_Menu_Item menu_items[] = {{"&File", 0, 0, 0, FL_SUBMENU},

                                    {"&New Project   ", FL_COMMAND + 'n', FCAL file_do_new_project},
                                    {"&Manage Project  ", FL_COMMAND + 'm', FCAL file_do_manage_project},

                                    {"", 0, 0, 0, FL_MENU_DIVIDER | FL_MENU_INACTIVE},

                                    {"&Open Map", FL_COMMAND + 'o', FCAL file_do_open},
                                    {M_GIVEN_FILES, 0, 0, 0, FL_SUBMENU | FL_MENU_INACTIVE},
                                    {0},
                                    {M_RECENT_FILES, 0, 0, 0, FL_SUBMENU | FL_MENU_INACTIVE},
                                    {0},

                                    {"&Save Map", FL_COMMAND + 's', FCAL file_do_save},
                                    {"&Export Map", FL_COMMAND + 'e', FCAL file_do_export},

                                    {"", 0, 0, 0, FL_MENU_DIVIDER | FL_MENU_INACTIVE},

                                    {"&Fresh Map", 0, FCAL file_do_fresh_map},
                                    {"&Copy Map", 0, FCAL file_do_copy_map},
                                    {"Rename Map", 0, FCAL file_do_rename},
                                    {"Delete Map", 0, FCAL file_do_delete},

                                    {"", 0, 0, 0, FL_MENU_DIVIDER | FL_MENU_INACTIVE},

                                    {"&Quit", FL_COMMAND + 'q', FCAL file_do_quit},
                                    {0},

                                    {"&Edit", 0, 0, 0, FL_SUBMENU},

                                    {"&Undo", FL_COMMAND + 'z', FCAL edit_do_undo},
                                    {"&Redo", FL_COMMAND + 'y', FCAL edit_do_redo},

                                    {"", 0, 0, 0, FL_MENU_DIVIDER | FL_MENU_INACTIVE},

                                    {"Cu&t", FL_COMMAND + 'x', FCAL edit_do_cut},
                                    {"&Copy", FL_COMMAND + 'c', FCAL edit_do_copy},
                                    {"&Paste", FL_COMMAND + 'v', FCAL edit_do_paste},
                                    {"&Delete", FL_Delete, FCAL edit_do_delete},

                                    {"", 0, 0, 0, FL_MENU_DIVIDER | FL_MENU_INACTIVE},

                                    {"Select &All", FL_COMMAND + 'a', FCAL edit_do_select_all},
                                    {"Unselect All", FL_COMMAND + 'u', FCAL edit_do_unselect_all},
                                    {"&Invert Selection", FL_COMMAND + 'i', FCAL edit_do_invert_sel},
                                    {"&Last Selection", FL_COMMAND + 'l', FCAL edit_do_last_sel},

                                    {"", 0, 0, 0, FL_MENU_DIVIDER | FL_MENU_INACTIVE},

                                    {"&Operation Menu", FL_F + 1, FCAL edit_do_op_menu},
                                    {"&Move Objects...", FL_F + 2, FCAL edit_do_move},
                                    {"&Scale Objects...", FL_F + 3, FCAL edit_do_scale},
                                    {"Rotate Objects...", FL_F + 4, FCAL edit_do_rotate},

                                    {"", 0, 0, 0, FL_MENU_DIVIDER | FL_MENU_INACTIVE},

                                    {"Mirror &Horizontally", 0, FCAL edit_do_mirror_horiz},
                                    {"Mirror &Vertically", 0, FCAL edit_do_mirror_vert},
                                    {0},

                                    {"&View", 0, 0, 0, FL_SUBMENU},

                                    // Note: FL_Tab cannot be used as a shortcut here, as it
                                    //       invokes FLTK's hard-coded navigation stuff.

                                    {"Toggle S&prites", FL_F + 10, FCAL view_do_sprites},
                                    {"Toggle &Gamma", FL_F + 11, FCAL view_do_gamma},
                                    {"Toggle Object Nums", FL_F + 12, FCAL view_do_object_nums},

                                    {"", 0, 0, 0, FL_MENU_DIVIDER | FL_MENU_INACTIVE},

                                    {"Zoom &In", 0, FCAL view_do_zoom_in},
                                    {"Zoom &Out", 0, FCAL view_do_zoom_out},
                                    {"&Whole Map", 0, FCAL view_do_whole_map},
                                    {"Whole &Selection", 0, FCAL view_do_whole_selection},

                                    {"", 0, 0, 0, FL_MENU_DIVIDER | FL_MENU_INACTIVE},

                                    {"&Default Props  ", FL_COMMAND + 'd', FCAL view_do_default_props},
                                    {"Toggle &3D View", 0, FCAL view_do_toggle_3d},

                                    {"", 0, 0, 0, FL_MENU_DIVIDER | FL_MENU_INACTIVE},

                                    {"&Find / Replace", FL_COMMAND + 'f', FCAL view_do_find},
                                    {"Find &Next", FL_COMMAND + 'g', FCAL view_do_next},
                                    {"Go to &Camera", 0, FCAL view_do_camera_pos},
                                    {"&Jump to Object", 0, FCAL view_do_jump},
                                    {0},

                                    {"&Browser", 0, 0, 0, FL_SUBMENU},

                                    {"&Textures", FL_F + 5, FCAL browser_do_textures},
                                    {"&Flats", FL_F + 6, FCAL browser_do_flats},
                                    {"Thin&gs", FL_F + 7, FCAL browser_do_things},

                                    {"", 0, 0, 0, FL_MENU_DIVIDER | FL_MENU_INACTIVE},

                                    {"&Line Specials", 0, FCAL browser_do_lines},
                                    {"&Sector Types", 0, FCAL browser_do_sectors},
                                    {"&Generalized Types", 0, FCAL browser_do_gen_types},

                                    {"", 0, 0, 0, FL_MENU_DIVIDER | FL_MENU_INACTIVE},

                                    {"&Recent Textures", 0, FCAL browser_do_recent_tex},
                                    {"Recent Flats", 0, FCAL browser_do_recent_flats},
                                    {"Recent Things", 0, FCAL browser_do_recent_things},

                                    {"", 0, 0, 0, FL_MENU_DIVIDER | FL_MENU_INACTIVE},

                                    {"&Hide", 0, FCAL browser_hide},
                                    {0},

                                    {"&Check", 0, 0, 0, FL_SUBMENU},

                                    {"&ALL", FL_F + 9, FCAL checks_do_all},
                                    {"&Major stuff  ", 0, FCAL checks_do_major},

                                    {"", 0, 0, 0, FL_MENU_DIVIDER | FL_MENU_INACTIVE},

                                    {"&Vertices", 0, FCAL checks_do_vertices},
                                    {"&Sectors", 0, FCAL checks_do_sectors},
                                    {"&LineDefs", 0, FCAL checks_do_linedefs},
                                    {"&Things", 0, FCAL checks_do_things},

                                    {"", 0, 0, 0, FL_MENU_DIVIDER | FL_MENU_INACTIVE},

                                    {"Te&xtures", 0, FCAL checks_do_textures},
                                    {"Ta&gs", 0, FCAL checks_do_tags},
                                    {0},

                                    {"&Tools", 0, 0, 0, FL_SUBMENU},

#ifndef __APPLE__ // for macOS it will be in the app menu
                                    {"&Preferences", FL_COMMAND + 'p', FCAL tools_do_preferences},
#endif
                                    {"&View Logs", 0, FCAL tools_do_view_logs},
                                    {"&Recalc Sectors", 0, FCAL tools_do_recalc_sectors},

                                    {"", 0, 0, 0, FL_MENU_DIVIDER | FL_MENU_INACTIVE},

                                    {"&Test in Game", FL_COMMAND + 't', FCAL tools_do_test_map},
                                    {"&Build All Nodes  ", FL_COMMAND + 'b', FCAL tools_do_build_nodes},
                                    {"&Edit Text Lump  ", 0, FCAL tools_do_lump_editor},
                                    {"&Add BEHAVIOR Lump  ", 0, FCAL tools_do_add_behavior},
                                    {0},

                                    {"&Help", 0, 0, 0, FL_SUBMENU},

                                    {"&Online Docs...", 0, FCAL help_do_online_docs},
                                    {"&About Eureka...", 0, FCAL help_do_about},
                                    {0},

                                    {0}};

//------------------------------------------------------------------------

#define MAX_PWAD_LIST 20

//
// allow the hard-coded menu shortcuts to be bind to other functions
// by the user, hence we remove those shortcuts when this happens.
//
static void Menu_RemovedBoundKeys(Fl_Menu_Item *items)
{
    int total = items[0].size(); // includes {0} at end

    for (int i = 0; i < total; i++)
    {
        if (!items[i].text)
            continue;

        int shortcut = items[i].shortcut_;
        if (!shortcut)
            continue;

        // convert to a Eureka key code
        keycode_t key = shortcut & FL_KEY_MASK;

        if (shortcut & FL_COMMAND)
            key |= MOD_COMMAND;
        else if (shortcut & FL_META)
            key |= MOD_META;
        else if (shortcut & FL_ALT)
            key |= MOD_ALT;

        if (M_IsKeyBound(key, KCTX_General))
            items[i].shortcut_ = 0;
    }
}

//
// remove the blank menu items with FL_MENU_DIVIDER flag, and
// setting the flag on the previous item.  those blank lines
// make for nicer looking menus in Windows/Linux, but are not
// needed for the MacOS system menu bar.
//
void Menu_PackForMac(Fl_Menu_Item *src)
{
    int depth = 0;

    Fl_Menu_Item *dest = src;

    for (;;)
    {
        if (src->text == NULL)
        {
            *dest++ = *src++;

            if (depth == 0)
                break;

            depth--;
            continue;
        }

        if (src->flags & FL_SUBMENU)
        {
            *dest++ = *src++;

            depth++;
            continue;
        }

        if (src->text[0] == 0 && (src->flags & FL_MENU_DIVIDER))
        {
            dest[-1].flags |= FL_MENU_DIVIDER;
            src++;
            continue;
        }

        *dest++ = *src++;
    }
}

static int Menu_FindItem(const Fl_Menu_Item *items, const char *text)
{
    int total = items[0].size(); // includes {0} at end

    for (int i = 0; i < total; i++)
        if (items[i].text && y_stricmp(items[i].text, text) == 0)
            return i;

    return -1; // not found
}

static void Menu_CopyItem(Fl_Menu_Item *&pos, const Fl_Menu_Item &orig)
{
    memcpy(pos, &orig, sizeof(orig));

    pos++;
}

static void Menu_AddItem(Fl_Menu_Item *&pos, const char *text, Fl_Callback *cb, void *data, int flags)
{
    Fl_Menu_Item item;

    memset(&item, 0, sizeof(item));

    item.text       = text;
    item.flags      = flags;
    item.callback_  = cb;
    item.user_data_ = data;

    Menu_CopyItem(pos, item);
}

static Fl_Menu_Item *Menu_PopulateGivenFiles(Fl_Menu_Item *items)
{
    int count = (int)Pwad_list.size();

    if (count < 2)
        return items;

    // silently ignore excess pwads
    if (count > MAX_PWAD_LIST)
        count = MAX_PWAD_LIST;

    // find Given Files sub-menu and activate it
    int menu_pos = Menu_FindItem(items, M_GIVEN_FILES);

    if (menu_pos < 0) // [should not happen]
        return items;

    items[menu_pos++].activate();

    // create new array
    int total = items[0].size(); // includes {0} at end

    Fl_Menu_Item *new_array = new Fl_Menu_Item[total + count];
    Fl_Menu_Item *pos       = new_array;

    for (int i = 0; i < menu_pos; i++)
        Menu_CopyItem(pos, items[i]);

    for (int k = 0; k < count; k++)
    {
        const char *short_name = fl_filename_name(Pwad_list[k]);

        short_name = StringPrintf("%s%s%d:  %s", (k < 9) ? "  " : "", (k < 9) ? "&" : "", 1 + k, short_name);

        Menu_AddItem(pos, short_name, FCAL file_do_load_given, (void *)Pwad_list[k], 0);
    }

    for (; menu_pos < total; menu_pos++)
        Menu_CopyItem(pos, items[menu_pos]);

    return new_array;
}

static Fl_Menu_Item *Menu_PopulateRecentFiles(Fl_Menu_Item *items, Fl_Callback *cb)
{
    int count = M_RecentCount();

    if (count < 1)
        return items;

    // find Recent Files sub-menu and activate it
    int menu_pos = Menu_FindItem(items, M_RECENT_FILES);

    if (menu_pos < 0) // [should not happen]
        return items;

    items[menu_pos++].activate();

    // create new array
    int total = items[0].size(); // includes {0} at end

    Fl_Menu_Item *new_array = new Fl_Menu_Item[total + count];
    Fl_Menu_Item *pos       = new_array;

    for (int i = 0; i < menu_pos; i++)
        Menu_CopyItem(pos, items[i]);

    for (int k = 0; k < count; k++)
    {
        std::string name = M_RecentShortName(k);

        void *data = M_RecentData(k);

        Menu_AddItem(pos, StringDup(name.c_str()), cb, data, 0);
    }

    for (; menu_pos < total; menu_pos++)
        Menu_CopyItem(pos, items[menu_pos]);

    return new_array;
}

Fl_Sys_Menu_Bar *Menu_Create(int x, int y, int w, int h)
{
    Fl_Sys_Menu_Bar *bar = new Fl_Sys_Menu_Bar(x, y, w, h);

    Fl_Menu_Item *items = menu_items;

#ifdef __APPLE__
    Menu_PackForMac(items);
#else
    bar->textsize(16);
#endif

    Menu_RemovedBoundKeys(items);

    items = Menu_PopulateGivenFiles(items);
    items = Menu_PopulateRecentFiles(items, FCAL file_do_load_recent);

    bar->menu(items);

    // for macOS, the preferences shall be in the app menu
#ifdef __APPLE__
    static const Fl_Menu_Item macPreferencesItem = {"&Preferences\u2026", FL_COMMAND + ',', FCAL tools_do_preferences};
    Fl_Mac_App_Menu::custom_application_menu_items(&macPreferencesItem);
#endif

    return bar;
}

#endif

} // namespace smc