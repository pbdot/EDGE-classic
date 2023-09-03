
#include "lua_coal_vm.h"

void lua_coal_vm_c::Open()
{
    elua::lua_vm_c::Open();

    luabridge::LuaResult rvec3 = Require("vec3");
    SYS_ASSERT(rvec3.wasOk());
    SYS_ASSERT(rvec3.size() > 0);
    SYS_ASSERT(rvec3[0].isCallable());

    luabridge::setGlobal(state_, rvec3[0], "vec3");
}

luabridge::LuaRef LUA_Coal_NewVec3(lua_State* state, double x, double y, double z)
{
    luabridge::LuaRef vec3 = luabridge::getGlobal(state, "vec3");
    SYS_ASSERT(vec3.isCallable());
    return vec3(x, y, z)[0];
}
