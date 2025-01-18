
#pragma once

#include <lua.hpp>
#include <vector>

#include "epi.h"
#include "epi_ename.h"
#include "epi_file.h"

namespace epi
{

typedef epi::File *(*LuaOpenFileCallback)(const std::string &name);
typedef int (*LuaCheckFileCallback)(const std::string &name);

struct LuaModule
{
    lua_CFunction lua_open_func_;
    std::string   name_;
};

struct LuaConfig
{
    EName                  name_;
    bool                   debug_enabled_       = false;
    LuaOpenFileCallback    open_file_callback_  = nullptr;
    LuaCheckFileCallback   check_file_callback_ = nullptr;
    std::vector<LuaModule> modules_;
};

class ILuaState
{
  public:

    // Use sparingly
    virtual lua_State* GetLuaState() = 0;

    virtual bool DoFile(const char *filename) = 0;

    virtual void CallGlobalFunction(const char *function_name) = 0;

    static ILuaState *CreateVM(const LuaConfig &config);
};

} // namespace epi
