#pragma once

#include "epi.h"
#include "lua.hpp"

lua_State *LUA_CreateVM();

void LUA_Init();
void LUA_AddScript(const std::string &data, const std::string &source);
void LUA_LoadScripts();

void LUA_DoFile(lua_State *L, const std::string &name);
void LUA_DoString(lua_State *L, const char *source);
void LUA_CallGlobalFunction(lua_State *L, const char *function_name);

void LUA_RunHud(void);
void LUA_RegisterHudLibrary(lua_State *L);

extern lua_State *global_lua_state;