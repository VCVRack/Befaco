#pragma once

#include <cmath>
#include "NoisePlethoraPlugin.hpp"

class PulseWander : public NoisePlethoraPlugin {

public:

	PulseWander() { }

	~PulseWander() override {}

	PulseWander(const PulseWander&) = delete;
	PulseWander& operator=(const PulseWander&) = delete;

	void init() override {
		currentValue = random::uniform() * 2.0f - 1.0f;
		targetValue = random::uniform() * 2.0f - 1.0f;
		smoothAlpha = 0.1f;
		sampleCounter = 0;
		samplesPerTarget = 441;
	}

	void process(float k1, float k2) override {
		float rate = 5.0f + std::pow(k1, 2) * 395.0f;
		samplesPerTarget = std::max(1, (int)(44100.0f / rate));

		float minAlpha = 3.0f / (float)samplesPerTarget;
		smoothAlpha = minAlpha + (1.0f - k2) * (1.0f - minAlpha);
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {

		for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
			sampleCounter++;
			if (sampleCounter >= samplesPerTarget) {
				targetValue = random::uniform() * 2.0f - 1.0f;
				sampleCounter = 0;
			}

			currentValue = currentValue * (1.0f - smoothAlpha) + targetValue * smoothAlpha;

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
	float targetValue = 0.0f;
	float smoothAlpha = 0.1f;
	int sampleCounter = 0;
	int samplesPerTarget = 441;
};

REGISTER_PLUGIN(PulseWander);
