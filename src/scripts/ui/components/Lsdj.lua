local dialog = require("dialog")
local KeyboardActions = require("components.lsdj.actions")
local lsdj = require("liblsdj.liblsdj")

local ROM_FILTER = { "LSDj Rom Files", "*.gb" }
local SAV_FILTER = { "LSDj Sav Files", "*.sav" }
local SONG_FILTER = { "LSDj Song Files", "*.lsdsng" }
local KIT_FILTER = { "LSDj Kit Files", "*.kit" }

local function formatName(idx, name, empty)
	if empty == true then name = "(Empty)" end
	return string.format("%02X", idx) .. ": " .. name
end

local Lsdj = component({ name = "LSDj", romName = "LSDj*" })
function Lsdj:init()
	self._valid = false
	self._kits = {}
	self._songs = {}
	self._littleFm = false
	self._overclock = false

	self._keyboardActions = KeyboardActions(self:system())
	self:registerActions(self._keyboardActions)
end

function Lsdj:onBeforeButton(button, down)
	self._keyboardActions:_handleButtonPress(button, down)
end

function Lsdj:onPatchRom(romData)

end

function Lsdj:onBeforeRomLoad(romData)
	-- Patch the rom
	--liblsdj.parseRom(romData)
end

function Lsdj:onBeforeSavLoad(savData)
	-- Patch the sav
	--liblsdj.parseSav(savData)
end

function Lsdj:onRomLoad(romData)

end

local function overclockPatch(data, overclock)
	local overClockIdx = data:findSequence({ 0x3e }, 1, { 0xe0, 0x07 })
	if overClockIdx ~= -1 then
		local v = 0x04
		if overclock == true then v = 0x07 end
		data:write(overClockIdx + 1, { v })
	end
end

function Lsdj:updateRom()
	local d = self:system().sourceRomData
	overclockPatch(d, self._overclock)
end

function Lsdj:onMenu(menu)
	local root = menu:subMenu("LSDj")
	local system = self:system()
	local rom = lsdj.loadRom(system:rom())
	local sav = lsdj.loadSav(system:sram())

	self:createSongsMenu(root:subMenu("Songs"), sav)
	self:createKitsMenu(root:subMenu("Kits"), rom)

	root:subMenu("Fonts")
			:parent()
		:subMenu("Palettes")
			:parent()
		:separator()
		:subMenu("Patches")
			:select("LitteFM", self._littleFm, function(v) self._littleFm = v; self:updateRom() end)
			:select("4x Overclock", self._overclock, function(v) self._overclock = v; self:updateRom() end)
			:parent()
		:action("Export ROM...", function() end)
end

function Lsdj:createKitsMenu(menu, rom)
	local system = self:system()

	menu:action("Import (and reset)...", function()
		dialog.loadFile({ KIT_FILTER, ROM_FILTER }, function(paths)
			local err = rom:importKits(paths)
			if err == nil then
				system:setRom(rom:toBuffer(), true)
			else
				-- Log error
			end
		end)
	end)
	:action("Export All...", function()
		dialog.saveDirectory(function(path)
			rom:exportKits(path)
		end)
	end)
	:separator()

	local kits = rom:getKits()
	for i, kit in ipairs(kits) do
		local kitName = formatName(i - 1, kit.name, kit.data == nil)
		local kitMenu = menu:subMenu(kitName)

		if kit.data ~= nil then
			kitMenu:action("Replace...", function() end)
				:action("Export...", function() end)
				:action("Delete", function() end)
		else
			kitMenu:action("Load (and reset)...", function() end)
		end
	end
end

function Lsdj:createSongsMenu(menu, sav)
	local system = self:system()
	local desc = system:desc()

	menu:action("Import (and reset)...", function()
		dialog.loadFile({ SONG_FILTER, SAV_FILTER }, function(paths)
			local err = sav:importSongs(paths)
			if err == nil then
				sav:toBuffer(system.sram)
				system:setSram(system.sram, true)
			else
				print("Import failed:")
				table.foreach(err, print)
			end
		end)
	end)
	:action("Export All...", function()
		dialog.saveDirectory(function(path)
			local err = sav:exportSongs(path)
			if err ~= nil then
				print("Export failed:")
				table.foreach(err, print)
			end
		end)
	end)
	:separator()

	local songs = sav:getSongs()
	for _, song in ipairs(songs) do
		local songName = formatName(song.idx, song.name, song.empty)
		local songMenu = menu:subMenu(songName)

		songMenu:action("Load (and reset)", function()
			sav:loadSong(song.idx)
			system:setSram(sav:toBuffer(), true)
		end)

		if song.empty == false then
			songMenu:action("Export .lsdsng...", function()
				dialog.saveFile({ SONG_FILTER }, function(path)
					sav:songToFile(song.idx, path)
				end)
			end)
			:action("Delete", function()
				sav:deleteSong(song.idx)
				system:setSram(desc.sourceSavData, true)
			end)
		end
	end
end

return Lsdj
