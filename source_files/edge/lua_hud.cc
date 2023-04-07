#include "i_defs.h"
#include "hu_draw.h"
#include "am_map.h"
#include "e_player.h"
#include "dm_state.h"
#include "g_game.h"
#include "p_tick.h"
#include "image_data.h"
#include "r_colormap.h"
#include "r_misc.h"
#include "s_sound.h"
#include "font.h"
#include "w_wad.h"
#include "rad_trig.h"

#include "lua_vm.h"

typedef struct {
	double r;
	double g;
	double b;
} hud_rgb_t;

extern std::string w_map_title;
extern cvar_c r_doubleframes;

extern player_t *ui_player_who;

static int ui_hud_automap_flags[2]; // 0 = disabled, 1 = enabled
static float ui_hud_automap_zoom;
static player_t *ui_hud_who = nullptr;

extern epi::image_data_c *ReadAsEpiBlock(image_c *rim);

extern epi::image_data_c *R_PalettisedToRGB(epi::image_data_c *src,
											const byte *palette, int opacity);
//------------------------------------------------------------------------

static rgbcol_t VM_VectorToColor(const sol::table &v)
{
	if ((int)v[1] < 0)
		return RGB_NO_VALUE;

	int r = CLAMP(0, (int)v[1], 255);
	int g = CLAMP(0, (int)v[2], 255);
	int b = CLAMP(0, (int)v[3], 255);

	rgbcol_t rgb = RGB_MAKE(r, g, b);

	// ensure we don't get the "no color" value by mistake
	if (rgb == RGB_NO_VALUE)
		rgb ^= 0x000101;

	return rgb;
}

//------------------------------------------------------------------------
//  HUD MODULE
//------------------------------------------------------------------------

// hud.coord_sys(w, h)
//
static void HD_coord_sys(int w, int h, float &x_left, float &x_right)
{

	if (w < 64 || h < 64)
		I_Error("Bad hud.coord_sys size: %dx%d\n", w, h);

	HUD_SetCoordSys(w, h);

	x_left = hud_x_left;
	x_right = hud_x_right;
}

// hud.game_mode()
//
static const char *HD_game_mode()
{
	if (DEATHMATCH())
		return "dm";
	else if (COOP_MATCH())
		return "coop";
	else
		return "sp";
}

// hud.game_name()
//
static const char *HD_game_name()
{
	gamedef_c *g = currmap->episode;
	SYS_ASSERT(g);

	return g->name.c_str();
}

// hud.map_name()
//
static const char *HD_map_name() { return currmap->name.c_str(); }

// hud.map_title()
//
static const char *HD_map_title() { return w_map_title.c_str(); }

// hud.which_hud()
//
static int HD_which_hud() { return screen_hud; }

// hud.check_automap()
//
static bool HD_check_automap() { return automapactive; }

// hud.get_time()
//
static float HD_get_time()
{
	int time = I_GetTime() / (r_doubleframes.d ? 2 : 1);
	return (float)time;
}

// hud.text_font(name)
//
static void HD_text_font(const char *font_name)
{
	fontdef_c *DEF = fontdefs.Lookup(font_name);
	SYS_ASSERT(DEF);

	if (!DEF)
		I_Error("hud.text_font: Bad font name: %s\n", font_name);

	font_c *font = hu_fonts.Lookup(DEF);
	SYS_ASSERT(font);

	if (!font)
		I_Error("hud.text_font: Bad font name: %s\n", font_name);

	HUD_SetFont(font);
}

// hud.text_color(rgb)
//
static void HD_text_color(const sol::table &v)
{
	rgbcol_t color = VM_VectorToColor(v);

	HUD_SetTextColor(color);
}

// hud.set_scale(value)
//
static void HD_set_scale(float scale)
{
	if (scale <= 0)
		I_Error("hud.set_scale: Bad scale value: %1.3f\n", scale);

	HUD_SetScale(scale);
}

// hud.set_alpha(value)
//
static void HD_set_alpha(float alpha) { HUD_SetAlpha(alpha); }

