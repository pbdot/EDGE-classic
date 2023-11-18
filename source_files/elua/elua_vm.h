

#pragma once

namespace elua
{

class lua_module_c;

class lua_vm_c final
{
  public:

    lua_State* GetState()
    {
        SYS_ASSERT(state_);
        return state_;
    }

    void DoString(const char* source);

    void Close();

    template <class T> void AddModule()
    {
        static_assert(std::is_base_of<lua_module_c, T>::value, "module must derive from lua_module_c");

        auto module = new T(this);
        module->Open();

        modules_[module->GetName()] = module;
    }

    static lua_vm_c *Create(const std::string &name, lua_CFunction searcher)
    {
        SYS_ASSERT(vms_.find(name) == vms_.end());

        // create and open the new vm
        lua_vm_c *vm = new lua_vm_c(name);
        vms_[name]   = vm;
        vm->Open(searcher);

        return vm;
    }

  private:
    lua_vm_c(const std::string &name) : name_(name)
    {
    }

    void Open(lua_CFunction searcher);

    std::string name_;
    lua_State  *state_ = nullptr;

    std::unordered_map<std::string, lua_module_c *> modules_;

    static std::unordered_map<std::string, lua_vm_c *> vms_;
};

} // namespace elua