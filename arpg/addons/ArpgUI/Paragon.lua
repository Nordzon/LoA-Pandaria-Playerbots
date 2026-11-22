-- ArpgUI: Paragon window (change m4-paragon). The server (mod_arpg) answers ".paragon data" with one system line
-- "ARPGP|level|xp|next|freeC,freeO,freeD,freeU|c0,c1|o0,o1,o2|d0,d1,d2|u0,u1,u2|statPct,rating,armorPct,move,moveCap,find,mfCap"
-- which this addon reads and hides from chat. Spending sends ".paragon spend <tab> <stat> [n]"; the server replies with a
-- fresh data line. Diablo 3 rotation: each Paragon level gives one point to the next tab; you pick the stat inside it.

ArpgUIDB = ArpgUIDB or {}

local TABS = {
    { key = "core",    title = "Core",    stats = { { "main", "Main stat" }, { "stamina", "Stamina" } } },
    { key = "offense", title = "Offense", stats = { { "crit", "Critical strike" }, { "haste", "Haste" }, { "mastery", "Mastery" } } },
    { key = "defense", title = "Defense", stats = { { "armor", "Armor" }, { "dodge", "Dodge" }, { "parry", "Parry" } } },
    { key = "utility", title = "Utility", stats = { { "speed", "Movement speed" }, { "magicfind", "Magic find" }, { "goldfind", "Gold find" } } },
}

local data = nil   -- parsed server line
local rows = {}    -- [tab][stat] = { label, value, button }
local tabHeaders = {}

local function Send(cmd)
    SendChatMessage(cmd, "SAY")
end