// hud.solid_box(x, y, w, h, color)
//
static void HD_solid_box(float x, float y, float w, float h,
						 const sol::table &color)
{
	rgbcol_t rgb = VM_VectorToColor(color);

	HUD_SolidBox(x, y, x + w, y + h, rgb);
}

// hud.solid_line(x1, y1, x2, y2, color)
//
static void HD_solid_line(float x1, float y1, float x2, float y2,
						  const sol::table &color)
{
	rgbcol_t rgb = VM_VectorToColor(color);

	HUD_SolidLine(x1, y1, x2, y2, rgb);
}

// hud.thin_box(x, y, w, h, color)
//
static void HD_thin_box(float x, float y, float w, float h,
						const sol::table &color)
{
	rgbcol_t rgb = VM_VectorToColor(color);

	HUD_ThinBox(x, y, x + w, y + h, rgb);
}

// hud.gradient_box(x, y, w, h, TL, BL, TR, BR)
//
static void HD_gradient_box(float x, float y, float w, float h, uint32_t tl,
							uint32_t bl, uint32_t tr, uint32_t br)
{
	rgbcol_t cols[4];

	cols[0] = tl;
	cols[1] = bl;
	cols[2] = tr;
	cols[3] = br;

	HUD_GradientBox(x, y, x + w, y + h, cols);
}

// hud.draw_image(x, y, name, [noOffset])
// if we specify noOffset then it ignores
// X and Y offsets from doom or images.ddf
//
static void HD_draw_image(float x, float y, const char *name,
						  const sol::variadic_args &va)
{
	const image_c *img = W_ImageLookup(name, INS_Graphic);

	bool noOffset = false;
	if (va.size() > 0) {
		noOffset = (bool)va[0];
	}

	if (img) {
		if (noOffset)
			HUD_DrawImageNoOffset(x, y, img);
		else
			HUD_DrawImage(x, y, img);
	}
}

// Dasho 2022: Same as above but adds x/y texcoord scrolling
// hud.scroll_image(x, y, name, sx, sy, [noOffset])
//
static void HD_scroll_image(float x, float y, const char *name, float sx,
							float sy, const sol::variadic_args &va)
{

	bool noOffset = false;
	if (va.size() > 0) {
		noOffset = (bool)va[0];
	}

	const image_c *img = W_ImageLookup(name, INS_Graphic);

	if (img) {
		if (noOffset)
			HUD_ScrollImageNoOffset(
				x, y, img, -sx,
				-sy); // Invert sx/sy so that user can enter positive X for
					  // right and positive Y for up
		else
			HUD_ScrollImage(x, y, img, -sx,
							-sy); // Invert sx/sy so that user can enter
								  // positive X for right and positive Y for up
	}
}

// hud.stretch_image(x, y, w, h, name, [noOffset])
// if we specify noOffset then it ignores
// X and Y offsets from doom or images.ddf
//
static void HD_stretch_image(float x, float y, float w, float h,
							 const char *name, const sol::variadic_args &va)
{

	bool noOffset = false;
	if (va.size() > 0) {
		noOffset = (bool)va[0];
	}

	const image_c *img = W_ImageLookup(name, INS_Graphic);

	if (img) {
		if (noOffset)
			HUD_StretchImageNoOffset(x, y, w, h, img, 0.0, 0.0);
		else
			HUD_StretchImage(x, y, w, h, img, 0.0, 0.0);
	}
}

// hud.tile_image(x, y, w, h, name, offset_x, offset_y)
//
static void HD_tile_image(float x, float y, float w, float h, const char *name,
						  float offset_x, float offset_y)
{
	const image_c *img = W_ImageLookup(name, INS_Texture);

	if (img) {
		HUD_TileImage(x, y, w, h, img, offset_x, offset_y);
	}
}

// hud.draw_text(x, y, str, [size])
//
static void HD_draw_text(float x, float y, const char *str,
						 const sol::variadic_args &va)
{
	float size = 0;
	if (va.size() > 0) {
		size = (float)va[0];
	}
	HUD_DrawText(x, y, str, size);
}

