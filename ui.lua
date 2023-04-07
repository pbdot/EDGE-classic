if os.getenv("LOCAL_LUA_DEBUGGER_VSCODE") == "1" then
    require("lldebugger").start()
  end

local exports = {};

-- Default all of these to black so that in a "worse-case scenario", it's just the old behavior of black areas
local custom_stbar_average_color = { 0, 0, 0 }
local custom_stbar_darkest_color = { 0, 0, 0 }
local custom_stbar_lightest_color = { 0, 0, 0 }
local custom_stbar_average_hue = { 0, 0, 0 }

local function doom_weapon_icon(slot, x, y, off_pic, on_pic)
    --if (player.has_weapon_slot(slot))
    --   hud.draw_image(x, y, on_pic)
    --else
    hud.draw_image(x, y, off_pic)
    --end
end

local function check_face_state()
    return 6;
end


local function doomguy_face(x, y)
    hud.draw_image(x - 1, y - 1, "STFGOD0")
end


local function doom_status_bar()
    hud.draw_image(0, 168, "STBAR")

    if not hud.custom_stbar then
        hud.draw_image(-83, 168, "STBARL") -- Widescreen border
        hud.draw_image(320, 168, "STBARR") -- Widescreen border
    else
        hud.solid_box(-83, 168, 83, 32, custom_stbar_average_color)
        hud.solid_box(320, 168, 83, 32, custom_stbar_average_color)
    end

    hud.draw_image(90, 171, "STTPRCNT")
    hud.draw_image(221, 171, "STTPRCNT")

    hud.text_font("BIG_DIGIT")

    --hud.draw_num2(44, 171, 3, player.main_ammo(1))
    --hud.draw_num2(90, 171, 3, player.health())
    --hud.draw_num2(221, 171, 3, player.total_armor())

    if hud.game_mode() == "dm" then
        -- hud.draw_num2(138, 171, 2, player.frags())
    else
        hud.draw_image(104, 168, "STARMS")

        doom_weapon_icon(2, 111, 172, "STGNUM2", "STYSNUM2")
        doom_weapon_icon(3, 123, 172, "STGNUM3", "STYSNUM3")
        doom_weapon_icon(4, 135, 172, "STGNUM4", "STYSNUM4")

        doom_weapon_icon(5, 111, 182, "STGNUM5", "STYSNUM5")
        doom_weapon_icon(6, 123, 182, "STGNUM6", "STYSNUM6")
        doom_weapon_icon(7, 135, 182, "STGNUM7", "STYSNUM7")
    end

    doomguy_face(144, 169)

    --doom_key(239, 171, 1, 5, "STKEYS0", "STKEYS3", "STKEYS6")
    --doom_key(239, 181, 2, 6, "STKEYS1", "STKEYS4", "STKEYS7")
    --doom_key(239, 191, 3, 7, "STKEYS2", "STKEYS5", "STKEYS8")

    --doom_little_ammo()
end

local function doom_automap()
    hud.render_automap(0, 0, 320, 200 - 32)

    hud.set_scale(1.0)
end

local function draw_all()
    hud.coord_sys(320, 200)

    if (hud.check_automap()) then
        doom_automap()
        return
    end

    -- there are four standard HUDs:
    -- the first two have different styles for the widescreen status bar extenders
    -- the third is the fullscreen hud
    -- the fourth is "hudless", i.e. you can only see your weapon

    local which = hud.which_hud() % 4

    if (which == 0) then
        hud.universal_y_adjust = -16
        hud.render_world(0, 0, 320, 200 - 32)
        doom_status_bar()
    elseif which == 1 then
        hud.universal_y_adjust = -16
        hud.render_world(0, 0, 320, 200 - 32)
        --doom_status_bar2()
    elseif which == 2 then
        hud.universal_y_adjust = 0
        hud.render_world(0, 0, 320, 200)
        new_overlay_status()
    else
        hud.universal_y_adjust = 0
        --hud.render_world(0, 0, 320, 200)
    end
end


exports.draw_all = draw_all;

return exports
