#pragma once

#include "elua.h"

enum lua_vm_edge_t : unsigned int 
{ 
    LUA_VM_EDGE_HUD,
    LUA_VM_EDGE_PLAYSIM,
    LUA_VM_EDGE_DATA,
    LUA_VM__EDGE_MAX
};