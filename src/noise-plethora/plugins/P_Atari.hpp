#pragma once

#include "NoisePlethoraPlugin.hpp"

class Atari : public NoisePlethoraPlugin {

public:

	Atari()
	//:patchCord1(waveformMod1, 0, waveformMod2, 1),
	//patchCord3(waveformMod2, 0, waveformMod1, 0)
	{ }

	~Atari() override {}

	Atari(const Atari&) = delete;
	Atari& operator=(const Atari&) = delete;

	void init() override {
		int masterVolume = 1;
		waveformMod1.begin(WAVEFORM_SQUARE);
		waveformMod2.begin(WAVEFORM_PULSE);
		waveformMod1.offset(1);
		waveformMod1.amplitude(masterVolume);
		waveformMod2.amplitude(masterVolume);

	}

	void process(float k1, float k2) override {
		float knob_1 = k1;
		float knob_2 = k2;
		float pitch1 = pow(knob_1, 2);
		// float pitch2 = pow(knob_2, 2);
		waveformMod1.frequency(10 + (pitch1 * 50));
		waveformMod2.frequency(10 + (knob_2 * 200));
		waveformMod1.frequencyModulation(knob_2 * 8 + 3);
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {

		// waveformMod1 has FM from waveformMod2
		waveformMod1.update(&output2, nullptr, &output1);

		// waveformMod2 has PWM from waveformMod1
		waveformMod2.update(nullptr, &output1, &output2);

		blockBuffer.pushBuffer(output2.data, AUDIO_BLOCK_SAMPLES);
	}

	AudioStream& getStream() override {
		return waveformMod2;
	}
	unsigned char getPort() override {
		return 0;
	}

private:
	audio_block_t output1, output2;

	AudioSynthWaveformModulated waveformMod1;
	AudioSynthWaveformModulated waveformMod2;

	//AudioSynthWaveformModulated waveformMod1;   //xy=334,349
	//AudioSynthWaveformModulated waveformMod2; //xy=616,284
	//AudioConnection          patchCord1;
	//AudioConnection          patchCord3;

};

REGISTER_PLUGIN(Atari);
