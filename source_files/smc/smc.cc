
#include "smc.h"

namespace smc
{

void SMC_Init(const epi::LuaConfig &lua_config)
{
    epi::ILuaState *state = epi::ILuaState::CreateVM(lua_config);
    state->DoFile("scripts/lua/smc/test.lua");
}

} // namespace smc
