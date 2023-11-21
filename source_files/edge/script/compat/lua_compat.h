#pragma once

#include "epi.h"
#include "math_vector.h"
#include "lua.hpp"

lua_State *LUA_CreateVM();

void LUA_Init();
void LUA_AddScript(const std::string &data, const std::string &source);
void LUA_LoadScripts();

void LUA_DoFile(lua_State *L, const std::string &name);
void LUA_DoString(lua_State *L, const char *source);
void LUA_CallGlobalFunction(lua_State *L, const char *function_name);

// Game
void LUA_NewGame(void);
void LUA_LoadGame(void);
void LUA_SaveGame(void);
void LUA_BeginLevel(void);
void LUA_EndLevel(void);

// Core
void LUA_RegisterCoreLibraries(lua_State* L);

// Player
void LUA_RegisterPlayerLibrary(lua_State *L);

// Hud
void LUA_RunHud(void);
void LUA_RegisterHudLibrary(lua_State *L);

inline epi::vec3_c LUA_CheckVector3(lua_State *L, int index)
{
    epi::vec3_c v;
    SYS_ASSERT(lua_istable(L, index));
    lua_geti(L, index, 1);
    v.x = luaL_checknumber(L, -1);
    lua_geti(L, index, 2);
    v.y = luaL_checknumber(L, -1);
    lua_geti(L, index, 3);
    v.z = luaL_checknumber(L, -1);
    lua_pop(L, 3);
    return v;
}

inline void LUA_PushVector3(lua_State*L, epi::vec3_c v)
{
    lua_getglobal(L, "vec3");
    lua_pushnumber(L, v.x);
    lua_pushnumber(L, v.y);
    lua_pushnumber(L, v.z);
    lua_call(L, 3, 0);
}

extern lua_State *global_lua_state;