local function Split(s, sep)
    local out = {}
    for part in string.gmatch(s, "([^" .. sep .. "]+)") do out[#out + 1] = part end
    return out
end

local function Parse(msg)
    local f = Split(msg, "|")
    if #f < 10 then return nil end
    local d = { level = tonumber(f[2]) or 0, xp = tonumber(f[3]) or 0, nextXp = tonumber(f[4]) or 1, free = {}, pts = {} }
    local free = Split(f[5], ",")
    for t = 1, 4 do d.free[t] = tonumber(free[t]) or 0 end
    for t = 1, 4 do
        d.pts[t] = {}
        local p = Split(f[5 + t], ",")
        for i = 1, #TABS[t].stats do d.pts[t][i] = tonumber(p[i]) or 0 end
    end
    local c = Split(f[10], ",")
    d.statPct, d.rating, d.armorPct = tonumber(c[1]) or 0, tonumber(c[2]) or 0, tonumber(c[3]) or 0
    d.move, d.moveCap, d.find, d.mfCap = tonumber(c[4]) or 0, tonumber(c[5]) or 0, tonumber(c[6]) or 0, tonumber(c[7]) or 0
    return d
end

local function BonusText(t, i, n)
    if not data then return "" end
    if t == 1 then return string.format("+%.1f%%", n * data.statPct) end
    if t == 2 then return string.format("+%d rating", n * data.rating) end
    if t == 3 then
        if i == 1 then return string.format("+%.1f%%", n * data.armorPct) end
        return string.format("+%d rating", n * data.rating)
    end
    if i == 1 then return string.format("+%.0f%%", math.min(n * data.move, data.moveCap) / 100) end
    if i == 2 then return string.format("+%.0f%%", math.min(n * data.find, data.mfCap) / 100) end
    return string.format("+%.0f%%", n * data.find / 100)
end

-- ---------------------------------------------------------------- window
local frame = CreateFrame("Frame", "ArpgParagonFrame", UIParent)
frame:SetSize(420, 470)
frame:SetPoint("CENTER")
frame:SetFrameStrata("DIALOG")
frame:SetBackdrop({
    bgFile = "Interface\\DialogFrame\\UI-DialogBox-Background",
    edgeFile = "Interface\\DialogFrame\\UI-DialogBox-Border",
    tile = true, tileSize = 32, edgeSize = 32,
    insets = { left = 11, right = 12, top = 12, bottom = 11 },
})
frame:EnableMouse(true)
frame:SetMovable(true)
frame:RegisterForDrag("LeftButton")
frame:SetScript("OnDragStart", frame.StartMoving)
frame:SetScript("OnDragStop", frame.StopMovingOrSizing)
frame:Hide()
tinsert(UISpecialFrames, "ArpgParagonFrame")   -- Escape closes it

local close = CreateFrame("Button", nil, frame, "UIPanelCloseButton")
close:SetPoint("TOPRIGHT", -6, -6)

local title = frame:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge")
title:SetPoint("TOP", 0, -20)
title:SetText("Paragon")

local bar = CreateFrame("StatusBar", nil, frame)
bar:SetSize(360, 16)
bar:SetPoint("TOP", 0, -50)
bar:SetStatusBarTexture("Interface\\TargetingFrame\\UI-StatusBar")
bar:SetStatusBarColor(0.95, 0.75, 0.1)
bar:SetMinMaxValues(0, 1)
local barBg = bar:CreateTexture(nil, "BACKGROUND")
barBg:SetAllPoints()
barBg:SetTexture(0, 0, 0, 0.6)
local barText = bar:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
barText:SetPoint("CENTER")

local hint = frame:CreateFontString(nil, "OVERLAY", "GameFontDisableSmall")
hint:SetPoint("TOP", bar, "BOTTOM", 0, -6)
hint:SetText("Each level gives a point to the next tab. Shift-click: +5.")

local y = -96
for t, tab in ipairs(TABS) do
    local header = frame:CreateFontString(nil, "OVERLAY", "GameFontNormal")
    header:SetPoint("TOPLEFT", 28, y)
    tabHeaders[t] = header
    y = y - 20
    rows[t] = {}
    for i, stat in ipairs(tab.stats) do
        local label = frame:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
        label:SetPoint("TOPLEFT", 44, y)
        label:SetText(stat[2])
        local value = frame:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
        value:SetPoint("TOPRIGHT", -70, y)
        local button = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
        button:SetSize(26, 20)
        button:SetPoint("TOPRIGHT", -32, y + 3)
        button:SetText("+")
        button:SetScript("OnClick", function()
            local n = IsShiftKeyDown() and 5 or 1
            if data then n = math.min(n, data.free[t]) end
            if n > 0 then Send(".paragon spend " .. tab.key .. " " .. stat[1] .. " " .. n) end
        end)
        rows[t][i] = { label = label, value = value, button = button }
        y = y - 20
    end
    y = y - 8
end

local respec = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
respec:SetSize(110, 24)
respec:SetPoint("BOTTOM", 0, 20)
respec:SetText("Respec (free)")
respec:SetScript("OnClick", function() Send(".paragon respec") end)

local function Refresh()
    if not data then
        title:SetText("Paragon")
        barText:SetText("waiting for the server...")
        return
    end
    title:SetText("Paragon " .. data.level)
    bar:SetMinMaxValues(0, math.max(1, data.nextXp))
    bar:SetValue(data.xp)
    barText:SetText(string.format("%d / %d XP", data.xp, data.nextXp))
    for t, tab in ipairs(TABS) do
        local free = data.free[t] or 0
        tabHeaders[t]:SetText(tab.title .. (free > 0 and ("  |cff00ff00(" .. free .. " unspent)|r") or ""))
        for i in ipairs(tab.stats) do
            local n = data.pts[t][i] or 0
            local r = rows[t][i]
            r.value:SetText(n > 0 and (n .. " pts  |cff00ff00" .. BonusText(t, i, n) .. "|r") or "|cff808080-|r")
            if free > 0 then r.button:Enable() else r.button:Disable() end
        end
    end
end

frame:SetScript("OnShow", function()
    Refresh()
    Send(".paragon data")
end)

-- ---------------------------------------------------------------- server lines
ChatFrame_AddMessageEventFilter("CHAT_MSG_SYSTEM", function(_, _, msg)
    if type(msg) == "string" and string.sub(msg, 1, 6) == "ARPGP|" then
        local d = Parse(msg)
        if d then
            data = d
            Refresh()
        end
        return true   -- hidden from chat
    end
    return false
end)

-- ---------------------------------------------------------------- launcher: /paragon and a small button by the minimap
local function Toggle()
    if frame:IsShown() then frame:Hide() else frame:Show() end
end

SLASH_ARPGPARAGON1 = "/paragon"
SLASH_ARPGPARAGON2 = "/pg"
SlashCmdList["ARPGPARAGON"] = Toggle

local launcher = CreateFrame("Button", "ArpgParagonLauncher", UIParent, "UIPanelButtonTemplate")
launcher:SetSize(26, 26)
launcher:SetText("P")
launcher:SetMovable(true)
launcher:RegisterForDrag("LeftButton")
launcher:SetScript("OnDragStart", launcher.StartMoving)
launcher:SetScript("OnDragStop", function(self)
    self:StopMovingOrSizing()
    local point, _, rel, x, y = self:GetPoint()
    ArpgUIDB.launcher = { point, rel, x, y }
end)
launcher:SetScript("OnClick", Toggle)
launcher:SetScript("OnEnter", function(self)
    GameTooltip:SetOwner(self, "ANCHOR_LEFT")
    GameTooltip:SetText("Paragon")
    if data then
        GameTooltip:AddLine("Level " .. data.level .. "  (" .. data.xp .. " / " .. data.nextXp .. " XP)", 1, 1, 1)
        local free = (data.free[1] or 0) + (data.free[2] or 0) + (data.free[3] or 0) + (data.free[4] or 0)
        if free > 0 then GameTooltip:AddLine(free .. " points to spend", 0, 1, 0) end
    end
    GameTooltip:Show()
end)
launcher:SetScript("OnLeave", function() GameTooltip:Hide() end)

local boot = CreateFrame("Frame")
boot:RegisterEvent("PLAYER_ENTERING_WORLD")
local wait = 0
boot:SetScript("OnEvent", function(self)
    local saved = ArpgUIDB.launcher
    launcher:ClearAllPoints()
    if saved then
        launcher:SetPoint(saved[1], UIParent, saved[2], saved[3], saved[4])
    else
        launcher:SetPoint("TOPRIGHT", Minimap, "BOTTOMLEFT", 4, 4)
    end
    wait = 3   -- ask the server a moment after loading (chat must be ready)
    self:SetScript("OnUpdate", function(s, elapsed)
        wait = wait - elapsed
        if wait <= 0 then
            s:SetScript("OnUpdate", nil)
            Send(".paragon data")
        end
    end)
end)
