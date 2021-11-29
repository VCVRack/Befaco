#pragma once

#include "NoisePlethoraPlugin.hpp"
#include <rack.hpp>

class WhiteNoise : public NoisePlethoraPlugin {

public:

	WhiteNoise()
	//: patchCord1(waveformMod1, 0, waveformMod2, 1),
	//patchCord3(waveformMod2, 0, waveformMod1, 0)
	{ }

	~WhiteNoise() override {}

	WhiteNoise(const WhiteNoise&) = delete;
	WhiteNoise& operator=(const WhiteNoise&) = delete;

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
	}

	void process(float k1, float k2) override {
	}

	float processGraph(float sampleTime) override {
		return noise.process();
	}

	AudioStream& getStream() override {
		return noise;
	}
	unsigned char getPort() override {
		return 0;
	}


private:
	AudioSynthNoiseWhiteFloat noise;

};

REGISTER_PLUGIN(WhiteNoise);
