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

	float processGraph(float sampleTime) override {


		if (w1.empty() || w2.empty()) {

			// waveformMod1
			waveformMod1.update(nullptr, &output2Prev, &output1);
			w1.pushBuffer(output1.data, AUDIO_BLOCK_SAMPLES);

			// waveformMod2 has FM from waveformMod1
			waveformMod2.update(&output1, nullptr, &output2);
			w2.pushBuffer(output2.data, AUDIO_BLOCK_SAMPLES);

			memcpy(output1Prev.data, output1.data, AUDIO_BLOCK_SAMPLES);
			memcpy(output2Prev.data, output2.data, AUDIO_BLOCK_SAMPLES);
		}


		float waveformMod1Out = int16_to_float_5v(w1.shift()) / 5.f ;
		float waveformMod2Out = int16_to_float_5v(w2.shift()) / 5.f;

		//w1Prev = output1;
		//w2Prev = output2;

		altOutput = waveformMod1Out;

		return waveformMod2Out;
	}

	AudioStream& getStream() override {
		return waveformMod2;
	}
	unsigned char getPort() override {
		return 0;
	}

private:
	audio_block_t output1 = {};
	audio_block_t output2 = {};
	audio_block_t output1Prev = {};
	audio_block_t output2Prev = {};

	TeensyBuffer w1, w2;
	AudioSynthWaveformModulated waveformMod1;
	AudioSynthWaveformModulated waveformMod2;

	//AudioSynthWaveformModulated waveformMod1;   //xy=334,349
	//AudioSynthWaveformModulated waveformMod2; //xy=616,284
	//AudioConnection          patchCord1;
	//AudioConnection          patchCord3;

};

REGISTER_PLUGIN(Atari);
