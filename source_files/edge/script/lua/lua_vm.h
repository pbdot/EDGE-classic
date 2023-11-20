#pragma once

namespace elua
{
    class lua_vm_c;    
}

void LUA_DoFile(elua::lua_vm_c* vm, const std::string& name);

elua::lua_vm_c* LUA_CreateVM(const char* name);