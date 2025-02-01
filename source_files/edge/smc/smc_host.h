
#pragma once

#include <SDL2/SDL.h>

namespace edge
{
void SMC_Host_Initialize();
bool SMC_Host_Initialized();

bool SMC_Host_HandleEvent(const SDL_Event* event);

bool SMC_Host_Activated();
void SMC_Host_Activate(bool activated);
void SMC_Host_Deactivate();

void SMC_Host_StartFrame();
void SMC_Host_FinishFrame();

void SMC_Host_Shutdown();
}
