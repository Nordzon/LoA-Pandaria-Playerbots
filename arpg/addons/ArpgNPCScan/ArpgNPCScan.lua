-- ArpgNPCScan (owner 2026-11-28): feeds _NPCScan with our Superuniques. Runs at PLAYER_LOGIN, after _NPCScan has loaded its
-- options (it registers for the event first, so its handler runs first), and only fills entries that are missing: your own
-- NPCScan edits (removing one, renaming) stay. _NPCScan activates custom NPCs when you enter the world.
-- /arpgnpcscan shows the counts; "/arpgnpcscan reset" re-adds anything you removed.
local frame = CreateFrame("Frame")
frame:RegisterEvent("PLAYER_LOGIN")

local function Apply(force)
    local options = _G._NPCScanOptions
    local data = _G.ArpgNPCScanData
    if type(options) ~= "table" or not data then
        return 0, 0
    end
    options.NPCs = options.NPCs or {}
    options.NPCWorldIDs = options.NPCWorldIDs or {}
    options.IgnoreList = options.IgnoreList or {}
    options.IgnoreList.NPCs = options.IgnoreList.NPCs or {}
    ArpgNPCScanDB = ArpgNPCScanDB or {}
    ArpgNPCScanDB.done = ArpgNPCScanDB.done or {}
    local done = ArpgNPCScanDB.done
    local added, ignored = 0, 0
    for id, name in pairs(data.watch) do
        if (force or not done[id]) and not options.NPCs[id] then
            options.NPCs[id] = name
            added = added + 1
        end
        done[id] = true
    end
    for id, name in pairs(data.ignore) do
        if (force or not done[-id]) and not options.IgnoreList.NPCs[id] then
            options.IgnoreList.NPCs[id] = name
            ignored = ignored + 1
        end
        done[-id] = true
    end
    return added, ignored
end

frame:SetScript("OnEvent", function()
    local added, ignored = Apply(false)
    if added > 0 or ignored > 0 then
        DEFAULT_CHAT_FRAME:AddMessage(string.format("|cff66ccffArpgNPCScan|r: %d Superuniques added to _NPCScan, %d replaced rares ignored.",
            added, ignored))
    end
end)

SLASH_ARPGNPCSCAN1 = "/arpgnpcscan"
SlashCmdList["ARPGNPCSCAN"] = function(msg)
    local added, ignored = Apply(msg == "reset")
    local w, i = 0, 0
    for _ in pairs(ArpgNPCScanData.watch) do w = w + 1 end
    for _ in pairs(ArpgNPCScanData.ignore) do i = i + 1 end
    DEFAULT_CHAT_FRAME:AddMessage(string.format("ArpgNPCScan: %d Superuniques, %d replaced rares (this time: +%d, +%d ignored). /reload to scan.",
        w, i, added, ignored))
end
