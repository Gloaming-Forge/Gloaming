-- scripts/levels/level_1.lua
-- "Green Hills" — teaches moving, jumping, collecting coins.
-- 60 tiles wide x 12 tiles tall. Grass biome.
--
-- Legend:
--   0 = air
--   1 = solid terrain (auto-tiled)
--   2 = cloud platform (one-way)
--
-- Grid is [row][col], top-to-bottom, left-to-right.

return {
    name = "Green Hills",
    width = 60,
    height = 12,
    player_spawn = { x = 3, y = 9 },
    background = "background_color_hills",
    biome = "grass",

    tiles = {
        -- Row 1 (top — mostly sky)
        { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
        -- Row 2
        { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
        -- Row 3
        { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
        -- Row 4
        { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
        -- Row 5 — floating cloud platform
        { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,0,0,0,0,0,0 },
        -- Row 6
        { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
        -- Row 7 — some platforms
        { 0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
        -- Row 8
        { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
        -- Row 9 — main ground with gaps
        { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
        -- Row 10 — main walking surface
        { 1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
        -- Row 11
        { 1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
        -- Row 12 (bottom — solid base)
        { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
    },

    entities = {
        -- Coins guiding the player forward
        { type = "coin",  x = 5,  y = 9 },
        { type = "coin",  x = 6,  y = 9 },
        { type = "coin",  x = 7,  y = 9 },

        -- Coins above first gap (reward for jumping)
        { type = "coin",  x = 11, y = 8 },

        -- Coins on first mid-air platform
        { type = "coin",  x = 13, y = 6 },
        { type = "coin",  x = 14, y = 6 },

        -- Coins on floating cloud platform
        { type = "coin",  x = 20, y = 4 },
        { type = "coin",  x = 21, y = 4 },

        -- Slime enemy on main ground
        { type = "slime", x = 16, y = 9 },

        -- More coins
        { type = "coin",  x = 27, y = 9 },
        { type = "coin",  x = 28, y = 9 },

        -- Coins on second platform
        { type = "coin",  x = 27, y = 6 },
        { type = "coin",  x = 28, y = 6 },

        -- Second slime
        { type = "slime", x = 33, y = 9 },

        -- Coins on high cloud platform
        { type = "coin",  x = 36, y = 4 },
        { type = "coin",  x = 37, y = 4 },
        { type = "coin",  x = 38, y = 4 },

        -- Fly enemy over third gap
        { type = "fly",   x = 40, y = 7 },

        -- Coins on third platform
        { type = "coin",  x = 44, y = 6 },

        -- Coins leading to the end
        { type = "coin",  x = 48, y = 9 },
        { type = "coin",  x = 50, y = 9 },
        { type = "coin",  x = 52, y = 4 },
        { type = "coin",  x = 53, y = 4 },

        -- End flag
        { type = "flag",  x = 58, y = 9 },
    }
}
