
#include "i_defs.h"
#include "e_player.h"
#include "hu_draw.h"
#include "lua_compat.h"

player_t *ui_hud_who = nullptr;

static int HUD_coord_sys(lua_State *L)
{
    double w = luaL_checknumber(L, 1);
    double h = luaL_checknumber(L, 2);

    if (w < 64 || h < 64)
        I_Error("Bad hud.coord_sys size: %dx%d\n", w, h);

    HUD_SetCoordSys(w, h);

    // VM_SetFloat(ui_vm, "hud", "x_left", hud_x_left);
    // VM_SetFloat(ui_vm, "hud", "x_right", hud_x_right);

    return 0;
}


static int HUD_render_world(lua_State *L)
{
    float x     = (float)luaL_checknumber(L, 1);
    float y     = (float)luaL_checknumber(L, 2);
    float w     = (float)luaL_checknumber(L, 3);
    float h     = (float)luaL_checknumber(L, 4);
    int   flags = (int)luaL_optnumber(L, 5, 0);

    HUD_RenderWorld(x, y, w, h, ui_hud_who->mo, flags);

    return 0;
}

static const luaL_Reg hudlib[] = { {"coord_sys", HUD_coord_sys}, {"render_world", HUD_render_world}, {NULL, NULL}};

static int luaopen_hud(lua_State *L)
{
    luaL_newlib(L, hudlib);
    return 1;
}

void LUA_RegisterHudLibrary(lua_State *L)
{
    luaL_requiref(L, "hud", luaopen_hud, 1);
    lua_pop(L, 1);
}

void LUA_RunHud(void)
{
    HUD_Reset();

    ui_hud_who    = players[displayplayer];
    //ui_player_who = players[displayplayer];

    LUA_CallGlobalFunction(global_lua_state, "draw_all");

    HUD_Reset();
}
