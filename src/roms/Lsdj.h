#pragma once

#include <atomic>
#include <string>
#include <vector>

#include "liblsdj/error.h"
#include "liblsdj/project.h"
#include "liblsdj/sav.h"

#include "platform/Logger.h"

enum class LsdjSyncModes {
	Off,
	Slave,
	Master,
	Midi,
	Nanoloop,
	Keyboard,
	AnalogIn,
	AnalogOut,
	MidiMap,
	MidiOut,
	MidiArduinoboy
};

static int midiMapRowNumber(int channel, int noteNumber) {
	if (channel == 0) {
		return noteNumber;
	} else if (channel == 1) {
		return noteNumber + 128;
	}

	return -1;
}

static std::string syncModeToString(LsdjSyncModes syncMode) {
	switch (syncMode) {
	case LsdjSyncModes::Midi: return "midiSync";
	case LsdjSyncModes::MidiArduinoboy: return "midiSyncArduinoboy";
	case LsdjSyncModes::MidiMap: return "midiMap";
	}

	return "off";
}

static LsdjSyncModes syncModeFromString(const std::string& syncMode) {
	if (syncMode == "midiSync") return LsdjSyncModes::Midi;
	if (syncMode == "midiSyncArduinoboy") return LsdjSyncModes::MidiArduinoboy;
	if (syncMode == "midiMap") return LsdjSyncModes::MidiMap;
	return LsdjSyncModes::Off;
}

class Lsdj {
public:
	bool found = false;
	std::string version;
	bool arduinoboyPlaying = false;
	int tempoDivisor = 1;
	int lastRow = -1;

	std::atomic<LsdjSyncModes> syncMode = LsdjSyncModes::Off;
	std::atomic<bool> autoPlay = false;
	std::atomic<bool> keyboardShortcuts = true;

	std::vector<char> saveData;

	void importSongs(const std::vector<std::wstring>& paths);

	void removeSong(int idx);

	void loadSong(int idx);

	void exportSong(int idx, const std::string& target);

	void getSongNames(std::vector<std::string>& names);
};
