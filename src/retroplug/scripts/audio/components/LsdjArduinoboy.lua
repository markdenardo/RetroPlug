local midi = require("midi")
local enumtil = require("util.enum")

local LsdjSyncModes = {
	Off = 0,
	MidiSync = 1,
	MidiSyncArduinoboy = 2,
	MidiMap = 3
}

local function midiMapRowNumber(channel, note)
	if channel == 0 then return note end
	if channel == 1 then return note + 128 end
	return -1;
end

local LsdjArduinoboy = component({
	name = "Arduinoboy",
	romName = "LSDj*",
	systemState = {
		syncMode = LsdjSyncModes.Off,
		autoPlay = false,
		playing = false,
		lastRow = -1,
		tempoDivisor = 1
	}
})

local function isLsdj(system)
	return system.desc.romName:match("LSDj*") ~= nil
end

function LsdjArduinoboy.requires()
	return System ~= nil and isLsdj(System)
end

function LsdjArduinoboy.onMenu(menu)
	local state = System.state.arduinoboy
	return menu
		:subMenu("LSDj")
			:subMenu("Sync")
				:multiSelect({
					"Off",
					"MIDI Sync [MIDI]",
					"MIDI Sync (Arduinoboy) [MIDI]",
					"MIDI Map [MI. MAP]"
				}, state.syncMode, function(idx)
					state.syncMode = idx
				end)
				:separator()
				:select("Autoplay", state.autoPlay, function(value) state.autoPlay = value end)
end

function LsdjArduinoboy.onTransportChanged(running)
	for _, system in ipairs(Project.systems) do
		local state = system.state.arduinoboy

		if state ~= nil then
			if state.syncMode == LsdjSyncModes.MidiMap then
				state.playing = running
			end

			if state.autoPlay == true then
				system:buttons():press(Button.Start)
			end

			if running == false and state.lastRow ~= -1 then
				system:sendSerialByte(0, 0xFE)
			end
		end
	end
end

function LsdjArduinoboy.onUpdate(frameCount)
	--print("---")
end

local function processSync(system, offset)
	local state = system.state.arduinoboy

	if state.syncMode == LsdjSyncModes.MidiSync then
		system:sendSerialByte(offset, 0xF8)
	elseif state.syncMode == LsdjSyncModes.MidiSyncArduinoboy then
		if state.playing == true then
			system:sendSerialByte(offset, 0xF8)
		end
	elseif state.syncMode == LsdjSyncModes.MidiMap then
		system:sendSerialByte(offset, 0xFF)
	end
end

function LsdjArduinoboy.onPpq(ppq, offset)
	for _, system in ipairs(Project.systems) do
		if system.state.arduinoboy ~= nil then
			processSync(system, offset)
		end
	end
end

function LsdjArduinoboy.onMidi(system, msg)
	local state = system.state.arduinoboy

	local status = msg.status
	if status == midi.Status.System then
		if msg.systemStatus == midi.SystemStatus.TimingClock then
			processSync(system, msg.offset)
		else
			log.info(enumtil.toEnumString(midi.SystemStatus, msg.systemStatus))
		end
	end

	if state.syncMode == LsdjSyncModes.MidiSyncArduinoboy then
		if status == midi.Status.NoteOn then
			if 	   msg.note == 24 then state.playing = true
			elseif msg.note == 25 then state.playing = false
			elseif msg.note == 26 then state.tempoDivisor = 1
			elseif msg.note == 27 then state.tempoDivisor = 2
			elseif msg.note == 28 then state.tempoDivisor = 4
			elseif msg.note == 29 then state.tempoDivisor = 8
			elseif msg.note >= 30 then
				system:sendSerialByte(msg.offset, msg.note - 30)
			end
		end
	elseif state.syncMode == LsdjSyncModes.MidiMap then
		-- Notes trigger row numbers

		if status == midi.Status.NoteOn then
			log.info("noteon")
			local rowIdx = midiMapRowNumber(msg.channel, msg.note)

			if rowIdx ~= -1 then
				system:sendSerialByte(msg.offset, rowIdx)
				state.lastRow = rowIdx
			end
		elseif status == midi.Status.NoteOff then
			local rowIdx = midiMapRowNumber(msg.channel, msg.note)

			if rowIdx == state.lastRow then
				system:sendSerialByte(msg.offset, 0xFE)
				state.lastRow = -1
			end
		elseif status == midi.Status.System then
			if msg.systemStatus == midi.SystemStatus.SequenceStop then
				system:sendSerialByte(msg.offset, 0xFE)
			end
		end
	end
end

return LsdjArduinoboy