// hud.draw_num2(x, y, len, num, [size])
//
static void HD_draw_num2(float x, float y, int len, int num,
						 const sol::variadic_args &va)
{
	float size = 0;
	if (va.size() > 0) {
		size = (float)va[0];
	}

	if (len < 1 || len > 20)
		I_Error("hud.draw_number: bad field length: %d\n", len);

	bool is_neg = false;

	if (num < 0 && len > 1) {
		is_neg = true;
		len--;
	}

	// build the integer backwards

	char buffer[200];
	char *pos = &buffer[sizeof(buffer) - 4];

	*--pos = 0;

	if (num == 0) {
		*--pos = '0';
	}
	else {
		for (; num > 0 && len > 0; num /= 10, len--)
			*--pos = '0' + (num % 10);

		if (is_neg)
			*--pos = '-';
	}

	HUD_SetAlignment(+1, -1);
	HUD_DrawText(x, y, pos, size);
	HUD_SetAlignment();
}

// Lobo November 2021:
// hud.draw_number(x, y, len, num, align_right, [size])
//
static void HD_draw_number(float x, float y, int len, int num, int align_right,
						   const sol::variadic_args &va)
{
	float size = 0;
	if (va.size() > 0) {
		size = (float)va[0];
	}

	if (len < 1 || len > 20)
		I_Error("hud.draw_number: bad field length: %d\n", len);

	bool is_neg = false;

	if (num < 0 && len > 1) {
		is_neg = true;
		len--;
	}

	// build the integer backwards

	char buffer[200];
	char *pos = &buffer[sizeof(buffer) - 4];

	*--pos = 0;

	if (num == 0) {
		*--pos = '0';
	}
	else {
		for (; num > 0 && len > 0; num /= 10, len--)
			*--pos = '0' + (num % 10);

		if (is_neg)
			*--pos = '-';
	}

	if (align_right == 0) {
		HUD_DrawText(x, y, pos, size);
	}
	else {
		HUD_SetAlignment(+1, -1);
		HUD_DrawText(x, y, pos, size);
		HUD_SetAlignment();
	}
}

// hud.game_paused()
//
static int HD_game_paused()
{
	if (paused || menuactive || rts_menuactive || time_stop_active ||
		erraticism_active) {
		return 1;
	}
	else {
		return 0;
	}
}

// hud.erraticism_active()
//
static int HD_erraticism_active()
{
	if (erraticism_active) {
		return 1;
	}
	else {
		return 0;
	}
}

// hud.time_stop_active()
//
static int HD_time_stop_active()
{
	if (time_stop_active) {
		return 1;
	}
	else {
		return 0;
	}
}

// hud.render_world(x, y, w, h, [flags])
//
static void HD_render_world(float x, float y, float w, float h,
							const sol::variadic_args &va)
{
	int flags = 0;
	if (va.size() > 0) {
		flags = (int)va[0];
	}
	HUD_RenderWorld(x, y, w, h, ui_hud_who->mo, flags);
}

