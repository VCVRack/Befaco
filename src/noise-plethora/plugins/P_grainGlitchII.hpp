#pragma once

#include "NoisePlethoraPlugin.hpp"
#define GRANULAR_MEMORY_SIZE 12800  // enough for 290 ms at 44.1 kHz

class grainGlitchII : public NoisePlethoraPlugin {

public:

	grainGlitchII()
	// : patchCord1(granular1, 0, waveformMod1, 0),
	//   patchCord2(granular1, amp1),
	//   patchCord3(waveformMod1, granular1)
	{ }

	~grainGlitchII() override {}

	grainGlitchII(const grainGlitchII&) = delete;
	grainGlitchII& operator=(const grainGlitchII&) = delete;

	void init() override {

		float msec = 150.0;
		waveformMod1.begin(1, 1000, WAVEFORM_SQUARE);
		amp1.gain(32000);
		// the Granular effect requires memory to operate
		granular1.begin(granularMemory, GRANULAR_MEMORY_SIZE);
		granular1.beginFreeze(msec);
	}

	void process(float k1, float k2) override {
		float knob_1 = k1;
		float knob_2 = k2;

		float pitch1 = pow(knob_1, 2);
		float msec2 = 25.0 + (knob_2 * 75.0);
		float ratio;

		waveformMod1.frequencyModulation(knob_2 * 2);
		granular1.beginPitchShift(msec2);

		waveformMod1.frequency(pitch1 * 5000 + 500);
		ratio = powf(2.0, knob_2 * 6.0 - 3.0); // 0.125 to 8.0 -- uncomment for far too much range!
		granular1.setSpeed(ratio);

	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {

		granular1.update(&waveformMod1Out, &granularOut);

		waveformMod1.update(&granularOut, nullptr, &waveformMod1Out);

		amp1.update(&granularOut);

		blockBuffer.pushBuffer(granularOut.data, AUDIO_BLOCK_SAMPLES);
	}

	AudioStream& getStream() override {
		return amp1;
	}
	unsigned char getPort() override {
		return 0;
	}

private:
	AudioEffectGranular      granular1;      //xy=369,297
	AudioSynthWaveformModulated waveformMod1;   //xy=376,406
	AudioAmplifier           amp1;           //xy=602,359
	// AudioConnection          patchCord1;
	// AudioConnection          patchCord2;
	// AudioConnection          patchCord3;
	int16_t granularMemory[GRANULAR_MEMORY_SIZE];

	audio_block_t granularOut, waveformMod1Out;
};

REGISTER_PLUGIN(grainGlitchII);
