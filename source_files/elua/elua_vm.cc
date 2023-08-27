
#include "elua.h"

namespace elua
{
    std::unordered_map<lua_vm_id, lua_vm_c *> lua_vm_c::vms_;

    lua_vm_c *lua_vm_c::Create(lua_vm_id id)
    {
        SYS_ASSERT(vms_.find(id) == vms_.end());

        // create and open the new vm
        lua_vm_c *vm = new lua_vm_c(id);
        vms_[id] = vm;
        vm->Open();

        return vm;
    }    

    void lua_vm_c::Open()
    {
        SYS_ASSERT(!state_);

        // we could specify a lua allocator, which would be a good idea to hook up to a debug allocator library for tracing
        // l = lua_newstate(lua_Alloc alloc, nullptr);

        state_ = luaL_newstate();
        luaL_openlibs(state_);

#if LUABRIDGE_HAS_EXCEPTIONS
        luabridge::enableExceptions(state_);
#endif
    }

    void lua_vm_c::Close()
    {
        if (state_)
        {
            lua_close(state_);
            state_ = nullptr;
        }
    }
}