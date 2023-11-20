
#include "elua.h"

namespace elua
{

std::unordered_map<std::string, lua_vm_c *> lua_vm_c::vms_;
std::unordered_map<lua_State*, lua_vm_c *> lua_vm_c::vm_state_lookup_;

void lua_vm_c::DoString(const char* source)
{
    int ret = luaL_dostring(state_, source);

    if (ret != 0)
    {
        I_Warning("LUA: %s\n", lua_tostring(state_, -1));
    }
}

void lua_vm_c::Open(lua_CFunction searcher)
{
    SYS_ASSERT(!state_);

    // we could specify a lua allocator, which would be a good idea to hook up to a debug allocator
    // library for tracing l = lua_newstate(lua_Alloc alloc, nullptr);

    state_ = luaL_newstate();

    /*
    ** these libs are loaded by lua.c and are readily available to any Lua
    ** program
    */
    const luaL_Reg loadedlibs[] = {
        {LUA_GNAME, luaopen_base},          {LUA_LOADLIBNAME, luaopen_package}, // todo: remove sandboxing
        {LUA_COLIBNAME, luaopen_coroutine}, {LUA_TABLIBNAME, luaopen_table},    {LUA_IOLIBNAME, luaopen_io},
        {LUA_OSLIBNAME, luaopen_os},        {LUA_STRLIBNAME, luaopen_string},   {LUA_MATHLIBNAME, luaopen_math},
        {LUA_UTF8LIBNAME, luaopen_utf8},    {LUA_DBLIBNAME, luaopen_debug},     {NULL, NULL}};

    const luaL_Reg *lib;
    /* "require" functions from 'loadedlibs' and set results to global table */
    for (lib = loadedlibs; lib->func; lib++)
    {
        luaL_requiref(state_, lib->name, lib->func, 1);
        lua_pop(state_, 1); /* remove lib */
    }

#if LUABRIDGE_HAS_EXCEPTIONS
    luabridge::enableExceptions(state_);
#endif

    // replace searchers with only preload and custom searcher
    int top = lua_gettop(state_);
    lua_getglobal(state_, "package");
    lua_getfield(state_, -1, "searchers");
    lua_newtable(state_);
    lua_geti(state_, -2, 1);
    lua_seti(state_, -2, 1);
    lua_pushcfunction(state_, searcher);
    lua_seti(state_, -2, 2);
    lua_setfield(state_, -3, "searchers");
    // pop package and searchers off stack
    lua_pop(state_, 2);
}

void lua_vm_c::Close()
{
    if (!state_)
    {
        return;
    }

    lua_close(state_);
    state_ = nullptr;
}

} // namespace elua