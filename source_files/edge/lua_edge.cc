
#include "lua_edge.h"
#include "lua_coal_vm.h"

using namespace elua;

void LUA_Sys_Init(lua_vm_c *vm);
void LUA_Hud_Init();
void LUA_Coal_Init();

// todo: make this create coal compatibility vm
lua_vm_c* LUA_CreateEdgeVM(lua_vm_edge_t vm_type)
{
    lua_coal_vm_c* vm = lua_vm_c::Create<lua_coal_vm_c>(vm_type);
    
    LUA_Sys_Init(vm);

    return vm;
}

void LUA_Init()
{
    LUA_Hud_Init();
    LUA_Coal_Init();
}
