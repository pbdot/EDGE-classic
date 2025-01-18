
#pragma once

#include "epi_lua.h"

namespace smc
{

void SMC_Init(epi::LuaConfig &lua_config);
void SMC_Frame();

void SMC_InputBegin();
int SMC_InputEvent(void *event);
void SMC_InputEnd();

} // namespace smc