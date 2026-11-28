-- ArpgUI: rift timer (change m9-rifts). Inside a dungeon it asks the server ".rift data" every 5 s and shows
--   "ARPGR|key|best|pending|active|level|elapsed|par|deaths|affix"
-- as a small line at the top of the screen while a rift runs; the counter ticks locally between answers.
local frame = CreateFrame("Frame", "ArpgRiftFrame", UIParent)
frame:SetSize(360, 24)
frame:SetPoint("TOP", UIParent, "TOP", 0, -120)
frame:Hide()
local text = frame:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge")
text:SetPoint("CENTER")

local state = { active = false, level = 0, elapsed = 0, par = 0, deaths = 0, affix = "none", at = 0 }

local function Clock(s)
    s = math.max(0, math.floor(s))
    return string.format("%d:%02d", math.floor(s / 60), s % 60)
end

local function Draw()
    if not state.active then
        frame:Hide()
        return
    end
    local e = state.elapsed + (GetTime() - state.at)
    local color = (e + state.deaths * 10 <= state.par) and "|cff66ff66" or "|cffff6666"
    local affix = state.affix ~= "none" and ("  " .. state.affix) or ""
    text:SetText(string.format("Rift %d  %s%s|r / %s%s%s", state.level, color, Clock(e), Clock(state.par),
        state.deaths > 0 and ("  deaths " .. state.deaths) or "", affix))
    frame:Show()
end

local ask, tick = 0, 0
frame:SetScript("OnUpdate", function(_, dt)
    tick = tick + dt
    if tick >= 1 then
        tick = 0
        Draw()
    end
end)

local poller = CreateFrame("Frame")
poller:SetScript("OnUpdate", function(_, dt)
    ask = ask + dt
    if ask < 5 then return end
    ask = 0
    local inInstance, kind = IsInInstance()
    if inInstance and kind == "party" then
        SendChatMessage(".rift data", "SAY")
    elseif state.active then
        state.active = false
        Draw()
    end
end)

ChatFrame_AddMessageEventFilter("CHAT_MSG_SYSTEM", function(_, _, msg)
    if string.sub(msg, 1, 6) ~= "ARPGR|" then return false end
    local _, _, _, _, active, level, elapsed, par, deaths, affix = strsplit("|", msg)
    state.active = active == "1"
    state.level = tonumber(level) or 0
    state.elapsed = tonumber(elapsed) or 0
    state.par = tonumber(par) or 0
    state.deaths = tonumber(deaths) or 0
    state.affix = affix or "none"
    state.at = GetTime()
    Draw()
    return true   -- hide the data line from chat
end)
