#pragma once

namespace elua
{
    class lua_vm_c;    
}

elua::lua_vm_c* LUA_CreateVM(const char* name);