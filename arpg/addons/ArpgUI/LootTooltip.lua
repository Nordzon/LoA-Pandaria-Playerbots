-- Corpse loot (owner 2026-11-27): the loot window tooltip shows the affix lines the item gets when picked up. The server
-- fixes the values when the loot is made; ".lootinfo data" answers one hidden line per planned item:
--   "ARPGV|<item id>|<suffix id>|s:<stat>:<value>;e:<effect>:<tenths, 0 = no value>;u:<unique id>:0"
-- Texts: AffixText.lua (generated from the client patch builder).
local preview = {}   -- ["<item id>:<suffix id>"] = encoded lines

local function KeyOf(link)
    if not link then return nil end
    -- item:id:enchant:gem1:gem2:gem3:gem4:suffix:...
    local id, suffix = link:match("item:(%-?%d+):%-?%d+:%-?%d+:%-?%d+:%-?%d+:%-?%d+:(%-?%d+)")
    if id then return id .. ":" .. suffix end
end

local function Tenths(t)
    if t % 10 == 0 then return string.format("%d", t / 10) end
    return string.format("%d.%d", math.floor(t / 10), t % 10)
end

local function AddPreviewLines(tooltip, slot)
    local data = preview[KeyOf(GetLootSlotLink(slot)) or ""]
    if not data then return end
    for kind, idx, value in data:gmatch("(%a):(%d+):(%-?%d+)") do
        idx, value = tonumber(idx), tonumber(value)
        if kind == "s" and ArpgStatNames and ArpgStatNames[idx] then
            tooltip:AddLine("+" .. value .. " " .. ArpgStatNames[idx], 1, 1, 1)
        elseif kind == "e" and ArpgEffectTexts and ArpgEffectTexts[idx] then
            tooltip:AddLine((ArpgEffectTexts[idx]:gsub("{v}", Tenths(value))), 1, 1, 1)
        elseif kind == "u" then
            tooltip:AddLine("Unique powers revealed when picked up", 1, 0.5, 0)
        end
    end
    tooltip:Show()
end

hooksecurefunc(GameTooltip, "SetLootItem", AddPreviewLines)

local f = CreateFrame("Frame")
f:RegisterEvent("LOOT_OPENED")
f:SetScript("OnEvent", function()
    wipe(preview)
    SendChatMessage(".lootinfo data", "SAY")
end)

ChatFrame_AddMessageEventFilter("CHAT_MSG_SYSTEM", function(_, _, msg)
    if string.sub(msg, 1, 6) ~= "ARPGV|" then return false end
    local item, suffix, lines = msg:match("^ARPGV|(%d+)|(%-?%d+)|(.*)$")
    if item then preview[item .. ":" .. suffix] = lines end
    -- The answer can arrive after the mouse is already on a loot button: redraw that tooltip.
    local owner = GameTooltip:IsShown() and GameTooltip:GetOwner()
    if owner and owner.slot and owner:GetScript("OnEnter") then owner:GetScript("OnEnter")(owner) end
    return true
end)
