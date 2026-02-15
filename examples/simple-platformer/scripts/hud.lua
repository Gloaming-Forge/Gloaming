-- scripts/hud.lua
-- HUD layout and dynamic updates.

local hud_mod = {}

function hud_mod.init()
    ui.register("hud", function()
        local state = _G.game_state

        -- Build heart children
        local heart_children = {}
        for i = 1, 3 do
            local icon = (i <= state.lives) and "hud_heart" or "hud_heart_empty"
            table.insert(heart_children, ui.Image({
                id = "heart_" .. i,
                style = { width = 40, height = 40 }
            }))
        end

        return ui.Box({ id = "hud_root", style = {
            width = "100%", height = "auto",
            flex_direction = "row",
            justify_content = "space_between",
            align_items = "center",
            padding = { top = 16, left = 16, right = 16 }
        }}, {
            -- Lives (left)
            ui.Box({ id = "lives_row", style = {
                flex_direction = "row", gap = 8, align_items = "center"
            }}, heart_children),

            -- Level name (center)
            ui.Text({ id = "level_name", text = "Level " .. state.current_level, style = {
                font_size = 28, text_color = "#FFFFFFFF"
            }}),

            -- Coins (right)
            ui.Box({ id = "coin_display", style = {
                flex_direction = "row", gap = 8, align_items = "center"
            }}, {
                ui.Image({ id = "coin_icon", style = { width = 32, height = 32 }}),
                ui.Text({ id = "coin_text", text = "x " .. state.coins, style = {
                    font_size = 24, text_color = "#FFFFFFFF"
                }})
            })
        })
    end)
end

function hud_mod.update()
    local state = _G.game_state

    -- Update hearts
    for i = 1, 3 do
        -- Hearts are image elements; update visibility or style
        ui.setVisible("heart_" .. i, true)
    end

    -- Update text elements
    ui.setText("level_name", "Level " .. state.current_level)
    ui.setText("coin_text", "x " .. state.coins)

    -- Force rebuild of dynamic elements
    ui.markDirty("hud")
end

function hud_mod.show()
    ui.show("hud")
end

function hud_mod.hide()
    ui.hide("hud")
end

-- Initialize on load
hud_mod.init()

return hud_mod
