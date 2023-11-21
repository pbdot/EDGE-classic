#include "i_defs.h"

#include "file.h"
#include "filesystem.h"
#include "path.h"

#include "main.h"

#include "vm_coal.h"
#include "dm_state.h"
#include "e_main.h"
#include "g_game.h"
#include "version.h"

#include "e_player.h"
#include "hu_font.h"
#include "hu_draw.h"
#include "r_modes.h"
#include "w_wad.h"

#include "m_random.h"

#include "lua_compat.h"

//------------------------------------------------------------------------
//  SYSTEM MODULE
//------------------------------------------------------------------------

// sys.error(str)
//
static int SYS_error(lua_State *L)
{
    const char *s = luaL_checkstring(L, 0);
    I_Error("%s\n", s);
    return 0;
}

// sys.print(str)
//
static int SYS_print(lua_State *L)
{
    const char *s = luaL_checkstring(L, 0);
    I_Printf("%s\n", s);
    return 0;
}

// sys.debug_print(str)
//
static int SYS_debug_print(lua_State *L)
{
    const char *s = luaL_checkstring(L, 0);
    I_Debugf("%s\n", s);
    return 0;
}

// sys.edge_version()
//
static int SYS_edge_version(lua_State *L)
{
    lua_pushnumber(L, edgeversion.f);
    return 1;
}

//------------------------------------------------------------------------
//  ECMATH MODULE
//------------------------------------------------------------------------

// ecmath.random()
static int ECMATH_random(lua_State *L)
{
    lua_pushnumber(L, C_Random() / double(0x10000));
    return 1;
}

// Lobo November 2021: ecmath.random2() always between 0 and 10
static int ECMATH_random2(lua_State *L)
{
    lua_pushnumber(L, C_Random() % 11);
    return 1;
}

// SYSTEM
static const luaL_Reg syslib[] = {{"error", SYS_error},
                                  {"print", SYS_print},
                                  {"debug_print", SYS_debug_print},
                                  {"edge_version", SYS_edge_version},
                                  {NULL, NULL}};

// ECMATH
static const luaL_Reg ecmathlib[] = {{"random", ECMATH_random}, {"random2", ECMATH_random2}, {NULL, NULL}};

static int luaopen_sys(lua_State *L)
{
    luaL_newlib(L, syslib);
    return 1;
}

static int luaopen_ecmath(lua_State *L)
{
    luaL_newlib(L, ecmathlib);
    return 1;
}

const luaL_Reg loadlibs[] = {{"sys", luaopen_sys}, {"ecmath", luaopen_ecmath}};

void LUA_RegisterCoreLibraries(lua_State *L)
{
    const luaL_Reg *lib;
    /* "require" functions from 'loadedlibs' and set results to global table */
    for (lib = loadlibs; lib->func; lib++)
    {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1); /* remove lib */
    }
}