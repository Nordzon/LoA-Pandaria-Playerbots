-- ArpgUI: farther camera zoom (owner 2026-11-28 "ideally 2x the current distance"). The stock client stops at
-- cameraDistanceMax x cameraDistanceMaxFactor (the Interface slider tops out at factor 2); we raise both at every login.
-- /arpgcam <factor> sets another factor (1 = stock slider minimum, 4 = default here), /arpgcam shows the current values.
local MAX_DISTANCE, DEFAULT_FACTOR = 50, 4

local function Apply(factor)
    SetCVar("cameraDistanceMax", MAX_DISTANCE)
    SetCVar("cameraDistanceMaxFactor", factor)
end

local frame = CreateFrame("Frame")
frame:RegisterEvent("PLAYER_LOGIN")
frame:SetScript("OnEvent", function()
    ArpgUIDB = ArpgUIDB or {}
    Apply(ArpgUIDB.cameraFactor or DEFAULT_FACTOR)
end)

SLASH_ARPGCAM1 = "/arpgcam"
SlashCmdList["ARPGCAM"] = function(msg)
    local f = tonumber(msg)
    if f and f > 0 then
        ArpgUIDB = ArpgUIDB or {}
        ArpgUIDB.cameraFactor = f
        Apply(f)
    end
    DEFAULT_CHAT_FRAME:AddMessage(string.format("ArpgUI camera: max %s, factor %s",
        GetCVar("cameraDistanceMax"), GetCVar("cameraDistanceMaxFactor")))
end
