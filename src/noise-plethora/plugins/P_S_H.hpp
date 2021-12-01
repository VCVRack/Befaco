#pragma once

#include "NoisePlethoraPlugin.hpp"

class S_H : public NoisePlethoraPlugin {

public:

	S_H()
	//: patchCord1(waveformMod1, 0, mixer1, 0)
	//, patchCord2(waveformMod1, freeverb1)
	//, patchCord3(freeverb1, 0, mixer1, 1)
	{ }

	~S_H() override {}

	S_H(const S_H&) = delete;
	S_H& operator=(const S_H&) = delete;

	void init() override {
		waveformMod1.begin(1, 200, WAVEFORM_SAMPLE_HOLD);
		waveformMod1.frequencyModulation(10);

		freeverb1.damping(1);
		freeverb1.roomsize(0.5);

		mixer1.gain(0, 1);
		mixer1.gain(1, 0);
	}

	void process(float k1, float k2) override {
		waveformMod1.frequency(5000 * k1 + 15);
		mixer1.gain(0, 1 - k2);
		mixer1.gain(1, k2 * 4);
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {
		waveformMod1.update(nullptr, nullptr, &waveformModBlock);
		freeverb1.update(&waveformModBlock, &freeverbBlock);
		mixer1.update(&waveformModBlock, &freeverbBlock, nullptr, nullptr, &mixerBlock);

		blockBuffer.pushBuffer(mixerBlock.data, AUDIO_BLOCK_SAMPLES);
	}

	AudioStream& getStream() override {
		return mixer1;
	}
	unsigned char getPort() override {
		return 0;
	}

private:

	audio_block_t waveformModBlock, mixerBlock, freeverbBlock;

	AudioSynthWaveformModulated waveformMod1;   //xy=517.5666656494141,413.99998474121094
	AudioEffectFreeverb      freeverb1;      //xy=711.5666656494141,465.99998474121094
	AudioMixer4              mixer1;         //xy=868.5666656494141,399.99998474121094
	//AudioConnection          patchCord1(waveformMod1, 0, mixer1, 0);
	//AudioConnection          patchCord2(waveformMod1, freeverb1);
	//AudioConnection          patchCord3(freeverb1, 0, mixer1, 1);

};

REGISTER_PLUGIN(S_H);