// hud.render_automap(x, y, w, h, [flags])
//
static void HD_render_automap(float x, float y, float w, float h,
							  const sol::variadic_args &va)
{
	int flags = 0;
	if (va.size() > 0) {
		flags = (int)va[0];
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
}

// hud.automap_color(which, color)
//
static void HD_automap_color(int which, const sol::table &color)
{
	if (which < 1 || which > AM_NUM_COLORS)
		I_Error("hud.automap_color: bad color number: %d\n", which);

	which--;

	rgbcol_t rgb = VM_VectorToColor(color);

	AM_SetColor(which, rgb);
}

// hud.automap_option(which, value)
//
static void HD_automap_option(int which, int value)
{
	if (which < 1 || which > 7)
		I_Error("hud.automap_color: bad color number: %d\n", which);

	which--;

	if (value <= 0)
		ui_hud_automap_flags[0] |= (1 << which);
	else
		ui_hud_automap_flags[1] |= (1 << which);
}

// hud.automap_zoom(value)
//
static void HD_automap_zoom(float zoom)
{
	// impose a very broad limit
	ui_hud_automap_zoom = CLAMP(0.2f, zoom, 100.0f);
}

// hud.set_render_who(index)
//
static void HD_set_render_who(int index)
{
	if (index < 0 || index >= numplayers)
		I_Error("hud.set_render_who: bad index value: %d (numplayers=%d)\n",
				index, numplayers);

	if (index == 0) {
		ui_hud_who = players[consoleplayer];
		return;
	}

	int who = displayplayer;

	for (; index > 1; index--) {
		do {
			who = (who + 1) % MAXPLAYERS;
		} while (players[who] == NULL);
	}

	ui_hud_who = players[who];
}

// hud.play_sound(name)
//
static void HD_play_sound(const char *name)
{
	sfx_t *fx = sfxdefs.GetEffect(name);

	if (fx)
		S_StartFX(fx);
	else
		I_Warning("hud.play_sound: unknown sfx '%s'\n", name);
}

// hud.screen_aspect()
//
static float HD_screen_aspect()
{
	return (float)std::ceil(v_pixelaspect.f * 100.0) / 100.0;
}

static hud_rgb_t HD_get_average_color(const char *name,
									  const sol::variadic_args &va)
{

	double from_x = -1;
	double to_x = 1000000;
	double from_y = -1;
	double to_y = 1000000;

	if (va.size() > 0) {
		from_x = (double)va[0];
	}

	if (va.size() > 1) {
		to_x = (double)va[1];
	}

	if (va.size() > 2) {
		from_y = (double)va[2];
	}

	if (va.size() > 3) {
		to_y = (double)va[3];
	}

	hud_rgb_t rgb;
	const byte *what_palette = (const byte *)&playpal_data[0];
	const image_c *tmp_img_c = W_ImageLookup(name, INS_Graphic, 0);
	if (tmp_img_c->source_palette >= 0)
		what_palette = (const byte *)W_CacheLumpNum(tmp_img_c->source_palette);
	epi::image_data_c *tmp_img_data = R_PalettisedToRGB(
		ReadAsEpiBlock((image_c *)tmp_img_c), what_palette, tmp_img_c->opacity);
	u8_t *temp_rgb = new u8_t[3];
	tmp_img_data->AverageColor(temp_rgb, from_x, to_x, from_y, to_y);
	rgb.r = temp_rgb[0];
	rgb.g = temp_rgb[1];
	rgb.b = temp_rgb[2];
	delete tmp_img_data;
	delete[] temp_rgb;
	return rgb;
}

static hud_rgb_t HD_get_lightest_color(const char *name,
									   const sol::variadic_args &va)
{

	hud_rgb_t rgb;
	double from_x = -1;
	double to_x = 1000000;
	double from_y = -1;
	double to_y = 1000000;

	if (va.size() > 0) {
		from_x = (double)va[0];
	}

	if (va.size() > 1) {
		to_x = (double)va[1];
	}

	if (va.size() > 2) {
		from_y = (double)va[2];
	}

	if (va.size() > 3) {
		to_y = (double)va[3];
	}
	const byte *what_palette = (const byte *)&playpal_data[0];
	const image_c *tmp_img_c = W_ImageLookup(name, INS_Graphic, 0);
	if (tmp_img_c->source_palette >= 0)
		what_palette = (const byte *)W_CacheLumpNum(tmp_img_c->source_palette);
	epi::image_data_c *tmp_img_data = R_PalettisedToRGB(
		ReadAsEpiBlock((image_c *)tmp_img_c), what_palette, tmp_img_c->opacity);
	u8_t *temp_rgb = new u8_t[3];
	tmp_img_data->LightestColor(temp_rgb, from_x, to_x, from_y, to_y);
	rgb.r = temp_rgb[0];
	rgb.g = temp_rgb[1];
	rgb.b = temp_rgb[2];
	delete tmp_img_data;
	delete[] temp_rgb;
	return rgb;
}

static hud_rgb_t HD_get_darkest_color(const char *name,
									  const sol::variadic_args &va)
{

	hud_rgb_t rgb;
	double from_x = -1;
	double to_x = 1000000;
	double from_y = -1;
	double to_y = 1000000;

	if (va.size() > 0) {
		from_x = (double)va[0];
	}

	if (va.size() > 1) {
		to_x = (double)va[1];
	}

	if (va.size() > 2) {
		from_y = (double)va[2];
	}

	if (va.size() > 3) {
		to_y = (double)va[3];
	}
	const byte *what_palette = (const byte *)&playpal_data[0];
	const image_c *tmp_img_c = W_ImageLookup(name, INS_Graphic, 0);
	if (tmp_img_c->source_palette >= 0)
		what_palette = (const byte *)W_CacheLumpNum(tmp_img_c->source_palette);
	epi::image_data_c *tmp_img_data = R_PalettisedToRGB(
		ReadAsEpiBlock((image_c *)tmp_img_c), what_palette, tmp_img_c->opacity);
	u8_t *temp_rgb = new u8_t[3];
	tmp_img_data->DarkestColor(temp_rgb, from_x, to_x, from_y, to_y);
	rgb.r = temp_rgb[0];
	rgb.g = temp_rgb[1];
	rgb.b = temp_rgb[2];
	delete tmp_img_data;
	delete[] temp_rgb;
	return rgb;
}

static hud_rgb_t HD_get_average_hue(const char *name,
									const sol::variadic_args &va)
{

	hud_rgb_t rgb;
	double from_x = -1;
	double to_x = 1000000;
	double from_y = -1;
	double to_y = 1000000;

	if (va.size() > 0) {
		from_x = (double)va[0];
	}

	if (va.size() > 1) {
		to_x = (double)va[1];
	}

	if (va.size() > 2) {
		from_y = (double)va[2];
	}

	if (va.size() > 3) {
		to_y = (double)va[3];
	}
	const byte *what_palette = (const byte *)&playpal_data[0];
	const image_c *tmp_img_c = W_ImageLookup(name, INS_Graphic, 0);
	if (tmp_img_c->source_palette >= 0)
		what_palette = (const byte *)W_CacheLumpNum(tmp_img_c->source_palette);
	epi::image_data_c *tmp_img_data = R_PalettisedToRGB(
		ReadAsEpiBlock((image_c *)tmp_img_c), what_palette, tmp_img_c->opacity);
	u8_t *temp_rgb = new u8_t[3];
	tmp_img_data->AverageHue(temp_rgb, NULL, from_x, to_x, from_y, to_y);
	rgb.r = temp_rgb[0];
	rgb.g = temp_rgb[1];
	rgb.b = temp_rgb[2];
	delete tmp_img_data;
	delete[] temp_rgb;
	return rgb;
}

// These two aren't really needed anymore with the AverageColor rework, but
// keeping them in case COALHUDS in the wild use them - Dasho
static hud_rgb_t HD_get_average_top_border_color(const char *name)
{

	hud_rgb_t rgb;
	const byte *what_palette = (const byte *)&playpal_data[0];
	const image_c *tmp_img_c = W_ImageLookup(name, INS_Graphic, 0);
	if (tmp_img_c->source_palette >= 0)
		what_palette = (const byte *)W_CacheLumpNum(tmp_img_c->source_palette);
	epi::image_data_c *tmp_img_data = R_PalettisedToRGB(
		ReadAsEpiBlock((image_c *)tmp_img_c), what_palette, tmp_img_c->opacity);
	u8_t *temp_rgb = new u8_t[3];
	tmp_img_data->AverageColor(temp_rgb, 0, tmp_img_c->actual_w,
							   tmp_img_c->actual_h - 1, tmp_img_c->actual_h);
	rgb.r = temp_rgb[0];
	rgb.g = temp_rgb[1];
	rgb.b = temp_rgb[2];
	delete tmp_img_data;
	delete[] temp_rgb;
	return rgb;
}
static hud_rgb_t HD_get_average_bottom_border_color(const char *name)
{
	hud_rgb_t rgb;
	const byte *what_palette = (const byte *)&playpal_data[0];
	const image_c *tmp_img_c = W_ImageLookup(name, INS_Graphic, 0);
	if (tmp_img_c->source_palette >= 0)
		what_palette = (const byte *)W_CacheLumpNum(tmp_img_c->source_palette);
	epi::image_data_c *tmp_img_data = R_PalettisedToRGB(
		ReadAsEpiBlock((image_c *)tmp_img_c), what_palette, tmp_img_c->opacity);
	u8_t *temp_rgb = new u8_t[3];
	tmp_img_data->AverageColor(temp_rgb, 0, tmp_img_c->actual_w, 0, 1);
	rgb.r = temp_rgb[0];
	rgb.g = temp_rgb[1];
	rgb.b = temp_rgb[2];
	delete tmp_img_data;
	delete[] temp_rgb;
	return rgb;
}

// hud.rts_enable(tag)
//
static void HD_rts_enable(const std::string &name)
{

	if (!name.empty())
		RAD_EnableByTag(NULL, name.c_str(), false);
}

// hud.rts_isactive(tag)
//
static bool HD_rts_isactive(const std::string &name)
{
	if (!name.empty()) {
		if (RAD_IsActiveByTag(NULL, name.c_str()))
			return true;
		else
			return false;
	}
}

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
			HD_coord_sys(w, h, lua_hud_module_c::x_left_,
						 lua_hud_module_c::x_right_);
		});

		hud.set("game_mode", HD_game_mode);
		hud.set("game_name", HD_game_name);
		hud.set("map_name", HD_map_name);
		hud.set("map_title", HD_map_title);

		hud.set("which_hud", HD_which_hud);
		hud.set("check_automap", HD_check_automap);
		hud.set("get_time", HD_get_time);

		hud.set("text_font", HD_text_font);
		hud.set("text_color", HD_text_color);

		hud.set("set_scale", HD_set_scale);
		hud.set("set_alpha", HD_set_alpha);

		hud.set("solid_box", HD_solid_box);
		hud.set("solid_line", HD_solid_line);
		hud.set("thin_box", HD_thin_box);
		hud.set("gradient_box", HD_gradient_box);

		hud.set("draw_image", HD_draw_image);
		hud.set("scroll_image", HD_scroll_image);
		hud.set("stretch_image", HD_stretch_image);
		hud.set("tile_image", HD_tile_image);

		hud.set("draw_text", HD_draw_text);
		hud.set("draw_num2", HD_draw_num2);
		hud.set("draw_number", HD_draw_number);

		hud.set("game_paused", HD_game_paused);
		hud.set("erraticism_active", HD_erraticism_active);
		hud.set("time_stop_active", HD_time_stop_active);

		hud.set("render_world", HD_render_world);
		hud.set("render_automap", HD_render_automap);

		hud.set("automap_color", HD_automap_color);
		hud.set("automap_option", HD_automap_option);
		hud.set("automap_zoom", HD_automap_zoom);

		hud.set("set_render_who", HD_set_render_who);

		hud.set("play_sound", HD_play_sound);

		hud.set("screen_aspect", HD_screen_aspect);

		hud.set("get_average_color", HD_get_average_color);
		hud.set("get_average_top_border_color",
				HD_get_average_top_border_color);
		hud.set("get_average_bottom_border_color",
				HD_get_average_bottom_border_color);
		hud.set("get_darkest_color", HD_get_darkest_color);
		hud.set("get_lightest_color", HD_get_lightest_color);
		hud.set("get_average_hue", HD_get_average_hue);

		hud.set("rts_enable", HD_rts_enable);
		hud.set("rts_isactive", HD_rts_isactive);

		Init("ui.lua");

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

	lua_vm_c *ui_vm = lua_vm_c::GetVM(LUA_VM_UI);
	ui_vm->Call("hud", "draw_all");

	HUD_Reset();
}

void LUA_Hud_Init(lua_vm_c *vm) { vm->addModule<lua_hud_module_c>(); }