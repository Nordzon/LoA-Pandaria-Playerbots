-- ArpgUI: professions panel (change m1b task 2.1). A character here can know all 11 primary professions, but the client's own
-- spellbook tab shows only two, so the server sends the list as a hidden system line (".professions data"):
--   "ARPGT|skill:value:max:spell;skill:value:max:spell;..."   spell = the spell that opens the profession window, 0 = none
-- Each row is a secure spell button: clicking it opens that profession's window (cannot be changed in combat).

ArpgUIDB = ArpgUIDB or {}

local NAMES = {
    [171] = "Alchemy", [164] = "Blacksmithing", [333] = "Enchanting", [202] = "Engineering", [182] = "Herbalism",
    [773] = "Inscription", [755] = "Jewelcrafting", [165] = "Leatherworking", [186] = "Mining", [393] = "Skinning",
    [197] = "Tailoring",
}
local MAX_ROWS = 11
local ROW_H = 30

local profs = {}
local pending = false   -- data arrived in combat: apply when combat ends

local function Send(cmd)
    SendChatMessage(cmd, "SAY")
end

-- ---------------------------------------------------------------- window
local frame = CreateFrame("Frame", "ArpgProfessionsFrame", UIParent)
frame:SetSize(300, 70 + MAX_ROWS * ROW_H)
frame:SetPoint("CENTER", 220, 0)
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
tinsert(UISpecialFrames, "ArpgProfessionsFrame")

local close = CreateFrame("Button", nil, frame, "UIPanelCloseButton")
close:SetPoint("TOPRIGHT", -6, -6)

local title = frame:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge")
title:SetPoint("TOP", 0, -20)
title:SetText("Professions")

local empty = frame:CreateFontString(nil, "OVERLAY", "GameFontDisable")
empty:SetPoint("TOP", 0, -60)
empty:SetText("No professions learned yet - visit a trainer.")

local rows = {}
for i = 1, MAX_ROWS do
    local b = CreateFrame("Button", "ArpgProfessionRow" .. i, frame, "SecureActionButtonTemplate")
    b:SetSize(256, ROW_H - 2)
    b:SetPoint("TOPLEFT", 22, -46 - (i - 1) * ROW_H)
    b:RegisterForClicks("LeftButtonUp")
    b:SetHighlightTexture("Interface\\QuestFrame\\UI-QuestTitleHighlight", "ADD")
    b.icon = b:CreateTexture(nil, "ARTWORK")
    b.icon:SetSize(24, 24)
    b.icon:SetPoint("LEFT", 2, 0)
    b.name = b:CreateFontString(nil, "OVERLAY", "GameFontNormal")
    b.name:SetPoint("LEFT", b.icon, "RIGHT", 8, 0)
    b.skill = b:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
    b.skill:SetPoint("RIGHT", -4, 0)
    b:SetScript("OnEnter", function(self)
        if not self.skillId then return end
        GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
        GameTooltip:SetText(NAMES[self.skillId] or ("Skill " .. self.skillId))
        if self.spellName then
            GameTooltip:AddLine("Click: open the profession window", 1, 1, 1)
        else
            GameTooltip:AddLine("Gathering: no window to open", 0.7, 0.7, 0.7)
        end
        GameTooltip:Show()
    end)
    b:SetScript("OnLeave", function() GameTooltip:Hide() end)
    b:Hide()
    rows[i] = b
end

local function Refresh()
    if InCombatLockdown() then pending = true; return end
    pending = false
    for i = 1, MAX_ROWS do
        local b, p = rows[i], profs[i]
        if p then
            local spellName, _, icon
            if p.spell > 0 then spellName, _, icon = GetSpellInfo(p.spell) end
            b.skillId = p.skill
            b.spellName = spellName
            b.icon:SetTexture(icon or "Interface\\Icons\\INV_Misc_QuestionMark")
            b.name:SetText(NAMES[p.skill] or spellName or ("Skill " .. p.skill))
            b.skill:SetText(p.value .. " / " .. p.max)
            if spellName then
                b:SetAttribute("type", "spell")
                b:SetAttribute("spell", spellName)
            else
                b:SetAttribute("type", nil)
                b:SetAttribute("spell", nil)
            end
            b:Show()
        else
            b.skillId = nil
            b:Hide()
        end
    end
    if #profs == 0 then empty:Show() else empty:Hide() end
end

frame:SetScript("OnShow", function() Refresh(); Send(".professions data") end)

-- ---------------------------------------------------------------- server line
ChatFrame_AddMessageEventFilter("CHAT_MSG_SYSTEM", function(_, _, msg)
    if type(msg) ~= "string" or string.sub(msg, 1, 6) ~= "ARPGT|" then return false end
    profs = {}
    for skill, value, max, spell in string.gmatch(string.sub(msg, 7), "(%d+):(%d+):(%d+):(%d+)") do
        profs[#profs + 1] = { skill = tonumber(skill), value = tonumber(value), max = tonumber(max), spell = tonumber(spell) }
    end
    Refresh()
    return true
end)

local combat = CreateFrame("Frame")
combat:RegisterEvent("PLAYER_REGEN_ENABLED")
combat:RegisterEvent("SKILL_LINES_CHANGED")
combat:SetScript("OnEvent", function(_, event)
    if event == "PLAYER_REGEN_ENABLED" then
        if pending then Refresh() end
    elseif frame:IsShown() then
        Send(".professions data")   -- learned / skilled up while the panel is open
    end
end)

-- ---------------------------------------------------------------- launcher "T" (next to the Paragon "P" and loot "L")
local launcher = CreateFrame("Button", "ArpgProfessionsLauncher", UIParent, "UIPanelButtonTemplate")
launcher:SetSize(26, 26)
launcher:SetText("T")
launcher:SetMovable(true)
launcher:RegisterForDrag("LeftButton")
launcher:SetScript("OnDragStart", launcher.StartMoving)
launcher:SetScript("OnDragStop", function(self)
    self:StopMovingOrSizing()
    local point, _, rel, x, y = self:GetPoint()
    ArpgUIDB.profLauncher = { point, rel, x, y }
end)
launcher:SetScript("OnClick", function() if frame:IsShown() then frame:Hide() else frame:Show() end end)
launcher:SetScript("OnEnter", function(self)
    GameTooltip:SetOwner(self, "ANCHOR_LEFT")
    GameTooltip:SetText("Professions")
    GameTooltip:AddLine("Click: list your professions; click one to open it", 1, 1, 1)
    GameTooltip:Show()
end)
launcher:SetScript("OnLeave", function() GameTooltip:Hide() end)

SLASH_ARPGPROFESSIONS1 = "/professions"
SLASH_ARPGPROFESSIONS2 = "/prof"
SlashCmdList["ARPGPROFESSIONS"] = function() if frame:IsShown() then frame:Hide() else frame:Show() end end

local boot = CreateFrame("Frame")
boot:RegisterEvent("PLAYER_ENTERING_WORLD")
boot:SetScript("OnEvent", function()
    local saved = ArpgUIDB.profLauncher
    launcher:ClearAllPoints()
    if saved then
        launcher:SetPoint(saved[1], UIParent, saved[2], saved[3], saved[4])
    else
        launcher:SetPoint("TOPRIGHT", Minimap, "BOTTOMLEFT", 60, 4)   -- right of the loot "L"
    end
end)
