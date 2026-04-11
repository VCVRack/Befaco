#pragma once

#include "NoisePlethoraPlugin.hpp"

#define FLANGE_NOISE_DELAY_LENGTH (8 * AUDIO_BLOCK_SAMPLES)

class FlangeNoise : public NoisePlethoraPlugin {

public:

	FlangeNoise()
	// : patchCord1(pink1, flange1)
	{ }

	~FlangeNoise() override {}

	// delete copy constructors
	FlangeNoise(const FlangeNoise&) = delete;
	FlangeNoise& operator=(const FlangeNoise&) = delete;

	void init() override {
		pink1.amplitude(1.0);
		flange1.begin(flangeDelayLine, FLANGE_NOISE_DELAY_LENGTH, s_offset, s_depth, s_rate);
	}

	void process(float k1, float k2) override {
		float rate = 0.05f + pow(k1, 2) * 5.0f;
		int depth = (int)(2 + k2 * 120);   // wider sweep range for audible flanging
		int offset = (int)(2 + k2 * 80);

		flange1.voices(offset, depth, rate);
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {
		pink1.update(&pinkBlock);
		flange1.update(&pinkBlock, &flangeBlock);

		blockBuffer.pushBuffer(flangeBlock.data, AUDIO_BLOCK_SAMPLES);
	}

	AudioStream& getStream() override {
		return pink1;
	}
	unsigned char getPort() override {
		return 0;
	}

private:
	audio_block_t pinkBlock, flangeBlock;

	AudioSynthNoisePink      pink1;
	AudioEffectFlange        flange1;

	short flangeDelayLine[FLANGE_NOISE_DELAY_LENGTH];
	int s_offset = 2 * FLANGE_NOISE_DELAY_LENGTH / 4;
	int s_depth = FLANGE_NOISE_DELAY_LENGTH / 4;
	float s_rate = 0.5;
};

REGISTER_PLUGIN(FlangeNoise);
