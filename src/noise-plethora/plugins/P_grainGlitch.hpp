#pragma once

#include "NoisePlethoraPlugin.hpp"
#define GRANULAR_MEMORY_SIZE 12800  // enough for 290 ms at 44.1 kHz

class grainGlitch : public NoisePlethoraPlugin {

public:

	grainGlitch()
	// : patchCord1(granular1, 0, waveformMod1, 0),
	//   patchCord2(granular1, 0, combine1, 0),
	//   patchCord3(waveformMod1, granular1),
	//   patchCord4(waveformMod1, 0, combine1, 1)
	{ }

	~grainGlitch() override {}

	grainGlitch(const grainGlitch&) = delete;
	grainGlitch& operator=(const grainGlitch&) = delete;

	void init() override {

		// combine1.AudioEffectDigitalCombine::XOR;


		waveformMod1.begin(1, 1000, WAVEFORM_SQUARE);

		// the Granular effect requires memory to operate
		granular1.begin(granularMemory, GRANULAR_MEMORY_SIZE);

		float msec = 150.0;
		granular1.beginFreeze(msec);

	}

	void process(float k1, float k2) override {
		float knob_1 = k1;
		float knob_2 = k2;

		float pitch1 = pow(knob_1, 2);

		waveformMod1.frequencyModulation(knob_2 * 2);

		float msec2 = 25.0 + (knob_2 * 75.0);

		granular1.beginPitchShift(msec2);
		waveformMod1.frequency(pitch1 * 5000 + 500);

		float ratio;

		ratio = powf(2.0, knob_2 * 6.0 - 3.0);
		granular1.setSpeed(ratio);
	}

	float processGraph(float sampleTime) override {
		if (buffer.empty()) {

			granular1.update(&waveformMod1Previous, &granular1Previous);

			waveformMod1.update(&granular1Previous, nullptr, &waveformMod1Previous);

			combine1.update(&granular1Previous, &waveformMod1Previous, &combine1Out);

			bufferAlt.pushBuffer(granular1Previous.data, AUDIO_BLOCK_SAMPLES);
			buffer.pushBuffer(combine1Out.data, AUDIO_BLOCK_SAMPLES);
		}

		altOutput = int16_to_float_5v(bufferAlt.shift()) / 5.f;;

		return int16_to_float_5v(buffer.shift()) / 5.f;;
	}

	AudioStream& getStream() override {
		return combine1;
	}
	unsigned char getPort() override {
		return 0;
	}

private:
	AudioEffectGranular      granular1;      //xy=885.75,359.9999985694885
	AudioSynthWaveformModulated waveformMod1;   //xy=889.75,480.74999871477485
	AudioEffectDigitalCombine combine1;       //xy=1111.7500114440918,295.74999809265137
	// AudioConnection          patchCord1;
	// AudioConnection          patchCord2;
	// AudioConnection          patchCord3;
	// AudioConnection          patchCord4;

	int16_t granularMemory[GRANULAR_MEMORY_SIZE];

	TeensyBuffer buffer, bufferAlt;

	audio_block_t granular1Previous;
	audio_block_t waveformMod1Previous;
	audio_block_t combine1Out;
};

REGISTER_PLUGIN(grainGlitch);
