#pragma once

namespace elua
{
class lua_vm_c;

class lua_module_c
{
    friend class lua_vm_c;

  public:
    virtual const std::string &GetName() = 0;

  protected:
    lua_module_c(lua_vm_c *vm);

    virtual void Open() = 0;

    lua_vm_c *vm_ = nullptr;
};

} // namespace elua