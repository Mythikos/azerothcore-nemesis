-- UI.lua
-- Frame creation, layout, styling, scroll handling, tabs, and render logic.
-- All frames are created programmatically — no XML.

local NBB  = NemesisBountyBoard
local TD   = NemesisBountyBoard_TraitData

-- Dimensions --------------------------------------------------------------

local FRAME_W       = 520
local FRAME_H       = 510
local TITLE_H       = 36
local TAB_H         = 26
local UMBRAL_H      = 46
local FOOTER_H      = 28
local BORDER_PAD    = 14   -- inner padding from frame edges
local SCROLL_TOP    = TITLE_H + UMBRAL_H + TAB_H + 10
local SCROLL_CONTENT_W = FRAME_W - BORDER_PAD * 2 - 28

local ENTRY_LINE_H  = 15   -- height per text line within an entry
local ENTRY_GAP     = 10   -- vertical gap between entries

-- Leaderboard entries: 6 lines (name, stats, zone, traits, origin, last seen)
local LB_ENTRY_LINES = 6
local LB_ENTRY_H     = LB_ENTRY_LINES * ENTRY_LINE_H + ENTRY_GAP

-- Revenge entries: 5 lines (name, stats, zone, traits, deaths from this nemesis)
local RV_ENTRY_LINES = 5
local RV_ENTRY_H     = RV_ENTRY_LINES * ENTRY_LINE_H + ENTRY_GAP

-- Cached frames -----------------------------------------------------------

local mainFrame       = nil
local umbralBar       = nil
local umbralLine1     = nil
local umbralLine2     = nil
local footerText      = nil
local tabLeaderboard  = nil
local tabRevenge      = nil
local scrollFrame     = nil
local scrollChild     = nil
local entryPool       = {}    -- reused FontString groups

-- Helpers -----------------------------------------------------------------

local function ColorText(hex, text)
	return "|cFF" .. hex .. text .. "|r"
end

local function GetThreatColor(threat)
	if threat < 200 then return "00FF00"
	elseif threat < 400 then return "FFFF00"
	elseif threat < 600 then return "FF8800"
	else return "FF3333" end
end

-- FormatScheduleSummary: builds the human-readable schedule line.
local function FormatScheduleSummary(header)
	local times = NBB.ParseScheduleTimes(header.scheduleTimes)
	local timeStrs = {}
	for _, t in ipairs(times) do
		local h, ampm = t.hour, "AM"
		if h == 0 then
			h, ampm = 12, "AM"
		elseif h == 12 then
			ampm = "PM"
		elseif h > 12 then
			h, ampm = h - 12, "PM"
		end
		table.insert(timeStrs, string.format("%d:%02d %s", h, t.minute, ampm))
	end

	local durH = math.floor(header.durationMinutes / 60)
	local durM = header.durationMinutes % 60
	local durStr
	if durH > 0 and durM > 0 then
		durStr = durH .. "h " .. durM .. "m"
	elseif durH > 0 then
		durStr = durH .. "h"
	else
		durStr = durM .. "m"
	end

	local dayStr = header.scheduleDays:gsub(",", " & ")
	return "Schedule: " .. dayStr .. "  |  " .. table.concat(timeStrs, ", ") .. "  |  Duration: " .. durStr
end

-- Tab styling -------------------------------------------------------------

local TAB_ACTIVE_BG   = { 0.15, 0.15, 0.15, 1 }
local TAB_INACTIVE_BG = { 0.05, 0.05, 0.05, 0.6 }
local TAB_ACTIVE_TEXT  = "FFD100"
local TAB_INACTIVE_TEXT = "999999"

local function StyleTab(tab, active)
	if active then
		tab:SetBackdropColor(unpack(TAB_ACTIVE_BG))
		tab.text:SetTextColor(1, 0.82, 0, 1)
	else
		tab:SetBackdropColor(unpack(TAB_INACTIVE_BG))
		tab.text:SetTextColor(0.6, 0.6, 0.6, 1)
	end
end

