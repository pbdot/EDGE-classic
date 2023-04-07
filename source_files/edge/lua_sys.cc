
#include "i_defs.h"
#include "main.h"
#include "version.h"
#include "lua_vm.h"

extern cvar_c r_doubleframes;
extern int gametic;

static int LUA_Sys_Print(lua_State *L, bool debug = false);

class lua_sys_module_c : public lua_module_c {
  public:
	lua_sys_module_c(lua_vm_c *vm) : lua_module_c("sys", vm) {}

	void Open()
	{
		sol::state &state = vm_->GetState();
		sol::usertype<lua_sys_module_c> sys =
			state.new_usertype<lua_sys_module_c>("sys");

		// constants
		sys.set("TICRATE", sol::readonly(&lua_sys_module_c::_TICRATE));
		sys.set("EDGEVER", sol::readonly(&lua_sys_module_c::_EDGEVER));

		// properties
		sys.set("gametic", sol::property(&lua_sys_module_c::GetGameTic));

		// output
		sys.set("error", [](const char *error) { I_Error(error); });

		sys.set("print", [this]() {
			LUA_Sys_Print(this->vm_->GetState().lua_state(), false);
		});

		sys.set("debug_print", [this]() {
			LUA_Sys_Print(this->vm_->GetState().lua_state(), true);
		});

		// set module
		state["edge"][name_] = this;
	}

  private:
	const int _TICRATE = TICRATE;
	const uint32_t _EDGEVER = EDGEVERHEX;

	int GetGameTic() { return gametic / (r_doubleframes.d ? 2 : 1); }
};


int LUA_Sys_Print(lua_State *L, bool debug)
{
	int n = lua_gettop(L);
	int i;
	for (i = 1; i <= n; i++) {
		size_t l;
		const char *s = luaL_tolstring(L, i, &l);
		if (i > 1)
			debug ? I_Debugf("   ") : I_Printf("   ");
		debug ? I_Debugf(s) : I_Printf(s);
		lua_pop(L, 1);
	}
	debug ? I_Debugf("\n") : I_Printf("\n");
	return 0;
}

void LUA_CheckError(const sol::protected_function_result &result, bool fatal)
{
	if (result.valid()) {
		return;
	}

	sol::error error = result;
	if (fatal) {
		I_Error("Lua: Error %s\n", error.what());
	}
	else {
		I_Printf("Lua: Error %s\n", error.what());
	}
}

void LUA_Sys_Init(lua_vm_c* vm)
{
    vm->addModule<lua_sys_module_c>();
}