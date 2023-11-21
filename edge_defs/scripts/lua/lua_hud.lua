

function doom_automap()

        -- Background is already black, only need to use 'solid_box'
        -- when we want a different color.
        --
        -- hud.solid_box(0, 0, 320, 200 - 32, '80 80 80')

        hud.render_automap(0, 0, 320, 200 - 32)

        local which = hud.which_hud() % 4

        if which == 0 then
            --doom_status_bar()
        elseif which == 1 then

            --doom_status_bar2()
        elseif which == 2 then
            --new_overlay_status()
        end


        hud.text_font("DOOM")
        hud.text_color(hud.GREEN)

        if (false) then--strings.len(hud.map_author()) > 0)

            hud.draw_text(0, 200 - 32 - 20, hud.map_title())
            hud.draw_text(0, 200 - 32 - 10, " Author: " + hud.map_author())
        else
            hud.draw_text(0, 200 - 32 - 10, hud.map_title())
            hud.draw_text(0, 200 - 32 - 10, hud.map_title())
        end

        hud.set_scale(0.75)
        hud.draw_text(10, 20, "Kills:    " .. player.kills() .. "/" .. player.map_enemies())

        if player.map_secrets() > 0 then
            hud.draw_text(10, 25, "Secrets: " .. player.secrets() .. "/" .. player.map_secrets())
        end

        if player.map_items() > 0 then
            hud.draw_text(10, 30, "Items:    " .. player.items() .. "/" .. player.map_items())
        end
        hud.set_scale(1.0)
end

function draw_all()
    hud.coord_sys(320, 200)

    if hud.check_automap() then
        doom_automap()
        return
    end

    hud.render_world(0, 0, 320, 200)
end