local function CreateTab(parent, label, anchorPoint, anchorTo, anchorRel, xOff, yOff, width)
	local tab = CreateFrame("Button", nil, parent)
	tab:SetWidth(width)
	tab:SetHeight(TAB_H)
	tab:SetPoint(anchorPoint, anchorTo, anchorRel, xOff, yOff)
	tab:SetBackdrop({
		bgFile   = "Interface\\Buttons\\WHITE8X8",
		edgeFile = "Interface\\Buttons\\WHITE8X8",
		edgeSize = 1,
		insets   = { left = 0, right = 0, top = 0, bottom = 0 },
	})
	tab:SetBackdropBorderColor(0.3, 0.3, 0.3, 1)

	local text = tab:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
	text:SetPoint("CENTER", tab, "CENTER", 0, 0)
	text:SetText(label)
	tab.text = text

	return tab
end

-- Frame construction ------------------------------------------------------

local function CreateMainFrame()
	local f = CreateFrame("Frame", "NemesisBountyBoardFrame", UIParent)
	f:SetWidth(FRAME_W)
	f:SetHeight(FRAME_H)
	f:SetPoint("CENTER", UIParent, "CENTER", 0, 0)
	f:SetFrameStrata("DIALOG")
	f:SetMovable(true)
	f:EnableMouse(true)
	f:SetClampedToScreen(true)

	f:SetBackdrop({
		bgFile   = "Interface\\DialogFrame\\UI-DialogBox-Background",
		edgeFile = "Interface\\DialogFrame\\UI-DialogBox-Border",
		tile     = true,
		tileSize = 32,
		edgeSize = 32,
		insets   = { left = 11, right = 12, top = 12, bottom = 11 },
	})
	f:SetBackdropColor(0, 0, 0, 0.92)

	-- Register for ESC-to-close
	table.insert(UISpecialFrames, "NemesisBountyBoardFrame")

	-- Title bar (draggable region) ----------------------------------------
	local titleBar = CreateFrame("Frame", nil, f)
	titleBar:SetPoint("TOPLEFT",  f, "TOPLEFT",  0,   0)
	titleBar:SetPoint("TOPRIGHT", f, "TOPRIGHT", -37, 0)
	titleBar:SetHeight(TITLE_H)
	titleBar:EnableMouse(true)
	titleBar:SetScript("OnMouseDown", function() f:StartMoving() end)
	titleBar:SetScript("OnMouseUp",   function() f:StopMovingOrSizing() end)

	-- Title text
	local titleText = titleBar:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge")
	titleText:SetPoint("LEFT", titleBar, "LEFT", BORDER_PAD, 0)
	titleText:SetText(ColorText("FFD100", "NEMESIS BOUNTY BOARD"))

	-- Close button
	local closeBtn = CreateFrame("Button", "NemesisBountyBoardCloseBtn", f, "UIPanelCloseButton")
	closeBtn:SetPoint("TOPRIGHT", f, "TOPRIGHT", -5, -5)
	closeBtn:SetScript("OnClick", function() f:Hide() end)

	-- Divider under title
	local div1 = f:CreateTexture(nil, "ARTWORK")
	div1:SetTexture("Interface\\DialogFrame\\UI-DialogBox-Header")
	div1:SetHeight(2)
	div1:SetPoint("TOPLEFT",  f, "TOPLEFT",  BORDER_PAD,  -(TITLE_H))
	div1:SetPoint("TOPRIGHT", f, "TOPRIGHT", -BORDER_PAD, -(TITLE_H))
	div1:SetTexCoord(0, 1, 0.49, 0.51)
	div1:SetVertexColor(0.3, 0.3, 0.3, 1)

	-- Umbral Moon bar (always visible, above tabs) -----------------------
	umbralBar = CreateFrame("Frame", nil, f)
	umbralBar:SetPoint("TOPLEFT",  f, "TOPLEFT",  BORDER_PAD,      -(TITLE_H + 2))
	umbralBar:SetPoint("TOPRIGHT", f, "TOPRIGHT", -BORDER_PAD - 8, -(TITLE_H + 2))
	umbralBar:SetHeight(UMBRAL_H)

	umbralLine1 = umbralBar:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
	umbralLine1:SetPoint("TOPLEFT", umbralBar, "TOPLEFT", 0, -4)
	umbralLine1:SetPoint("RIGHT",   umbralBar, "RIGHT",   0,  0)
	umbralLine1:SetJustifyH("LEFT")
	umbralLine1:SetText("")

	umbralLine2 = umbralBar:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
	umbralLine2:SetPoint("TOPLEFT", umbralLine1, "BOTTOMLEFT", 0, -2)
	umbralLine2:SetPoint("RIGHT",   umbralBar,   "RIGHT",      0,  0)
	umbralLine2:SetJustifyH("LEFT")
	umbralLine2:SetText("")

	-- Divider under umbral bar
	local div2 = f:CreateTexture(nil, "ARTWORK")
	div2:SetHeight(2)
	div2:SetPoint("TOPLEFT",  f, "TOPLEFT",  BORDER_PAD,      -(TITLE_H + UMBRAL_H + 2))
	div2:SetPoint("TOPRIGHT", f, "TOPRIGHT", -BORDER_PAD - 8, -(TITLE_H + UMBRAL_H + 2))
	div2:SetTexture(0.27, 0.27, 0.27, 1)

	-- Tab buttons ---------------------------------------------------------
	local tabWidth = (FRAME_W - BORDER_PAD * 2) / 2

	tabLeaderboard = CreateTab(f, "Threat List", "TOPLEFT", f, "TOPLEFT", BORDER_PAD, -(TITLE_H + UMBRAL_H + 4), tabWidth)
	tabLeaderboard:SetScript("OnClick", function()
		NBB.activeTab = "leaderboard"
		StyleTab(tabLeaderboard, true)
		StyleTab(tabRevenge, false)
		NBB:RenderContent()
	end)

	tabRevenge = CreateTab(f, "Grudge List", "TOPLEFT", tabLeaderboard, "TOPRIGHT", 0, 0, tabWidth)
	tabRevenge:SetScript("OnClick", function()
		NBB.activeTab = "revenge"
		StyleTab(tabLeaderboard, false)
		StyleTab(tabRevenge, true)
		NBB:RenderContent()
	end)

	-- Scroll frame --------------------------------------------------------
	local sf = CreateFrame("ScrollFrame", "NemesisBountyBoardScrollFrame", f, "UIPanelScrollFrameTemplate")
	sf:SetPoint("TOPLEFT",     f, "TOPLEFT",     BORDER_PAD,            -SCROLL_TOP)
	sf:SetPoint("BOTTOMRIGHT", f, "BOTTOMRIGHT", -(BORDER_PAD + 28), FOOTER_H + 4)

	local sc = CreateFrame("Frame", "NemesisBountyBoardScrollChild", sf)
	sc:SetWidth(SCROLL_CONTENT_W)
	sc:SetHeight(1)
	sf:SetScrollChild(sc)

	scrollFrame = sf
	scrollChild = sc

	-- Footer bar ----------------------------------------------------------
	local footerBar = CreateFrame("Frame", nil, f)
	footerBar:SetPoint("BOTTOMLEFT",  f, "BOTTOMLEFT",  BORDER_PAD,  4)
	footerBar:SetPoint("BOTTOMRIGHT", f, "BOTTOMRIGHT", -BORDER_PAD, 4)
	footerBar:SetHeight(FOOTER_H)

	-- Divider above footer
	local div3 = f:CreateTexture(nil, "ARTWORK")
	div3:SetHeight(2)
	div3:SetPoint("BOTTOMLEFT",  f, "BOTTOMLEFT",  BORDER_PAD,  FOOTER_H + 4)
	div3:SetPoint("BOTTOMRIGHT", f, "BOTTOMRIGHT", -BORDER_PAD, FOOTER_H + 4)
	div3:SetTexture(0.27, 0.27, 0.27, 1)

	footerText = footerBar:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
	footerText:SetPoint("LEFT", footerBar, "LEFT", 0, 0)
	footerText:SetText("")

	f:Hide()
	return f
