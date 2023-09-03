
#include "lua_coal_vm.h"

void lua_coal_vm_c::Open()
{
    elua::lua_vm_c::Open();

    luaL_requiref(state_, "__math", luaopen_math, 1);
    lua_pop(state_, 1); 

    luabridge::LuaResult rvec3 = Require("vec3");
    SYS_ASSERT(rvec3.wasOk());
    SYS_ASSERT(rvec3.size() > 0);
    SYS_ASSERT(rvec3[0].isCallable());

    luabridge::setGlobal(state_, rvec3[0], "vec3");
}