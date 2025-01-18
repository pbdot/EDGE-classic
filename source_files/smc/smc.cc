
#include "smc.h"

#include "nuklear_defines.h"
#include "sokol_gfx.h"
#include "sokol_nuklear.h"

extern "C"
{
    // defined in moon nuklear static lib
    int luaopen_moonnuklear(lua_State *L);
}

namespace smc
{

static epi::ILuaState *lua_state = nullptr;
static bool moon_context_set = false;

void SMC_Frame()
{
    // can happen when drawing intial stuff at boot
    if (!lua_state)
    {
        return;
    }

    nk_context *ctx = snk_new_frame();

    // todo: refactor this
    if (!moon_context_set)
    {
        moon_context_set = true;

        lua_State *L = lua_state->GetLuaState();        
        lua_getglobal(L, "SMC_SetContext");
        lua_pushlightuserdata(L, (void*) ctx);
        lua_call(L, 1, 0);        
    }    

    lua_state->CallGlobalFunction("SMC_Frame");


    snk_render(1920, 1080);
}

void SMC_Init(epi::LuaConfig &lua_config)
{
    EPI_ASSERT(!lua_state);

    snk_desc_t snk_desc = {0};
    snk_setup(&snk_desc);

    lua_config.modules_.push_back({luaopen_moonnuklear, "nk"});

    lua_state = epi::ILuaState::CreateVM(lua_config);    

    lua_state->DoFile("scripts/lua/smc/test.lua");
}

} // namespace smc
