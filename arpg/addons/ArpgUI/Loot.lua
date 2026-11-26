-- ArpgUI: loot companion toggle and loot filter panel (change m2b tasks 4.1/4.2). Talks to mod_arpg through .lootpet and
-- .lootfilter (player commands); the server answers with hidden system lines:
--   "ARPGL|<0/1>"                      loot companion on/off
--   "ARPGF|<count>|<rule>|<rule>|..."  filter rules in order (text form, e.g. "quality<=0 -> sell")

ArpgUIDB = ArpgUIDB or {}

local petOn = false
local rules = {}

local function Send(cmd)
    SendChatMessage(cmd, "SAY")
end

local function Split(s, sep)
    local out = {}
    for part in string.gmatch(s, "([^" .. sep .. "]+)") do out[#out + 1] = part end
    return out
end

-- Human-readable form of a rule ("quality<=0 -> sell" -> "Grey and below: sell").
local QUALITY = { [0] = "Grey", [1] = "White", [2] = "Green", [3] = "Blue", [4] = "Purple", [5] = "Orange" }
local function Pretty(rule)
    local parts = {}
    local cond, action = string.match(rule, "^(.-)%s*%->%s*(%a+)$")
    if not cond then return rule end
    local qmin = tonumber(string.match(cond, "quality>=(%d+)"))
    local qmax = tonumber(string.match(cond, "quality<=(%d+)"))
    if qmin and qmax and qmin == qmax then parts[#parts + 1] = QUALITY[qmin] or ("q" .. qmin)
    elseif qmax then parts[#parts + 1] = (QUALITY[qmax] or qmax) .. " and below"
    elseif qmin then parts[#parts + 1] = (QUALITY[qmin] or qmin) .. " and above" end
    if string.find(cond, "usable=0") then parts[#parts + 1] = "not usable by my class" end
    if string.find(cond, "usable=1") then parts[#parts + 1] = "usable" end
    local a = string.match(cond, "affixes>=(%d+)"); if a then parts[#parts + 1] = a .. "+ affixes" end
    local t = string.match(cond, "tier>=(%d+)"); if t then parts[#parts + 1] = "a T" .. t .. "+ affix" end
    if string.find(cond, "vs=0") then parts[#parts + 1] = "worse than equipped" end
    if string.find(cond, "vs=1") then parts[#parts + 1] = "upgrade" end
    if string.find(cond, "ids=") then parts[#parts + 1] = "listed items" end
    if string.find(cond, "class=") then parts[#parts + 1] = "item class " .. string.match(cond, "class=(%d+)") end
    local what = #parts > 0 and table.concat(parts, ", ") or "everything"
    local verbs = { pickup = "|cff00ff00pick up|r", leave = "|cffff8000leave|r", sell = "|cffffd100sell|r", highlight = "|cff66ccffhighlight|r" }
    return what .. ": " .. (verbs[action] or action)
end

-- ---------------------------------------------------------------- filter window
local frame = CreateFrame("Frame", "ArpgLootFilterFrame", UIParent)
frame:SetSize(460, 420)
frame:SetPoint("CENTER", 40, 0)
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
tinsert(UISpecialFrames, "ArpgLootFilterFrame")

local close = CreateFrame("Button", nil, frame, "UIPanelCloseButton")
close:SetPoint("TOPRIGHT", -6, -6)

local title = frame:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge")
title:SetPoint("TOP", 0, -20)
title:SetText("Loot filter")

local hint = frame:CreateFontString(nil, "OVERLAY", "GameFontDisableSmall")
hint:SetPoint("TOP", title, "BOTTOM", 0, -4)
hint:SetText("First matching rule wins. Quest items and Uniques are always kept.")

local MAX_ROWS = 10
local rows = {}
for i = 1, MAX_ROWS do
    local y = -58 - (i - 1) * 24
    local label = frame:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
    label:SetPoint("TOPLEFT", 26, y)
    label:SetWidth(330)
    label:SetJustifyH("LEFT")
    local up = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
    up:SetSize(22, 20); up:SetPoint("TOPRIGHT", -76, y + 3); up:SetText("^")
    up:SetScript("OnClick", function() if i > 1 then Send(".lootfilter move " .. i .. " " .. (i - 1)) end end)
    local down = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
    down:SetSize(22, 20); down:SetPoint("TOPRIGHT", -52, y + 3); down:SetText("v")
    down:SetScript("OnClick", function() if i < #rules then Send(".lootfilter move " .. i .. " " .. (i + 1)) end end)
    local del = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
    del:SetSize(22, 20); del:SetPoint("TOPRIGHT", -28, y + 3); del:SetText("x")
    del:SetScript("OnClick", function() Send(".lootfilter remove " .. i) end)
    rows[i] = { label = label, up = up, down = down, del = del }
end

local edit = CreateFrame("EditBox", "ArpgLootFilterEdit", frame, "InputBoxTemplate")
edit:SetSize(300, 20)
edit:SetPoint("BOTTOMLEFT", 30, 82)
edit:SetAutoFocus(false)
edit:SetScript("OnEnterPressed", function(self)
    local text = self:GetText()
    if text and text ~= "" then Send(".lootfilter add " .. text) end
    self:SetText("")
    self:ClearFocus()
end)
local add = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
add:SetSize(70, 22)
add:SetPoint("LEFT", edit, "RIGHT", 8, 0)
add:SetText("Add")
add:SetScript("OnClick", function() edit:GetScript("OnEnterPressed")(edit) end)
local syntax = frame:CreateFontString(nil, "OVERLAY", "GameFontDisableSmall")
syntax:SetPoint("BOTTOMLEFT", edit, "TOPLEFT", -4, 4)
syntax:SetText("Rule: quality<=0 usable=0 affixes>=3 tier>=5 vs=0 class=4 ids=1,2 -> sell|leave|pickup|highlight")

-- Quick rules.
local quick = {
    { "Sell greys", "quality<=0 -> sell" },
    { "Sell whites", "quality=1 -> sell" },
    { "Leave unusable", "usable=0 -> leave" },
    { "Highlight T5", "tier>=5 -> highlight" },
}
for i, q in ipairs(quick) do
    local b = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
    b:SetSize(100, 22)
    b:SetPoint("BOTTOMLEFT", 26 + (i - 1) * 104, 46)
    b:SetText(q[1])
    b:SetScript("OnClick", function() Send(".lootfilter add " .. q[2]) end)
end

local reset = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
reset:SetSize(110, 22)
reset:SetPoint("BOTTOM", 0, 18)
reset:SetText("Reset to default")
reset:SetScript("OnClick", function() Send(".lootfilter reset") end)

local function Refresh()
    for i = 1, MAX_ROWS do
        local r = rows[i]
        local rule = rules[i]
        if rule then
            r.label:SetText(i .. ". " .. Pretty(rule))
            r.up:Show(); r.down:Show(); r.del:Show()
            if i == 1 then r.up:Disable() else r.up:Enable() end
            if i == #rules then r.down:Disable() else r.down:Enable() end
        else
            r.label:SetText("")
            r.up:Hide(); r.down:Hide(); r.del:Hide()
        end
    end
    if #rules == 0 then rows[1].label:SetText("|cff808080no rules - everything is picked up|r") end
end

frame:SetScript("OnShow", function() Refresh(); Send(".lootfilter data") end)

-- ---------------------------------------------------------------- loot companion button
local pet = CreateFrame("Button", "ArpgLootPetLauncher", UIParent, "UIPanelButtonTemplate")
pet:SetSize(26, 26)
pet:SetText("L")
pet:SetMovable(true)
pet:RegisterForDrag("LeftButton")
pet:RegisterForClicks("LeftButtonUp", "RightButtonUp")
pet:SetScript("OnDragStart", pet.StartMoving)
pet:SetScript("OnDragStop", function(self)
    self:StopMovingOrSizing()
    local point, _, rel, x, y = self:GetPoint()
    ArpgUIDB.lootLauncher = { point, rel, x, y }
end)
pet:SetScript("OnClick", function(_, button)
    if button == "RightButton" then
        if frame:IsShown() then frame:Hide() else frame:Show() end
    else
        Send(petOn and ".lootpet off" or ".lootpet on")
    end
end)
local function PetLook()
    if petOn then pet:LockHighlight() else pet:UnlockHighlight() end
end
pet:SetScript("OnEnter", function(self)
    GameTooltip:SetOwner(self, "ANCHOR_LEFT")
    GameTooltip:SetText("Loot companion: " .. (petOn and "|cff00ff00on|r" or "|cff808080off|r"))
    GameTooltip:AddLine("Left-click: on / off", 1, 1, 1)
    GameTooltip:AddLine("Right-click: loot filter", 1, 1, 1)
    GameTooltip:AddLine("Right-click the companion itself for its shop, bank, mail and looks.", 0.7, 0.7, 0.7, true)
    GameTooltip:Show()
end)
pet:SetScript("OnLeave", function() GameTooltip:Hide() end)

-- ---------------------------------------------------------------- server lines
ChatFrame_AddMessageEventFilter("CHAT_MSG_SYSTEM", function(_, _, msg)
    if type(msg) ~= "string" then return false end
    if string.sub(msg, 1, 6) == "ARPGL|" then
        petOn = string.sub(msg, 7, 7) == "1"
        PetLook()
        return true
    end
    if string.sub(msg, 1, 6) == "ARPGF|" then
        local f = Split(msg, "|")
        rules = {}
        for i = 3, #f do rules[#rules + 1] = f[i] end
        Refresh()
        return true
    end
    return false
end)

SLASH_ARPGLOOTFILTER1 = "/lootfilter"
SLASH_ARPGLOOTFILTER2 = "/lf"
SlashCmdList["ARPGLOOTFILTER"] = function() if frame:IsShown() then frame:Hide() else frame:Show() end end

local boot = CreateFrame("Frame")
boot:RegisterEvent("PLAYER_ENTERING_WORLD")
local wait = 0
boot:SetScript("OnEvent", function(self)
    local saved = ArpgUIDB.lootLauncher
    pet:ClearAllPoints()
    if saved then
        pet:SetPoint(saved[1], UIParent, saved[2], saved[3], saved[4])
    else
        pet:SetPoint("TOPRIGHT", Minimap, "BOTTOMLEFT", 32, 4)   -- next to the Paragon "P"
    end
    wait = 3.5
    self:SetScript("OnUpdate", function(s, elapsed)
        wait = wait - elapsed
        if wait <= 0 then
            s:SetScript("OnUpdate", nil)
            Send(".lootpet data")
        end
    end)
end)
