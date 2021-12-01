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
		noise1.amplitude(1);
	}

	void process(float k1, float k2) override {
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {
		noise1.update(&noiseOut);
		blockBuffer.pushBuffer(noiseOut.data, AUDIO_BLOCK_SAMPLES);
	}

	AudioStream& getStream() override {
		return noise1;
	}
	unsigned char getPort() override {
		return 0;
	}


private:

	TeensyBuffer buffer;
	audio_block_t noiseOut;

	AudioSynthNoiseWhite noise1;

};

REGISTER_PLUGIN(WhiteNoise);
