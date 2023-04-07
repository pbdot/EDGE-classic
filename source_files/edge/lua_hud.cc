

#include "hu_draw.h"
#include "am_map.h"
#include "e_player.h"

#include "lua_vm.h"



extern player_t *ui_player_who;

static int ui_hud_automap_flags[2]; // 0 = disabled, 1 = enabled
static float ui_hud_automap_zoom;
static player_t *ui_hud_who = NULL;


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


void LUA_RunHud()
{
	HUD_Reset();

	ui_hud_who = players[displayplayer];
	ui_player_who = players[displayplayer];

	ui_hud_automap_flags[0] = 0;
	ui_hud_automap_flags[1] = 0;
	ui_hud_automap_zoom = -1;

	lua_vm_c* ui_vm = lua_vm_c::GetVM(LUA_VM_UI);
	ui_vm->GetState()["draw_all"]();

	HUD_Reset();
}


void LUA_Hud_Init(lua_vm_c* vm)
{
    vm->addModule<lua_hud_module_c>();
}