#include "i_defs.h"
#include "main.h"
#include "dm_state.h"
#include "version.h"
#include "hu_draw.h"
#include "am_map.h"
#include "e_player.h"
#include "lua_vm.h"

#include <sol.hpp>

extern cvar_c r_doubleframes;
extern int gametic;
extern player_t *ui_player_who;

// fix me
static int ui_hud_automap_flags[2]; // 0 = disabled, 1 = enabled
static float ui_hud_automap_zoom;
static player_t *ui_hud_who = NULL;

typedef enum { LUA_VM_UI = 0, LUA_VM_PLAY = 1, LUA_VM_DATA = 2 } lua_vm_type_e;

// Lua print replacement
static int LUA_Print(lua_State *L);
static int LUA_Sys_Print(lua_State *L, bool debug = false);
static void LUA_CheckError(const sol::protected_function_result &result,
						   bool fatal = false);

class lua_vm_c;

class lua_module_c {

  public:
	const std::string &GetName() { return name_; }

	virtual void Open() = 0;

  protected:
	lua_module_c(std::string name, lua_vm_c *vm);

	std::string name_;
	lua_vm_c *vm_ = nullptr;
};

class lua_vm_c {
  public:
	lua_vm_c() { Open(); }

	template <class ModuleClass> void addModule()
	{
		auto module = new ModuleClass(this);
		module->Open();
		modules_[module->GetName()] = module;
	}

	sol::state &GetState() { return state_; }

  private:
	void Open()
	{
		state_.open_libraries(sol::lib::base, sol::lib::table,
							  sol::lib::package, sol::lib::os, sol::lib::io,
							  sol::lib::math, sol::lib::string,
							  sol::lib::coroutine, sol::lib::debug);

		// main edge module
		state_["edge"] = state_.create_table();
		// override the default lua print
		state_["print"] = LUA_Print;
	}

	std::unordered_map<std::string, lua_module_c *> modules_;

	sol::state state_;
};

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

class lua_hud_module_c : public lua_module_c {
  public:
	lua_hud_module_c(lua_vm_c *vm) : lua_module_c("hud", vm) {}

	void Open()
	{
		sol::state &state = vm_->GetState();
		sol::usertype<lua_hud_module_c> hud =
			state.new_usertype<lua_hud_module_c>("hud");

		hud.set("x_left", sol::readonly(&lua_hud_module_c::x_left_));
		hud.set("x_right", sol::readonly(&lua_hud_module_c::x_right_));

		hud.set("coord_sys", [this](int w, int h) {
			if (w < 64 || h < 64)
				I_Error("Bad hud.coord_sys size: %dx%d\n", w, h);
			HUD_SetCoordSys(w, h);
			x_left_ = hud_x_left;
			x_right_ = hud_x_right;
		});

		hud.set("set_scale", [](float scale) {
			if (scale <= 0)
				I_Error("hud.set_scale: Bad scale value: %1.3f\n", scale);

			HUD_SetScale(scale);
		});

		hud.set("check_automap", []() { return automapactive; });

		hud.set_function("render_automap", [](float x, float y, float w,
											  float h, sol::variadic_args va) {
			int flags = 0;

			if (va.size() > 0) {
				flags = va[0];
			}

			int old_state;
			float old_zoom;

			AM_GetState(&old_state, &old_zoom);

			int new_state = old_state;
			new_state &= ~ui_hud_automap_flags[0];
			new_state |= ui_hud_automap_flags[1];

			float new_zoom = old_zoom;
			if (ui_hud_automap_zoom > 0.1)
				new_zoom = ui_hud_automap_zoom;

			AM_SetState(new_state, new_zoom);

			HUD_RenderAutomap(x, y, w, h, ui_hud_who->mo, flags);

			AM_SetState(old_state, old_zoom);
		});

		hud.set_function("render_world", [](float x, float y, float w, float h,
											sol::variadic_args va) {
			int flags = 0;

			if (va.size() > 0) {
				flags = va[0];
			}

			HUD_RenderWorld(x, y, w, h, ui_hud_who->mo, flags);
		});

		// set module
		state["edge"][name_] = this;
	}

  private:
	float x_left_ = 0;
	float x_right_ = 0;
};

static lua_vm_c *vm_hud = nullptr;

void LUA_RunHud()
{
	HUD_Reset();

	ui_hud_who = players[displayplayer];
	ui_player_who = players[displayplayer];

	ui_hud_automap_flags[0] = 0;
	ui_hud_automap_flags[1] = 0;
	ui_hud_automap_zoom = -1;

	vm_hud->GetState()["draw_all"]();

	HUD_Reset();
}

lua_vm_c *LUA_VM_New(lua_vm_type_e vm_type)
{
	lua_vm_c *vm = new lua_vm_c();

	vm->addModule<lua_sys_module_c>();

	switch (vm_type) {
	case LUA_VM_UI:
		vm->addModule<lua_hud_module_c>();
		break;
	case LUA_VM_PLAY:
		break;
	case LUA_VM_DATA:
		break;
	}

	LUA_CheckError(vm->GetState().do_file("test.lua"));

	return vm;
}

void LUA_Init() { vm_hud = LUA_VM_New(LUA_VM_UI); }

lua_module_c::lua_module_c(std::string name, lua_vm_c *vm)
{
	name_ = name;
	vm_ = vm;
}

// Lua print replacement
int LUA_Print(lua_State *L)
{
	int n = lua_gettop(L);
	int i;
	for (i = 1; i <= n; i++) {
		size_t l;
		const char *s = luaL_tolstring(L, i, &l);
		if (i > 1)
			I_Printf("   ");
		I_Printf(s);
		lua_pop(L, 1);
	}
	I_Printf("\n");
	return 0;
}

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