
#include "elua.h"

namespace elua {
std::unordered_map<lua_vm_id, lua_vm_c*> lua_vm_c::vms_;

void lua_vm_c::DoFile(const std::string& filename)
{
    int ret = luaL_dofile(state_, filename.c_str());

    if (ret != 0)
    {
        I_Warning("LUA: %s\n", lua_tostring(state_, -1));
    }
}

void lua_vm_c::DoString(const std::string& source)
{
    int ret = luaL_dostring(state_, source.c_str());

    if (ret != 0)
    {
        I_Warning("LUA: %s\n", lua_tostring(state_, -1));
    }
}

void lua_vm_c::Open()
{
    SYS_ASSERT(!state_);

    // we could specify a lua allocator, which would be a good idea to hook up to a debug allocator
    // library for tracing l = lua_newstate(lua_Alloc alloc, nullptr);

    state_ = luaL_newstate();

    /*
    ** these libs are loaded by lua.c and are readily available to any Lua
    ** program
    */
    const luaL_Reg loadedlibs[] = {{LUA_GNAME, luaopen_base},
                                   {LUA_LOADLIBNAME, luaopen_package}, // todo: remove sandboxing
                                   {LUA_COLIBNAME, luaopen_coroutine},
                                   {LUA_TABLIBNAME, luaopen_table},
                                   {LUA_IOLIBNAME, luaopen_io},
                                   {LUA_OSLIBNAME, luaopen_os},
                                   {LUA_STRLIBNAME, luaopen_string},
                                   {LUA_MATHLIBNAME, luaopen_math}, // must be opened in vm subclass, due to collisions with COAL compatibility
                                   {LUA_UTF8LIBNAME, luaopen_utf8},
                                   {LUA_DBLIBNAME, luaopen_debug},
                                   {NULL, NULL}};

    const luaL_Reg* lib;
    /* "require" functions from 'loadedlibs' and set results to global table */
    for (lib = loadedlibs; lib->func; lib++)
    {
        luaL_requiref(state_, lib->name, lib->func, 1);
        lua_pop(state_, 1); /* remove lib */
    }

    DoFile("C:/Dev/EDGE-classic-typescript/edge_defs/lua/init.lua");    

    int result = luaL_dostring(
        state_, "package.path = 'C:/Dev/EDGE-classic-typescript/edge_defs/lua/?.lua'");
    lua_pop(state_, result);

#if LUABRIDGE_HAS_EXCEPTIONS
    luabridge::enableExceptions(state_);
#endif

    refRequire_ = luabridge::getGlobal(state_, "require");
    SYS_ASSERT(refRequire_.isFunction());
}

luabridge::LuaResult lua_vm_c::Require(const std::string& path)
{
    return refRequire_(path.c_str());
}

void lua_vm_c::Close()
{
    if (state_)
    {
        lua_close(state_);
        state_ = nullptr;
    }
}
} // namespace elua