
local face_state
local face_tic
local face_image
local step_tic

-- Default all of these to black so that in a "worse-case scenario", it's just the old behavior of black areas
local custom_stbar_average_color = vec3(0, 0, 0)
local custom_stbar_darkest_color = vec3(0, 0, 0)
local custom_stbar_lightest_color  = vec3(0, 0, 0)
local custom_stbar_average_hue  = vec3(0, 0, 0)
    
function draw_all()

    hud.coord_sys(320, 200)
    hud.render_world(0, 0, 320, 200)

end

    