end

-- Entry rendering ---------------------------------------------------------

local CONTENT_PAD = 6   -- left padding inside scroll child

-- GetOrCreateEntry: returns (or lazily creates) a pool slot at index i with numLines lines.
local function GetOrCreateEntry(i, numLines)
	if entryPool[i] and entryPool[i].numLines >= numLines then
		return entryPool[i]
	end

	local slot = entryPool[i] or {}
	slot.numLines = numLines
	for j = 1, numLines do
		if not slot[j] then
			local fs = scrollChild:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
			fs:SetJustifyH("LEFT")
			fs:SetWordWrap(false)
			slot[j] = fs
		end
	end
	-- Level string on the right of line 1
	if not slot.levelStr then
		slot.levelStr = scrollChild:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
		slot.levelStr:SetJustifyH("RIGHT")
		slot.levelStr:SetWordWrap(false)
	end

	entryPool[i] = slot
	return slot
end

-- HidePooledEntries: hide all pool entries from index startIdx onward.
-- Hides ALL allocated lines per slot (up to LB_ENTRY_LINES, the max across tabs).
local function HidePooledEntries(startIdx)
	for i = startIdx, #entryPool do
		local slot = entryPool[i]
		if slot then
			for j = 1, LB_ENTRY_LINES do
				if slot[j] then slot[j]:Hide() end
			end
			if slot.levelStr then slot.levelStr:Hide() end
		end
	end
