
#include "elua.h"
#include "i_defs.h"
#include "e_player.h"
#include "w_files.h"
#include  "../lua_vm.h"
#include "lua_compat.h"
#include "../../common/script_hud.h"
#include "hu_draw.h"

using namespace script_common;
using namespace elua;

static int LH_coord_sys(lua_State *L)
{
    double w = luaL_checknumber(L, 1);
    double h = luaL_checknumber(L, 2);

    if (w < 64 || h < 64)
        I_Error("Bad hud.coord_sys size: %dx%d\n", w, h);

    HUD_SetCoordSys(w, h);

    // VM_SetFloat(ui_vm, "hud", "x_left", hud_x_left);
    // VM_SetFloat(ui_vm, "hud", "x_right", hud_x_right);

    return 0;
}

static int LH_render_world(lua_State *L)
{
    float x     = (float)luaL_checknumber(L, 1);
    float y     = (float)luaL_checknumber(L, 2);
    float w     = (float)luaL_checknumber(L, 3);
    float h     = (float)luaL_checknumber(L, 4);
    int   flags = (int)luaL_optnumber(L, 5, 0);

    SC_render_world(x, y, w, h, flags);

    return 0;
}

static int LH_get_time(lua_State *L)
{
    lua_pushnumber(L, SC_get_time());
    return 1;
}

static const luaL_Reg hudlib[] = {
    {"get_time", LH_get_time}, {"coord_sys", LH_coord_sys}, {"render_world", LH_render_world}, {NULL, NULL}};

static int luaopen_hud(lua_State *L)
{
    luaL_newlib(L, hudlib);
    return 1;
}

class lua_hud_c : public lua_module_c
{
  public:
    const std::string &GetName() override
    {
        return name_;
    }

    lua_hud_c(lua_vm_c *vm) : lua_module_c(vm)
    {
    }

    void Open() override
    {
        lua_State *L = vm_->GetState();
        luaL_requiref(L, name_.c_str(), luaopen_hud, 1);
        lua_pop(L, 1);

        LUA_DoFile(vm_, "scripts/lua/compat/lua_hud.lua");
    }

    std::string name_ = "hud";
};

void LUA_RunHud()
{
    lua_vm_c* vm = lua_vm_c::GetVM(VM_COAL_COMPAT);
    lua_State *L = vm->GetState();
    lua_getglobal(L, "draw_all");
    lua_call(L, 0, 0);
}

void LUA_OpenHud()
{
    lua_vm_c* vm = lua_vm_c::GetVM(VM_COAL_COMPAT);
    SYS_ASSERT(vm);
    vm->AddModule<lua_hud_c>();    
}