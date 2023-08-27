#pragma once

#include <type_traits>
#include <unordered_map>
#include "lua/lua.hpp"
#include "luabridge/LuaBridge.h"
#include "epi.h"

namespace elua
{
    class lua_vm_c;

    class lua_module_c
    {
        friend class lua_vm_c;

    protected:
        lua_module_c(lua_vm_c *vm, const std::string& name);

        virtual void Open() = 0;

        lua_vm_c* vm_ = nullptr;
        std::string name_;
    };

}