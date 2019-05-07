#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "plugs/RetroPlug.h"

class RetroPlugInstrument : public IPlug {
public:
	RetroPlugInstrument(IPlugInstanceInfo instanceInfo);
	~RetroPlugInstrument();

#if IPLUG_DSP
public:
	void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
	void ProcessMidiMsg(const IMidiMsg& msg) override;
	void OnReset() override;
	void OnParamChange(int paramIdx) override;
	void OnIdle() override;

	bool SerializeState(IByteChunk& chunk) const override;
	int UnserializeState(const IByteChunk& chunk, int startPos) override;
private:
	void generateMidiClock(int frameCount);

	void processSync(int tempoDivisor, char value);

	double _lastPpq24 = 0;
	int _clockRemain = 0;

	RetroPlug _plug;

	float* _sampleScratch;
#endif
};