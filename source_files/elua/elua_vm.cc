
#include "elua.h"

namespace elua
{
    std::unordered_map<lua_vm_id, lua_vm_c *> lua_vm_c::vms_;

    void lua_vm_c::DoFile(const std::string &filename)
    {
        luaL_dofile(state_, filename.c_str());
    }

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

        int result = luaL_dostring(state_, "package.path = 'C:/Dev/EDGE-classic-pbdot/script/?.lua'");
        lua_pop(state_, result);
        // luabridge::LuaRef r = luabridge::get<luabridge::LuaRef>(state_, -1).value();
        // std::string poop = r.tostring();

#if LUABRIDGE_HAS_EXCEPTIONS
        luabridge::enableExceptions(state_);
#endif

        refRequire_ = luabridge::getGlobal(state_, "require");
        SYS_ASSERT(refRequire_.isFunction());
    }

    luabridge::LuaResult lua_vm_c::Require(const std::string &path)
    {
        return refRequire_(path.c_str());
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