end

-- Render a leaderboard entry (6 lines).
local function RenderLeaderboardEntry(entry, slotIndex, yOffset)
	local slot = GetOrCreateEntry(slotIndex, LB_ENTRY_LINES)
	local td   = NemesisBountyBoard_TraitData
	local w    = SCROLL_CONTENT_W - CONTENT_PAD * 2

	local originDecoded = td.DecodeBitmask(entry.originTraits, td.OriginTraits)
	local earnedDecoded = td.DecodeBitmask(entry.earnedTraits, td.EarnedTraits)
	local originStr = td.FormatTraitList(originDecoded)
	local earnedStr = td.FormatTraitList(earnedDecoded)
	local ageStr    = NBB.FormatAge(entry.ageSeconds)
	local threatClr = GetThreatColor(entry.threat)

	-- Line 1: rank + name (left) | level (right)
	local l1 = slot[1]
	l1:ClearAllPoints()
	l1:SetPoint("TOPLEFT", scrollChild, "TOPLEFT", CONTENT_PAD, yOffset)
	l1:SetWidth(w - 50)
	l1:SetText(ColorText("FFFFFF", "#" .. entry.rank .. "  ") .. ColorText("FF6600", entry.name))
	l1:Show()

	local lv = slot.levelStr
	lv:ClearAllPoints()
	lv:SetPoint("TOPRIGHT", scrollChild, "TOPRIGHT", -CONTENT_PAD, yOffset)
	lv:SetWidth(50)
	lv:SetText(ColorText("FFFFFF", "Lv " .. entry.level))
	lv:Show()

	-- Line 2: threat / kills / age
	local l2 = slot[2]
	l2:ClearAllPoints()
	l2:SetPoint("TOPLEFT", l1, "BOTTOMLEFT", 0, -1)
	l2:SetWidth(w)
	l2:SetText("Threat: " .. ColorText(threatClr, tostring(entry.threat)) .. "  |  Kills: " .. ColorText("FFFFFF", tostring(entry.kills)) .. "  |  Age: " .. ColorText("AAAAAA", ageStr))
	l2:Show()

	-- Line 3: zone
	local l3 = slot[3]
	l3:ClearAllPoints()
	l3:SetPoint("TOPLEFT", l2, "BOTTOMLEFT", 0, -1)
	l3:SetWidth(w)
	l3:SetText("Zone: " .. ColorText("69CCF0", entry.zoneName))
	l3:Show()

	-- Line 4: traits
	local l4 = slot[4]
	l4:ClearAllPoints()
	l4:SetPoint("TOPLEFT", l3, "BOTTOMLEFT", 0, -1)
	l4:SetWidth(w)
	l4:SetText("Origin: " .. originStr .. "  |  Earned: " .. earnedStr)
	l4:Show()

	-- Line 5: origin player
	local playerStr = entry.originPlayerName ~= "" and entry.originPlayerName or "Unknown"
	local umbralSuffix = entry.umbralOrigin and ("  " .. ColorText("9D69DE", "(Umbral)")) or ""
	local l5 = slot[5]
	l5:ClearAllPoints()
	l5:SetPoint("TOPLEFT", l4, "BOTTOMLEFT", 0, -1)
	l5:SetWidth(w)
	l5:SetText("Born from the death of: " .. ColorText("FFFFFF", playerStr) .. umbralSuffix)
	l5:Show()

	-- Line 6: last known location
	local l6 = slot[6]
	l6:ClearAllPoints()
	l6:SetPoint("TOPLEFT", l5, "BOTTOMLEFT", 0, -1)
	l6:SetWidth(w)
	if entry.lastSeenAt == 0 then
		l6:SetText("Last seen: " .. ColorText("888888", "Unknown"))
	else
		local serverTime = NBB.umbral and NBB.umbral.serverTime or 0
		local elapsed    = serverTime - entry.lastSeenAt
		local agoStr     = elapsed > 0 and (NBB.FormatTimeUntil(elapsed) .. " ago") or "just now"
		local coords     = string.format("(%.0f, %.0f, %.0f)", entry.lastPosX, entry.lastPosY, entry.lastPosZ)
		l6:SetText("Last seen: " .. ColorText("69CCF0", entry.zoneName) .. " " .. ColorText("AAAAAA", coords) .. "  •  " .. ColorText("AAAAAA", agoStr))
	end
	l6:Show()
