#pragma once

#include <rack.hpp>

#include "NoisePlethoraPlugin.hpp"


class TestPlugin : public NoisePlethoraPlugin {

public:

	TestPlugin()
	//: patchCord1(waveformMod1, 0, waveformMod2, 1),
	//patchCord3(waveformMod2, 0, waveformMod1, 0)
	{ }

	~TestPlugin() override {}

	TestPlugin(const TestPlugin&) = delete;
	TestPlugin& operator=(const TestPlugin&) = delete;

	void init() override {
		//test.begin(WaveformType::WAVEFORM_SQUARE);
		//test.offset(1);

		/*
		int masterVolume= 1;
		waveformMod1.begin(WAVEFORM_SQUARE);
		waveformMod2.begin(WAVEFORM_PULSE);
		waveformMod1.offset(1);
		waveformMod1.amplitude(masterVolume);
		waveformMod2.amplitude(masterVolume);
		*/

		waveform1.amplitude(1.0);
	}

	void process(float k1, float k2) override {
		float knob_1 = k1;
		float knob_2 = k2;
		float pitch1 = std::pow(knob_1, 2);

		waveform1.frequency((pitch1 * 10000));
		if (knob_2 > 0.5) {
			waveform1.begin((knob_2 > 0.75) ? WAVEFORM_SQUARE : WAVEFORM_SINE);
		}
		else {
			waveform1.begin((knob_2 > 0.25) ? WAVEFORM_TRIANGLE : WAVEFORM_SAWTOOTH);
		}

		//waveformMod1.frequency(10+(pitch1*50));
		//waveformMod2.frequency(10+(knob_2*200));
		//waveformMod1.frequencyModulation(knob_2*8+3);
		//DEBUG(string::f("%g %d %g %g", waveform1.frequency, waveform1.tone_type, k1, k2).c_str());
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {

		// waveformMod1
		waveform1.update(nullptr, nullptr, &waveformOut);
		blockBuffer.pushBuffer(waveformOut.data, AUDIO_BLOCK_SAMPLES);
	}

	AudioStream& getStream() override {
		return waveform1;
	}
	unsigned char getPort() override {
		return 0;
	}

private:

	audio_block_t waveformOut;

	AudioSynthWaveformModulated waveform1;

	//AudioSynthWaveformModulated waveformMod1;   //xy=334,349
	//AudioSynthWaveformModulated waveformMod2; //xy=616,284
	//AudioConnection          patchCord1;
	//AudioConnection          patchCord3;

};


REGISTER_PLUGIN(TestPlugin);