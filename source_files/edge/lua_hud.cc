

#include "lua_edge.h"

using namespace elua;

extern std::string w_map_title;

static lua_vm_c *vm_hud = nullptr;

class lua_hud_c : public lua_module_c
{
public:
    lua_hud_c(lua_vm_c *vm) : lua_module_c(vm, "hud"), hud_(nullptr) {}

    void Open()
    {
        lua_State *state = vm_->GetState();

        luabridge::getGlobalNamespace(state)
            .beginNamespace("ec")
            .beginNamespace("hud")
            .addVariable("mapTitle", w_map_title)
            .endNamespace()
            .endNamespace();

        /*
        luabridge::LuaResult result = vm_hud->Require("test");
        hud_ = result[0]["hud"];

        hud_["say_hello"](hud_, 42);

        vm_hud->Require("importtest");
        */
    }

    luabridge::LuaRef hud_;
};

void LUA_Hud_Init()
{
    SYS_ASSERT(!vm_hud);
    vm_hud = LUA_CreateEdgeVM(LUA_VM_EDGE_HUD);
    vm_hud->AddModule<lua_hud_c>();

    // vm_hud->DoFile("script/test.lua");
}