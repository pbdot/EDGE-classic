
#pragma once

#include "epi.h"
#include "epi_ename.h"
#include "epi_file.h"

namespace epi
{

typedef epi::File *(*LuaOpenFileCallback)(const std::string &name);
typedef int (*LuaCheckFileCallback)(const std::string &name);

struct LuaConfig
{
    EName                name_;
    bool                 debug_enabled_       = false;
    LuaOpenFileCallback  open_file_callback_  = nullptr;
    LuaCheckFileCallback check_file_callback_ = nullptr;
};

class ILuaState
{
  public:
    virtual bool DoFile(const char *filename) = 0;

    static ILuaState *CreateVM(const LuaConfig &config);
};

} // namespace epi
