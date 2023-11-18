

#include "elua.h"
#include "../lua_vm.h"
#include "lua_hud.h"
#include "i_defs.h"
#include "e_main.h"

using namespace elua;

static lua_vm_c *g_vm = nullptr;

void LUA_InitCompat()
{
    E_ProgressMessage("Starting Lua VM...");    

    SYS_ASSERT(!g_vm);
    g_vm = LUA_CreateVM("CoalCompat");
    LUA_OpenHud(g_vm);
}