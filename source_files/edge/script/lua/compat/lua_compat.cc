#include "elua.h"
#include "i_defs.h"
#include "e_main.h"
#include "lua_compat.h"
#include "../lua_vm.h"

using namespace elua;

void LUA_InitCompat()
{
    E_ProgressMessage("Starting Lua VM...");    

    SYS_ASSERT(!lua_vm_c::GetVM(VM_COAL_COMPAT));

    lua_vm_c* vm = LUA_CreateVM(VM_COAL_COMPAT);
    lua_State* L = vm->GetState();

    SYS_ASSERT(lua_gettop(L) == 0);    

    void LUA_OpenSys();
    LUA_OpenSys();
    void LUA_OpenHud();
    LUA_OpenHud();

    SYS_ASSERT(lua_gettop(L) == 0);
}