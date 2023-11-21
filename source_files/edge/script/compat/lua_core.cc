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

// ecmath.rint(val)
static int ECMATH_rint(lua_State *L)
{
    double val = luaL_checknumber(L, 1);
    lua_pushnumber(L, I_ROUND(val));
    return 1;
}

// ecmath.floor(val)
static int ECMATH_floor(lua_State *L)
{
    double val = luaL_checknumber(L, 1);
    lua_pushnumber(L, floor(val));
    return 1;
}

// ecmath.ceil(val)
static int ECMATH_ceil(lua_State *L)
{
    double val = luaL_checknumber(L, 1);
    lua_pushnumber(L, ceil(val));
    return 1;
}

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

// ecmath.cos(val)
static int ECMATH_cos(lua_State *L)
{
    double val = luaL_checknumber(L, 1);
    lua_pushnumber(L, cos(val * M_PI / 180.0));
    return 1;
}

// ecmath.sin(val)
static int ECMATH_sin(lua_State *L)
{
    double val = luaL_checknumber(L, 1);
    lua_pushnumber(L, sin(val * M_PI / 180.0));
    return 1;
}

// ecmath.tan(val)
static int ECMATH_tan(lua_State *L)
{
    double val = luaL_checknumber(L, 1);
    lua_pushnumber(L, tan(val * M_PI / 180.0));
    return 1;
}

// ecmath.acos(val)
static int ECMATH_acos(lua_State *L)
{
    double val = luaL_checknumber(L, 1);
    lua_pushnumber(L, acos(val) * 180.0 / M_PI);
    return 1;
}

// ecmath.asin(val)
static int ECMATH_asin(lua_State *L)
{
    double val = luaL_checknumber(L, 1);
    lua_pushnumber(L, asin(val) * 180.0 / M_PI);
    return 1;
}

// ecmath.atan(val)
static int ECMATH_atan(lua_State *L)
{
    double val = luaL_checknumber(L, 1);
    lua_pushnumber(L, atan(val) * 180.0 / M_PI);
    return 1;
}

// ecmath.atan2(x, y)
static int ECMATH_atan2(lua_State *L)
{
    double x = luaL_checknumber(L, 1);
    double y = luaL_checknumber(L, 2);

    lua_pushnumber(L, atan2(y, x) * 180.0 / M_PI);
    return 1;
}

// ecmath.log(val)
static int ECMATH_log(lua_State *L)
{
    double val = luaL_checknumber(L, 1);

    if (val <= 0)
        I_Error("ecmath.log: illegal input: %g\n", val);

    lua_pushnumber(L, log(val));
    return 1;
}

//------------------------------------------------------------------------
//  STRINGS MODULE
//------------------------------------------------------------------------

// strings.len(s)
//
static int STRINGS_len(lua_State *L)
{
    const char *s = luaL_checkstring(L, 0);

    lua_pushnumber(L, strlen(s));
    return 1;
}

// Lobo: December 2021
//  strings.find(s,TextToFind)
//  returns substring position or -1 if not found
//
static int STRINGS_find(lua_State *L)
{
    const char *s1 = luaL_checkstring(L, 0);
    const char *s2 = luaL_checkstring(L, 1);

    std::string str(s1);
    std::string str2(s2);

    int found = str.find(str2);

    lua_pushnumber(L, found);
    return 1;
}

// strings.sub(s, start, end)
//
static int STRINGS_sub(lua_State *L)
{
    const char *s = luaL_checkstring(L, 0);

    int start = (int)luaL_checknumber(L, 2);
    int end   = (int)luaL_checknumber(L, 3);
    int len   = strlen(s);

    // negative values are relative to END of the string (-1 = last character)
    if (start < 0)
        start += len + 1;
    if (end < 0)
        end += len + 1;

    if (start < 1)
        start = 1;
    if (end > len)
        end = len;

    if (end < start)
    {
        lua_pushstring(L, "");
        return 1;
    }

    SYS_ASSERT(end >= 1 && start <= len);

    // translate into C talk
    start--;
    end--;

    int new_len = (end - start + 1);

    lua_pushlstring(L, s + start, new_len);
    return 1;
}

// strings.tonumber(s)
//
static int STRINGS_tonumber(lua_State *L)
{
    const char *s = luaL_checkstring(L, 0);

    lua_pushnumber(L, atof(s));
    return 1;
}

// SYSTEM
static const luaL_Reg syslib[] = {{"error", SYS_error},
                                  {"print", SYS_print},
                                  {"debug_print", SYS_debug_print},
                                  {"edge_version", SYS_edge_version},
                                  {NULL, NULL}};

// ECMATH
static const luaL_Reg ecmathlib[] = {{"rint", ECMATH_rint},     {"floor", ECMATH_floor},     {"ceil", ECMATH_ceil},
                                     {"random", ECMATH_random}, {"random2", ECMATH_random2},

                                     {"cos", ECMATH_cos},       {"sin", ECMATH_sin},         {"tan", ECMATH_tan},
                                     {"acos", ECMATH_acos},     {"asin", ECMATH_asin},       {"atan", ECMATH_atan},
                                     {"atan2", ECMATH_atan2},   {"log", ECMATH_log},         {NULL, NULL}};

// STRINGS
static const luaL_Reg stringslib[] = {
    {"len", STRINGS_len}, {"sub", STRINGS_sub}, {"tonumber", STRINGS_tonumber}, {"find", STRINGS_find}, {NULL, NULL}};

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

static int luaopen_strings(lua_State *L)
{
    luaL_newlib(L, stringslib);
    return 1;
}

const luaL_Reg loadlibs[] = {
    {"sys", luaopen_sys}, {"ecmath", luaopen_ecmath}, {"strings", luaopen_strings}, {NULL, NULL}};


void LUA_RegisterCoreLibraries(lua_State* L)
{
    const luaL_Reg *lib;
    /* "require" functions from 'loadedlibs' and set results to global table */
    for (lib = loadlibs; lib->func; lib++)
    {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1); /* remove lib */
    }
}