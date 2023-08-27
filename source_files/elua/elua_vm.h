#pragma once

namespace elua
{

class lua_vm_c
    {
    public:
        static lua_vm_c *Create(lua_vm_id id);

        template <class T>
        void AddModule()
        {
            static_assert(std::is_base_of<lua_module_c, T>::value, "module must derive from lua_module_c");

            auto module = new T(this);
            module->Open();

            modules_[module->name_] = module;
        }

    private:
        lua_vm_c(lua_vm_id id)
        {
            id_ = id;
        }

        void Open();
        void Close();

        lua_vm_id id_ = 0xFFFFFFFF;
        lua_State *state_ = nullptr;

        std::unordered_map<std::string, lua_module_c *> modules_;

        static std::unordered_map<lua_vm_id, lua_vm_c *> vms_;
    };


}