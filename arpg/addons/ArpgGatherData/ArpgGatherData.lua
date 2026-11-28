-- ArpgGatherData (owner 2026-11-28): our server's own node spawns -> GatherMate2. At login, once per data version, every node
-- in Data.lua is injected with GatherMate2's own InjectNode (types whose database you locked in GatherMate2 are skipped;
-- nodes you already have stay). Data.lua is generated from the world DB (modules/mod_arpg/sql/tools/gen_gather_data.py).
--   /arpggather          show counts and whether this version was imported
--   /arpggather import   import again (e.g. after clearing GatherMate2's database)
local frame = CreateFrame("Frame")
frame:RegisterEvent("PLAYER_LOGIN")

local TYPES = { "Mining", "Herb Gathering", "Fishing", "Treasure", "Extract Gas" }

local function GM()
    local ok, gm = pcall(function() return LibStub("AceAddon-3.0"):GetAddon("GatherMate2") end)
    return ok and gm or nil
end

local function Import()
    local gm = GM()
    local data = _G.ArpgGatherData
    if not gm or not gm.InjectNode or not data then
        return nil
    end
    local locks = gm.db and gm.db.profile and gm.db.profile.dbLocks or {}
    local counts, total = {}, 0
    for _, nodeType in ipairs(TYPES) do
        local n = 0
        if data[nodeType] and not locks[nodeType] then
            for zone, list in pairs(data[nodeType]) do
                for i = 1, #list, 2 do
                    gm:InjectNode(zone, list[i], nodeType, list[i + 1])
                    n = n + 1
                end
            end
        end
        counts[nodeType] = n
        total = total + n
    end
    if gm.SendMessage then
        gm:SendMessage("GatherMate2ConfigChanged")
    end
    ArpgGatherDB = ArpgGatherDB or {}
    ArpgGatherDB.imported = data.version
    return total, counts
end

local function Report(prefix, total, counts)
    DEFAULT_CHAT_FRAME:AddMessage(string.format("|cff66ccffArpgGatherData|r: %s %d nodes (ore %d, herbs %d, fishing %d, treasure %d).",
        prefix, total, counts["Mining"] or 0, counts["Herb Gathering"] or 0, counts["Fishing"] or 0, counts["Treasure"] or 0))
end

frame:SetScript("OnEvent", function()
    ArpgGatherDB = ArpgGatherDB or {}
    if ArpgGatherData and ArpgGatherDB.imported ~= ArpgGatherData.version then
        local total, counts = Import()
        if total then
            Report("imported", total, counts)
        end
    end
end)

SLASH_ARPGGATHER1 = "/arpggather"
SlashCmdList["ARPGGATHER"] = function(msg)
    if msg == "import" then
        local total, counts = Import()
        if total then
            Report("imported", total, counts)
        else
            DEFAULT_CHAT_FRAME:AddMessage("ArpgGatherData: GatherMate2 is not loaded.")
        end
        return
    end
    DEFAULT_CHAT_FRAME:AddMessage(string.format("ArpgGatherData: data version %s, imported %s. '/arpggather import' to import again.",
        tostring(ArpgGatherData and ArpgGatherData.version), tostring(ArpgGatherDB and ArpgGatherDB.imported)))
end
