#pragma once

namespace elua {

class lua_vm_c
{
public:
    lua_State* GetState()
    {
        SYS_ASSERT(state_);
        return state_;
    }

    luabridge::LuaResult Require(const std::string& path);
    void DoFile(const std::string& filename);
    void DoString(const std::string& source);

    template<class T>
    void AddModule()
    {
        static_assert(std::is_base_of<lua_module_c, T>::value,
                      "module must derive from lua_module_c");

        auto module = new T(this);
        module->Open();

        modules_[module->name_] = module;
    }

    template<typename T>
    static T* Create(lua_vm_id id)
    {
        SYS_ASSERT(vms_.find(id) == vms_.end());

        // create and open the new vm
        T* vm = new T(id);
        vms_[id] = vm;
        vm->Open();

        return vm;
    }

protected:
    lua_vm_c(lua_vm_id id) : refRequire_(nullptr) { id_ = id; }

    virtual void Open();
    void Close();

    lua_vm_id id_ = 0xFFFFFFFF;
    lua_State* state_ = nullptr;
    luabridge::LuaRef refRequire_;

    std::unordered_map<std::string, lua_module_c*> modules_;

    static std::unordered_map<lua_vm_id, lua_vm_c*> vms_;
};

} // namespace elua