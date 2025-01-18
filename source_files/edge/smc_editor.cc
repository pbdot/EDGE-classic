
#include "smc.h"
#include "w_files.h"

void SMC_Editor_Init()
{
    epi::LuaConfig lua_config;
    lua_config.name_                = epi::EName("smc_editor");
    lua_config.debug_enabled_       = true;
    lua_config.check_file_callback_ = CheckPackFilesForName;
    lua_config.open_file_callback_  = OpenFileFromPack;
    smc::SMC_Init(lua_config);
}