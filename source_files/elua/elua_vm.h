

#pragma once

namespace elua
{

class lua_vm_c final
{
    void Open();

    void Close();

    private:
        lua_State* state_ = nullptr;
};

} // namespace elua