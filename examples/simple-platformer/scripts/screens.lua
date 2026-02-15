-- scripts/screens.lua
-- Title, pause, game over, and victory screens.

local screens_mod = {}

-- ── Title Screen ─────────────────────────────────────────────────────────────

function screens_mod.show_title()
    ui.register("title_screen", function()
        return ui.Box({ id = "title_root", style = {
            width = "100%", height = "100%",
            flex_direction = "column",
            justify_content = "center",
            align_items = "center",
            background = "#1a1a2eFF",
            gap = 40
        }}, {
            ui.Text({ id = "title_text", text = "Simple Platformer", style = {
                font_size = 48, text_color = "#FFFFFFFF"
            }}),
            ui.Text({ id = "title_subtitle", text = "A Gloaming Engine Example", style = {
                font_size = 20, text_color = "#AAAAAAFF"
            }}),
            ui.Button({ id = "start_btn", label = "Start Game", style = {
                width = 200, height = 50,
                font_size = 24,
                text_color = "#FFFFFFFF",
                background = "#4a9e4aFF"
            }, on_click = function()
                ui.hide("title_screen")
                events.emit("restart_game")
            end}),
        })
    end)
    ui.show("title_screen")
    ui.setBlocking("title_screen", true)
end

-- ── Pause Screen ─────────────────────────────────────────────────────────────

function screens_mod.show_pause()
    ui.register("pause_screen", function()
        return ui.Box({ id = "pause_root", style = {
            width = "100%", height = "100%",
            flex_direction = "column",
            justify_content = "center",
            align_items = "center",
            background = "#00000099",
            gap = 30
        }}, {
            ui.Text({ id = "pause_text", text = "Paused", style = {
                font_size = 48, text_color = "#FFFFFFFF"
            }}),
            ui.Button({ id = "resume_btn", label = "Resume", style = {
                width = 200, height = 50,
                font_size = 24,
                text_color = "#FFFFFFFF",
                background = "#4a9e4aFF"
            }, on_click = function()
                events.emit("pause_toggle")
            end}),
            ui.Button({ id = "quit_pause_btn", label = "Quit to Title", style = {
                width = 200, height = 50,
                font_size = 24,
                text_color = "#FFFFFFFF",
                background = "#9e4a4aFF"
            }, on_click = function()
                ui.hide("pause_screen")
                screens_mod.show_title()
            end}),
        })
    end)
    ui.show("pause_screen")
    ui.setBlocking("pause_screen", true)
end

function screens_mod.hide_pause()
    ui.hide("pause_screen")
end

-- ── Game Over Screen ─────────────────────────────────────────────────────────

function screens_mod.show_game_over(final_score)
    ui.register("gameover_screen", function()
        return ui.Box({ id = "gameover_root", style = {
            width = "100%", height = "100%",
            flex_direction = "column",
            justify_content = "center",
            align_items = "center",
            background = "#1a1a2eFF",
            gap = 30
        }}, {
            ui.Text({ id = "gameover_text", text = "Game Over", style = {
                font_size = 48, text_color = "#FF4444FF"
            }}),
            ui.Text({ id = "gameover_score", text = "Coins: " .. (final_score or 0), style = {
                font_size = 24, text_color = "#FFFFFFCC"
            }}),
            ui.Button({ id = "retry_btn", label = "Try Again", style = {
                width = 200, height = 50,
                font_size = 24,
                text_color = "#FFFFFFFF",
                background = "#4a9e4aFF"
            }, on_click = function()
                ui.hide("gameover_screen")
                events.emit("restart_game")
            end}),
            ui.Button({ id = "quit_gameover_btn", label = "Quit to Title", style = {
                width = 200, height = 50,
                font_size = 24,
                text_color = "#FFFFFFFF",
                background = "#9e4a4aFF"
            }, on_click = function()
                ui.hide("gameover_screen")
                screens_mod.show_title()
            end}),
        })
    end)
    ui.show("gameover_screen")
    ui.setBlocking("gameover_screen", true)
end

-- ── Victory Screen ───────────────────────────────────────────────────────────

function screens_mod.show_victory(final_score)
    ui.register("victory_screen", function()
        return ui.Box({ id = "victory_root", style = {
            width = "100%", height = "100%",
            flex_direction = "column",
            justify_content = "center",
            align_items = "center",
            background = "#1a2e1aFF",
            gap = 30
        }}, {
            ui.Text({ id = "victory_text", text = "You Win!", style = {
                font_size = 48, text_color = "#44FF44FF"
            }}),
            ui.Text({ id = "victory_score", text = "Coins: " .. (final_score or 0), style = {
                font_size = 24, text_color = "#FFFFFFCC"
            }}),
            ui.Button({ id = "playagain_btn", label = "Play Again", style = {
                width = 200, height = 50,
                font_size = 24,
                text_color = "#FFFFFFFF",
                background = "#4a9e4aFF"
            }, on_click = function()
                ui.hide("victory_screen")
                events.emit("restart_game")
            end}),
            ui.Button({ id = "quit_victory_btn", label = "Quit to Title", style = {
                width = 200, height = 50,
                font_size = 24,
                text_color = "#FFFFFFFF",
                background = "#9e4a4aFF"
            }, on_click = function()
                ui.hide("victory_screen")
                screens_mod.show_title()
            end}),
        })
    end)
    ui.show("victory_screen")
    ui.setBlocking("victory_screen", true)
end

return screens_mod
