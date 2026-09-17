#pragma once

#include "NoisePlethoraPlugin.hpp"
#include <cmath>

class NoiseSlew : public NoisePlethoraPlugin {

public:

	NoiseSlew() { }

	~NoiseSlew() override {}

	NoiseSlew(const NoiseSlew&) = delete;
	NoiseSlew& operator=(const NoiseSlew&) = delete;

	void init() override {
		noise1.amplitude(1.0f);
		slewState = 0.0f;
		alpha = 0.01f;
		mix = 0.0f;
	}

	void process(float k1, float k2) override {
		alpha = 0.0003f + std::pow(k1, 3) * 0.15f;
		mix = k2;
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {
		noise1.update(&noiseBlock);

		for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
			float noiseSample = noiseBlock.data[i] / 32767.0f;

			slewState = slewState * (1.0f - alpha) + noiseSample * alpha;

			// NaN guard
			if (std::isnan(slewState) || std::isinf(slewState)) {
				slewState = 0.0f;
			}

			float out = slewState * (1.0f - mix * 0.7f) + noiseSample * mix * 0.3f;

			if (out > 1.0f) out = 1.0f;
			if (out < -1.0f) out = -1.0f;

			outputBlock.data[i] = (int16_t)(out * 32767.0f);
		}

		blockBuffer.pushBuffer(outputBlock.data, AUDIO_BLOCK_SAMPLES);
	}

	AudioStream& getStream() override {
		return dc1;
	}
	unsigned char getPort() override {
		return 0;
	}

private:
	AudioSynthWaveformDc dc1; // dummy for getStream()
	AudioSynthNoiseWhite noise1;

	audio_block_t noiseBlock, outputBlock;

	float slewState = 0.0f;
	float alpha = 0.01f;
	float mix = 0.0f;
};

REGISTER_PLUGIN(NoiseSlew);
