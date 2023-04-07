local lib = {};

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

    hud.render_world(0, 0, 320, 200)
end


lib.draw_all = draw_all;

return lib
