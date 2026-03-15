-- Core.lua
-- Event registration, message interception, data parsing, and schedule helpers.

NemesisBountyBoard = {}

-- State -------------------------------------------------------------------

NemesisBountyBoard.umbral = nil

NemesisBountyBoard.leaderboard = {
	header  = nil,
	entries = {},
}

NemesisBountyBoard.revenge = {
	header  = nil,
	entries = {},
}

-- Tracks which tab is active: "leaderboard" or "revenge"
NemesisBountyBoard.activeTab = "leaderboard"

-- Set to true once both LF and RF have arrived for a single data push.
NemesisBountyBoard.leaderboardReady = false
NemesisBountyBoard.revengeReady     = false

-- Message filter ----------------------------------------------------------

-- Intercept CHAT_MSG_SYSTEM messages that begin with "@@NBB@@".
-- Returning true suppresses the message from the chat frame.
ChatFrame_AddMessageEventFilter("CHAT_MSG_SYSTEM", function(self, event, msg, ...)
	if msg:sub(1, 7) == "@@NBB@@" then
		NemesisBountyBoard:ProcessMessage(msg)
		return true
	end
	return false
end)

-- Message parsing ---------------------------------------------------------

function NemesisBountyBoard:ProcessMessage(msg)
	-- Strip the 7-character prefix "@@NBB@@"
	local body = msg:sub(8)
	if body == "" then return end

	-- Two-character message type
	local msgType = body:sub(1, 2)
	local data    = body:sub(3) -- everything after the type (starts with ~)

	if msgType == "UH" then
		-- Umbral Header: UH~version~serverTime~scheduleDays~scheduleTimes~durationMinutes
		local fields = { strsplit("~", data) }
		self.umbral = {
			version         = tonumber(fields[2]) or 0,
			serverTime      = tonumber(fields[3]) or 0,
			scheduleDays    = fields[4] or "",
			scheduleTimes   = fields[5] or "",
			durationMinutes = tonumber(fields[6]) or 0,
		}

	elseif msgType == "LH" then
		-- Leaderboard Header: LH~eliteCount~livingTotal
		local fields = { strsplit("~", data) }
		self.leaderboard.header = {
			eliteCount  = tonumber(fields[2]) or 0,
			livingTotal = tonumber(fields[3]) or 0,
		}
		self.leaderboard.entries = {}
		self.leaderboardReady = false

	elseif msgType == "LE" then
		-- Leaderboard Entry: LE~rank~name~level~threat~kills~originTraits~earnedTraits~zoneName~ageSeconds~umbralOrigin~originPlayerName~lastMapId~lastPosX~lastPosY~lastPosZ~lastSeenAt
		local fields = { strsplit("~", data) }
		local entry = {
			rank             = tonumber(fields[2])  or 0,
			name             = fields[3]            or "Unknown",
			level            = tonumber(fields[4])  or 0,
			threat           = tonumber(fields[5])  or 0,
			kills            = tonumber(fields[6])  or 0,
			originTraits     = tonumber(fields[7])  or 0,
			earnedTraits     = tonumber(fields[8])  or 0,
			zoneName         = fields[9]            or "Unknown",
			ageSeconds       = tonumber(fields[10]) or 0,
			umbralOrigin     = (fields[11] == "1"),
			originPlayerName = fields[12]           or "",
			lastMapId        = tonumber(fields[13]) or 0,
			lastPosX         = tonumber(fields[14]) or 0,
			lastPosY         = tonumber(fields[15]) or 0,
			lastPosZ         = tonumber(fields[16]) or 0,
			lastSeenAt       = tonumber(fields[17]) or 0,
		}
		table.insert(self.leaderboard.entries, entry)

	elseif msgType == "LF" then
		-- Leaderboard Footer: all leaderboard entries received.
		self.leaderboardReady = true
		self:TryRender()

	elseif msgType == "RH" then
		-- Revenge Header: RH~vengeanceCount~grudgeCount
		local fields = { strsplit("~", data) }
		self.revenge.header = {
			vengeanceCount = tonumber(fields[2]) or 0,
			grudgeCount    = tonumber(fields[3]) or 0,
		}
		self.revenge.entries = {}
		self.revengeReady = false

	elseif msgType == "RE" then
		-- Revenge Entry: RE~eliteId~name~level~threat~kills~originTraits~earnedTraits~zoneName~mapId~posX~posY~posZ~age~deathsToYou
		local fields = { strsplit("~", data) }
		local entry = {
			eliteId      = tonumber(fields[2])  or 0,
			name         = fields[3]            or "Unknown",
			level        = tonumber(fields[4])  or 0,
			threat       = tonumber(fields[5])  or 0,
			kills        = tonumber(fields[6])  or 0,
			originTraits = tonumber(fields[7])  or 0,
			earnedTraits = tonumber(fields[8])  or 0,
			zoneName     = fields[9]            or "Unknown",
			mapId        = tonumber(fields[10]) or 0,
			posX         = tonumber(fields[11]) or 0,
			posY         = tonumber(fields[12]) or 0,
			posZ         = tonumber(fields[13]) or 0,
			ageSeconds   = tonumber(fields[14]) or 0,
			deathsToYou  = tonumber(fields[15]) or 0,
		}
		table.insert(self.revenge.entries, entry)

	elseif msgType == "RF" then
		-- Revenge Footer: all revenge entries received.
		self.revengeReady = true
		self:TryRender()
	end