end

-- Render a revenge/grudge entry (5 lines).
local function RenderRevengeEntry(entry, slotIndex, yOffset)
	local slot = GetOrCreateEntry(slotIndex, RV_ENTRY_LINES)
	local td   = NemesisBountyBoard_TraitData
	local w    = SCROLL_CONTENT_W - CONTENT_PAD * 2

	local originDecoded = td.DecodeBitmask(entry.originTraits, td.OriginTraits)
	local earnedDecoded = td.DecodeBitmask(entry.earnedTraits, td.EarnedTraits)
	local originStr = td.FormatTraitList(originDecoded)
	local earnedStr = td.FormatTraitList(earnedDecoded)
	local ageStr    = NBB.FormatAge(entry.ageSeconds)
	local threatClr = GetThreatColor(entry.threat)

	-- Line 1: name (left) | level (right)
	local l1 = slot[1]
	l1:ClearAllPoints()
	l1:SetPoint("TOPLEFT", scrollChild, "TOPLEFT", CONTENT_PAD, yOffset)
	l1:SetWidth(w - 50)
	l1:SetText(ColorText("FF6600", entry.name))
	l1:Show()

	local lv = slot.levelStr
	lv:ClearAllPoints()
	lv:SetPoint("TOPRIGHT", scrollChild, "TOPRIGHT", -CONTENT_PAD, yOffset)
	lv:SetWidth(50)
	lv:SetText(ColorText("FFFFFF", "Lv " .. entry.level))
	lv:Show()

	-- Line 2: threat / kills / age / deaths to you
	local l2 = slot[2]
	l2:ClearAllPoints()
	l2:SetPoint("TOPLEFT", l1, "BOTTOMLEFT", 0, -1)
	l2:SetWidth(w)
	l2:SetText("Threat: " .. ColorText(threatClr, tostring(entry.threat)) .. "  |  Kills: " .. ColorText("FFFFFF", tostring(entry.kills)) .. "  |  Age: " .. ColorText("AAAAAA", ageStr) .. "  |  " .. ColorText("CC3333", "Killed you: " .. entry.deathsToYou .. "x"))
	l2:Show()

	-- Line 3: zone
	local l3 = slot[3]
	l3:ClearAllPoints()
	l3:SetPoint("TOPLEFT", l2, "BOTTOMLEFT", 0, -1)
	l3:SetWidth(w)
	l3:SetText("Zone: " .. ColorText("69CCF0", entry.zoneName))
	l3:Show()

	-- Line 4: traits
	local l4 = slot[4]
	l4:ClearAllPoints()
	l4:SetPoint("TOPLEFT", l3, "BOTTOMLEFT", 0, -1)
	l4:SetWidth(w)
	l4:SetText("Origin: " .. originStr .. "  |  Earned: " .. earnedStr)
	l4:Show()

	-- Line 5: this nemesis was born from YOUR death
	local l5 = slot[5]
	l5:ClearAllPoints()
	l5:SetPoint("TOPLEFT", l4, "BOTTOMLEFT", 0, -1)
	l5:SetWidth(w)
	l5:SetText(ColorText("CC3333", "This nemesis was born from your death."))
	l5:Show()

	-- Hide line 6 if it exists from a previous leaderboard render
	if slot[6] then slot[6]:Hide() end
end

