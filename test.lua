if os.getenv("LOCAL_LUA_DEBUGGER_VSCODE") == "1" then
    require("lldebugger").start()
end

local sys = edge.sys
local hud = edge.hud;

print(sys, sys.TICRATE)

sys.print(sys.EDGEVER)


function doom_automap()

    hud.render_automap(0, 0, 320, 200 - 32)

    hud.set_scale(1.0)
end

function draw_all()
    hud.coord_sys(320, 200)

    if (hud.check_automap()) then
        doom_automap()
        return
    end

    hud.render_world(0, 0, 320, 200)

    
end
