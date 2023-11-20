#include "i_defs.h"
#include "version.h"
#include "elua.h"
#include "../lua_vm.h"
#include "lua_compat.h"

using namespace elua;

//------------------------------------------------------------------------
//  SYSTEM MODULE
//------------------------------------------------------------------------

// sys.error(str)
//
static int SYS_error(lua_State *L)
{
    const char *s = luaL_checkstring(L, 1);

    I_Error("%s\n", s);

    return 0;
}

// sys.print(str)
//
static int SYS_print(lua_State *L)
{
    const char *s = luaL_checkstring(L, 1);

    I_Printf("%s\n", s);

    return 0;
}

// sys.debug_print(str)
//
static int SYS_debug_print(lua_State *L)
{
    const char *s = luaL_checkstring(L, 1);

    I_Debugf("%s\n", s);

    return 0;
}

// sys.edge_version()
//
static int SYS_edge_version(lua_State *L)
{
    lua_pushnumber(L, edgeversion.f);
    return 1;    
}

static const luaL_Reg syslib[] = {
    {"error", SYS_error}, {"print", SYS_print}, {"debug_pring", SYS_debug_print}, {"edge_version", SYS_edge_version}, {NULL, NULL}};

static int luaopen_sys(lua_State *L)
{
    luaL_newlib(L, syslib);
    return 1;
}


class lua_sys_c : public lua_module_c
{
  public:
    const std::string &GetName() override
    {
        return name_;
    }

    lua_sys_c(lua_vm_c *vm) : lua_module_c(vm)
    {
    }

    void Open() override
    {
        lua_State *L = vm_->GetState();
        luaL_requiref(L, name_.c_str(), luaopen_sys, 1);
        lua_pop(L, 1);

        LUA_DoFile(vm_, "scripts/lua/compat/lua_sys.lua");
    }

    std::string name_ = "sys";
};

void LUA_OpenSys()
{
    lua_vm_c* vm = lua_vm_c::GetVM(VM_COAL_COMPAT);
    SYS_ASSERT(vm);
    vm->AddModule<lua_sys_c>();    
}