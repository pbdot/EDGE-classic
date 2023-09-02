#pragma once

#include "elua.h"

enum lua_vm_edge_t : unsigned int 
{ 
    LUA_VM_EDGE_HUD,
    LUA_VM_EDGE_PLAYSIM,
    LUA_VM_EDGE_DATA,
    // COAL Compatibility VM
    LUA_VM_EDGE_COAL,
    LUA_VM__EDGE_MAX
};

elua::lua_vm_c* LUA_CreateEdgeVM(lua_vm_edge_t vm_type);