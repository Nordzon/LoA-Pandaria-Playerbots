-- ARPG potions: the item tooltip's "Use:" line tells the real effect (% of max health/mana, shared cooldown).
-- Texts come from PotionData.lua (generated from world.arpg_potion by build-client-patch.py).
local function FixPotionLine(tooltip)
    if not ArpgPotionText then return end
    local _, link = tooltip:GetItem()
    if not link then return end
    local id = tonumber(link:match("item:(%d+)"))
    local text = id and ArpgPotionText[id]
    if not text then return end
    local name = tooltip:GetName()
    for i = 2, tooltip:NumLines() do
        local line = _G[name .. "TextLeft" .. i]
        local t = line and line:GetText()
        if t and t:find("^Use:") then
            line:SetText(text)
            tooltip:Show()   -- re-layout for the new width
            return
        end
    end
end

for _, tip in ipairs({ GameTooltip, ItemRefTooltip, ShoppingTooltip1, ShoppingTooltip2 }) do
    if tip then tip:HookScript("OnTooltipSetItem", FixPotionLine) end
end