-- Render ------------------------------------------------------------------

-- RenderContent: populates the scroll area based on the active tab.
function NemesisBountyBoard:RenderContent()
	if not scrollChild then return end

	-- Clear all pooled entries before rendering the active tab.
	HidePooledEntries(1)

	local yOffset = -4

	if self.activeTab == "leaderboard" then
		local entries = self.leaderboard.entries
		for i, entry in ipairs(entries) do
			RenderLeaderboardEntry(entry, i, yOffset)
			yOffset = yOffset - LB_ENTRY_H
		end
		HidePooledEntries(#entries + 1)

		-- Footer
		local header = self.leaderboard.header
		if header and footerText then
			footerText:SetText(ColorText("AAAAAA", "Living nemeses: ") .. ColorText("FFFFFF", tostring(header.livingTotal)))
		end

	elseif self.activeTab == "revenge" then
		local entries = self.revenge.entries
		local rHeader = self.revenge.header

		if #entries == 0 then
			-- Show empty state message
			local slot = GetOrCreateEntry(1, 1)
			local l1 = slot[1]
			l1:ClearAllPoints()
			l1:SetPoint("TOPLEFT", scrollChild, "TOPLEFT", CONTENT_PAD, yOffset)
			l1:SetWidth(SCROLL_CONTENT_W - CONTENT_PAD * 2)
			l1:SetText(ColorText("888888", "No nemeses have been born from your deaths... yet."))
			l1:Show()
			if slot.levelStr then slot.levelStr:Hide() end
			HidePooledEntries(2)
			yOffset = yOffset - ENTRY_LINE_H
		else
			for i, entry in ipairs(entries) do
				RenderRevengeEntry(entry, i, yOffset)
				yOffset = yOffset - RV_ENTRY_H
			end
			HidePooledEntries(#entries + 1)
		end

		-- Footer: vengeance count
		if rHeader and footerText then
			footerText:SetText(ColorText("AAAAAA", "Vengeance kills: ") .. ColorText("FFD700", tostring(rHeader.vengeanceCount)))
		end

	end

	-- Update scroll child height
	local totalH = math.abs(yOffset) + ENTRY_GAP
	if totalH < 1 then totalH = 1 end
	scrollChild:SetHeight(totalH)

	-- Reset scroll position to top
	if scrollFrame then
		scrollFrame:SetVerticalScroll(0)
	end
end

function NemesisBountyBoard:Render()
	-- Lazy-initialize the main frame on first render.
	if not mainFrame then
		mainFrame = CreateMainFrame()
	end

	-- Style tabs
	StyleTab(tabLeaderboard, self.activeTab == "leaderboard")
	StyleTab(tabRevenge,     self.activeTab == "revenge")

	-- Umbral Moon bar (always visible, reads from self.umbral)
	local umbral = self.umbral
	if umbral then
		local days  = NBB.ParseScheduleDays(umbral.scheduleDays)
		local times = NBB.ParseScheduleTimes(umbral.scheduleTimes)
		local active, remaining = NBB.IsUmbralMoonActive(
			umbral.serverTime, days, times, umbral.durationMinutes)

		if active then
			umbralLine1:SetText(ColorText("9D69DE", "Umbral Moon: ACTIVE") .. ColorText("AAAAAA", "  (" .. remaining .. "m remaining)"))
		else
			local secsUntil = NBB.GetNextUmbralMoon(
				umbral.serverTime, days, times, umbral.durationMinutes)
			if secsUntil then
				umbralLine1:SetText(ColorText("AAAAAA", "Next Umbral Moon: in ") .. ColorText("DDDDDD", NBB.FormatTimeUntil(secsUntil)))
			else
				umbralLine1:SetText(ColorText("AAAAAA", "Next Umbral Moon: Unknown"))
			end
		end
		umbralLine2:SetText(ColorText("888888", FormatScheduleSummary(umbral)))
	end

	-- Render the active tab's content
	self:RenderContent()

	-- Update revenge tab label with count badge
	local rHeader = self.revenge.header
	if rHeader and rHeader.grudgeCount > 0 then
		tabRevenge.text:SetText("Grudge List (" .. rHeader.grudgeCount .. ")")
	else
		tabRevenge.text:SetText("Grudge List")
	end

	mainFrame:Show()
end
