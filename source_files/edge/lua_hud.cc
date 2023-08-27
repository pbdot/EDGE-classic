
#include "lua_edge.h"

using namespace elua;

extern std::string w_map_title;

static lua_vm_c *vm_hud = nullptr;

class lua_hud_c : public lua_module_c
{
public:
    lua_hud_c(lua_vm_c *vm) : lua_module_c(vm, "hud") {}

    void Open()
    {
        lua_State *state = vm_->GetState();

        luabridge::getGlobalNamespace(state)
            .beginNamespace("ec")
            .beginNamespace("hud")
            .addVariable("mapTitle", w_map_title)
            .endNamespace()
            .endNamespace();
    }
};

void LUA_Hud_Init()
{
    SYS_ASSERT(!vm_hud);
    vm_hud = LUA_CreateEdgeVM(LUA_VM_EDGE_HUD);
    vm_hud->AddModule<lua_hud_c>();

    vm_hud->DoFile("test.lua");
}