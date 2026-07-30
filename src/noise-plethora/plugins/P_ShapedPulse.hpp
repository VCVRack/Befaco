#pragma once

#include <cmath>
#include "NoisePlethoraPlugin.hpp"

class ShapedPulse : public NoisePlethoraPlugin {

public:

	ShapedPulse() { }

	~ShapedPulse() override {}

	ShapedPulse(const ShapedPulse&) = delete;
	ShapedPulse& operator=(const ShapedPulse&) = delete;

	void init() override {
		currentValue = 0.0f;
		sampleCounter = 0;
		samplesPerClock = 441;
		shape = 0.0f;
	}

	void process(float k1, float k2) override {
		float rate = 5.0f + std::pow(k1, 2) * 495.0f;
		samplesPerClock = std::max(1, (int)(44100.0f / rate));

		shape = k2;
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {

		for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
			sampleCounter++;
			if (sampleCounter >= samplesPerClock) {
				float val;

				if (shape < 0.5f) {
					// Blend uniform and triangular
					float blend = shape * 2.0f;
					float uniform = random::uniform() * 2.0f - 1.0f;
					float tri = (random::uniform() + random::uniform()) - 1.0f;
					val = uniform * (1.0f - blend) + tri * blend;
				}
				else {
					// Blend triangular and peaked (pseudo-gaussian)
					float blend = (shape - 0.5f) * 2.0f;
					float tri = (random::uniform() + random::uniform()) - 1.0f;
					float peaked = (random::uniform() + random::uniform() + random::uniform() + random::uniform()) * 0.5f - 1.0f;
					val = tri * (1.0f - blend) + peaked * blend;
				}

				if (val > 1.0f) val = 1.0f;
				if (val < -1.0f) val = -1.0f;

				currentValue = val;
				sampleCounter = 0;
			}

			// NaN/Inf guard
			if (!std::isfinite(currentValue)) {
				currentValue = 0.0f;
			}

			float out = currentValue + (random::uniform() - 0.5f) * 0.01f;

			if (out > 1.0f) out = 1.0f;
			if (out < -1.0f) out = -1.0f;

			int32_t sample = (int32_t)(out * 32767.0f);
			if (sample > 32767) sample = 32767;
			if (sample < -32767) sample = -32767;
			outputBlock.data[i] = (int16_t)sample;
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
	AudioSynthWaveformDc dc1;

	audio_block_t outputBlock;

	float currentValue = 0.0f;
	int sampleCounter = 0;
	int samplesPerClock = 441;
	float shape = 0.0f;
};

REGISTER_PLUGIN(ShapedPulse);
