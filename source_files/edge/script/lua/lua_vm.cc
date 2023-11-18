#include "elua.h"
#include "i_system.h"
#include "str_util.h"
#include "w_files.h"

using namespace elua;

static void LUA_GetRequirePackPath(const char *name, std::string &out)
{
    std::string require_name(name);
    std::replace(require_name.begin(), require_name.end(), '.', '/');
    out = epi::STR_Format("scripts/lua/%s.lua", require_name.c_str());
}

static int LUA_PackLoader(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);

    std::string pack_name;
    LUA_GetRequirePackPath(name, pack_name);

    epi::file_c *file = W_OpenPackFile(pack_name);

    if (!file)
    {
        return 0;
    }

    // TODO: Fix me ReadText is copying string on return, and should be taking a reference
    std::string source = file->ReadText();

    delete file;

    int result = luaL_dostring(L, source.c_str());
    if (result != LUA_OK)
    {
        I_Error("LUA: %s.lua: %s\n", name, lua_tostring(L, -1));
        lua_pop(L, 1);
        return 0;
    }

    return 1;
}

static int LUA_PackSearcher(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);

    std::string pack_name;
    LUA_GetRequirePackPath(name, pack_name);

    // TODO: This isn't working for some reason, better not to allocate a file
    /*
    if (W_CheckPackForName(pack_name) == -1)
    {
        return 0;
    }
    */

    epi::file_c *file = W_OpenPackFile(pack_name);

    if (!file)
    {
        return 0;
    }

    delete file;

    lua_pushcfunction(L, LUA_PackLoader);
    lua_pushstring(L, name);
    return 2;
}

static void RegisterGlobal(lua_vm_c *vm, const char *name, const char *module_name)
{
    lua_State *L = vm->GetState();
    vm->DoString(epi::STR_Format("return require \"%s\"", module_name).c_str());
    // pop loader data from 
    lua_pop(L, 1);
    lua_setglobal(L, name);
}

lua_vm_c *LUA_CreateVM(const char *name)
{
    lua_vm_c *vm = lua_vm_c::Create(name, LUA_PackSearcher);
    RegisterGlobal(vm, "vec3", "core.vec3");
    return vm;
}