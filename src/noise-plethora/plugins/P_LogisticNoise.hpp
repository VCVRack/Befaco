#pragma once

#include "NoisePlethoraPlugin.hpp"

class LogisticNoise : public NoisePlethoraPlugin {

public:

	LogisticNoise() { }

	~LogisticNoise() override {}

	LogisticNoise(const LogisticNoise&) = delete;
	LogisticNoise& operator=(const LogisticNoise&) = delete;

	void init() override {
		x = 0.5f;
		holdCounter = 0;
		samplesPerIteration = 1;
		r = 3.8f;
	}

	void process(float k1, float k2) override {
		r = 3.5f + k1 * 0.5f;
		samplesPerIteration = std::max(1, (int)(1 + (1.0f - k2) * 30));
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {

		for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
			holdCounter++;
			if (holdCounter >= samplesPerIteration) {
				x = r * x * (1.0f - x);
				holdCounter = 0;

				// Reset on numerical instability
				if (x < 0.0f || x > 1.0f || std::isnan(x) || std::isinf(x)) {
					x = 0.5f + (random::uniform() - 0.5f) * 0.1f;
				}
			}

			float out = x * 2.0f - 1.0f;
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
	AudioSynthWaveformDc dc1; // dummy for getStream()

	audio_block_t outputBlock;

	float x = 0.5f;
	float r = 3.8f;
	int holdCounter = 0;
	int samplesPerIteration = 1;
};

REGISTER_PLUGIN(LogisticNoise);
