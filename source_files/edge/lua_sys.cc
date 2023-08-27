
#include "lua_edge.h"

using namespace elua;

class lua_sys_c : public lua_module_c
{
public:
    lua_sys_c(lua_vm_c *vm) : lua_module_c(vm, "sys") {}

    static int Print(lua_State *L)
    {
        int n = lua_gettop(L);
        for (int i = 1; i <= n; i++)
        {
            size_t l;
            const char *s = luaL_tolstring(L, i, &l);
            if (i > 1)
            {
                I_Printf("   ");
            }
            I_Printf("%s", s);
            lua_pop(L, 1);
        }
        I_Printf("\n");
        return 0;
    }

    static int DebugPrint(lua_State *L)
    {
        int n = lua_gettop(L);
        for (int i = 1; i <= n; i++)
        {
            size_t l;
            const char *s = luaL_tolstring(L, i, &l);
            if (i > 1)
            {
                I_Debugf("   ");
            }
            I_Debugf("%s", s);
            lua_pop(L, 1);
        }
        I_Debugf("\n");
        return 0;
    }

    void Open()
    {
        lua_State *state = vm_->GetState();

        luabridge::getGlobalNamespace(state)
            .beginNamespace("ec")
            .beginNamespace("sys")
            .addFunction("print", Print)
            .addFunction("debug_print", DebugPrint)
            .endNamespace()
            .endNamespace();
    }
};

void LUA_Sys_Init(lua_vm_c *vm)
{
    vm->AddModule<lua_sys_c>();
}