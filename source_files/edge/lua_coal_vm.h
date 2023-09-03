#pragma once

#include "lua_edge.h"

class lua_coal_vm_c : public elua::lua_vm_c
{
    friend class elua::lua_vm_c;

public:
    lua_coal_vm_c(elua::lua_vm_id id) : elua::lua_vm_c(id) {}

private:
    void Open() override;
};