
#include "elua.h"
#include "i_defs.h"
#include "e_player.h"

#include "../../common/script_hud.h"

using namespace script_common;
using namespace elua;

static int LC_get_time(lua_State *L)
{
    lua_pushnumber(L, SC_get_time());
    return 1;
}

static const luaL_Reg hudlib[] = {{"get_time", LC_get_time}, {NULL, NULL}};

static int luaopen_hud(lua_State *L)
{
    luaL_newlib(L, hudlib);
    return 1;
}

class lua_hud : public lua_module_c
{
  public:
    const std::string &GetName() override
    {
        return name_;
    }

    lua_hud(lua_vm_c *vm) : lua_module_c(vm)
    {
    }

    void Open() override
    {
        lua_State *L = vm_->GetState();
        luaL_requiref(L, name_.c_str(), luaopen_hud, 1);
    }

    std::string name_ = "hud";
};

void LUA_OpenHud(lua_vm_c *vm)
{
    vm->AddModule<lua_hud>();
}