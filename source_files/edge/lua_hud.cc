
#include "lua_edge.h"

using namespace elua;

static lua_vm_c *vm_hud = nullptr;

class lua_hud_c : public lua_module_c
{
public:
    lua_hud_c(lua_vm_c *vm) : lua_module_c(vm, "hud") {}

    void Open()
    {
        
    }
};

void LUA_Hud_Init()
{
    SYS_ASSERT(!vm_hud);

    vm_hud = lua_vm_c::Create(LUA_VM_EDGE_HUD);
    vm_hud->AddModule<lua_hud_c>();
}