
#include "lua_compat.h"

void LUA_NewGame(void)
{
    //VM_CallFunction(ui_vm, "new_game");
}

void LUA_LoadGame(void)
{
    // Need to set these to prevent NULL references if using any player.xxx in the load_level hook
    //ui_hud_who    = players[displayplayer];
    //ui_player_who = players[displayplayer];

    //VM_CallFunction(ui_vm, "load_game");
}

void LUA_SaveGame(void)
{
    //VM_CallFunction(ui_vm, "save_game");
}

void LUA_BeginLevel(void)
{
    // Need to set these to prevent NULL references if using player.xxx in the begin_level hook
    //ui_hud_who    = players[displayplayer];
    //ui_player_who = players[displayplayer];
    //VM_CallFunction(ui_vm, "begin_level");
}

void LUA_EndLevel(void)
{
    //VM_CallFunction(ui_vm, "end_level");
}

