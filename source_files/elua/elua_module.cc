
#include "elua.h"

namespace elua
{
    lua_module_c::lua_module_c(lua_vm_c *vm, const std::string& name)
    {
        vm_ = vm;
        name_ = name;
    }
}