end

-- TryRender: only render once both sections have finished arriving.
function NemesisBountyBoard:TryRender()
	if self.leaderboardReady and self.revengeReady then
		self:Render()
	end
end

-- Schedule helpers --------------------------------------------------------

-- Day abbreviation → 0-based index (0=Sun, 1=Mon, ..., 6=Sat)
local DAY_NAMES = {
	Sun = 0, Mon = 1, Tue = 2, Wed = 3, Thu = 4, Fri = 5, Sat = 6,
}

-- ParseScheduleDays: "Wed,Sat" → {3, 6}
function NemesisBountyBoard.ParseScheduleDays(str)
	local days = {}
	for abbr in str:gmatch("[^,]+") do
		local idx = DAY_NAMES[abbr:sub(1, 3)]
		if idx then
			table.insert(days, idx)
		end
	end
	return days
end

-- ParseScheduleTimes: "08:00,20:00" → {{hour=8, minute=0}, {hour=20, minute=0}}
function NemesisBountyBoard.ParseScheduleTimes(str)
	local times = {}
	for token in str:gmatch("[^,]+") do
		local h, m = token:match("^(%d+):(%d+)$")
		if h then
			table.insert(times, { hour = tonumber(h), minute = tonumber(m) })
		end
	end
	return times
end

-- IsUmbralMoonActive: returns isActive (bool), minutesRemaining (number).
-- serverTime is a Unix timestamp. days is from ParseScheduleDays, times from ParseScheduleTimes.
function NemesisBountyBoard.IsUmbralMoonActive(serverTime, days, times, durationMin)
	local d    = date("*t", serverTime)
	-- Lua wday: 1=Sunday → convert to 0-based
	local wday = d.wday - 1
	local nowMin = d.hour * 60 + d.min

	for _, day in ipairs(days) do
		if day == wday then
			for _, t in ipairs(times) do
				local startMin = t.hour * 60 + t.minute
				local endMin   = startMin + durationMin
				if nowMin >= startMin and nowMin < endMin then
					return true, endMin - nowMin
				end
			end
		end
	end
	return false, 0
end

-- GetNextUmbralMoon: returns seconds until next window start, or nil if not found within 7 days.
function NemesisBountyBoard.GetNextUmbralMoon(serverTime, days, times, durationMin)
	local d    = date("*t", serverTime)
	local wday = d.wday - 1
	local nowMin = d.hour * 60 + d.min

	-- Build a sorted flat list of (dayOffset * 1440 + startMin) for the next 7 days.
	-- dayOffset 0 = today, 1 = tomorrow, ...
	local candidates = {}
	for offset = 0, 6 do
		local checkDay = (wday + offset) % 7
		for _, day in ipairs(days) do
			if day == checkDay then
				for _, t in ipairs(times) do
					local startMin = t.hour * 60 + t.minute
					local totalMin = offset * 1440 + startMin
					-- Skip windows that are already over or currently active
					if offset > 0 or startMin > nowMin then
						table.insert(candidates, totalMin)
					end
				end
			end
		end
	end

	if #candidates == 0 then return nil end

	table.sort(candidates)
	local soonestMin = candidates[1]
	local nowMinAbsolute = d.hour * 60 + d.min
	return (soonestMin - nowMinAbsolute) * 60  -- return seconds
end

-- FormatTimeUntil: seconds → "2d 6h", "3h 15m", or "12m"
function NemesisBountyBoard.FormatTimeUntil(seconds)
	local days  = math.floor(seconds / 86400)
	local hours = math.floor((seconds % 86400) / 3600)
	local mins  = math.floor((seconds % 3600) / 60)
	if days > 0 then
		return days .. "d " .. hours .. "h"
	elseif hours > 0 then
		return hours .. "h " .. mins .. "m"
	else
		return mins .. "m"
	end
end

-- FormatAge: identical formatting, used for nemesis age display.
function NemesisBountyBoard.FormatAge(seconds)
	return NemesisBountyBoard.FormatTimeUntil(seconds)
end

-- Stub: Render is defined in UI.lua.
-- Declared here so Core.lua can call self:Render() without load-order issues.
function NemesisBountyBoard:Render()
	-- Implemented in UI.lua
end
