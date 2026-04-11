#pragma once

#include <cmath>
#include "NoisePlethoraPlugin.hpp"

class ShiftPulse : public NoisePlethoraPlugin {

public:

	ShiftPulse() { }

	~ShiftPulse() override {}

	ShiftPulse(const ShiftPulse&) = delete;
	ShiftPulse& operator=(const ShiftPulse&) = delete;

	void init() override {
		for (int i = 0; i < 4; i++) {
			stages[i] = 0.0f;
			stageWeights[i] = 0.0f;
		}
		stageWeights[0] = 1.0f;
		sampleCounter = 0;
		samplesPerClock = 441;
	}

	void process(float k1, float k2) override {
		float rate = 5.0f + std::pow(k1, 2) * 295.0f;
		samplesPerClock = std::max(1, (int)(44100.0f / rate));

		// Crossfade stage weights based on k2
		stageWeights[0] = 1.0f;
		stageWeights[1] = std::min(1.0f, k2 * 3.0f);
		stageWeights[2] = std::min(1.0f, std::max(0.0f, k2 * 3.0f - 1.0f));
		stageWeights[3] = std::min(1.0f, std::max(0.0f, k2 * 3.0f - 2.0f));

		// Normalize weights
		float sum = stageWeights[0] + stageWeights[1] + stageWeights[2] + stageWeights[3];
		if (sum > 0.0f) {
			for (int i = 0; i < 4; i++) {
				stageWeights[i] /= sum;
			}
		}
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {

		for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
			sampleCounter++;
			if (sampleCounter >= samplesPerClock) {
				// Shift register: shift down
				stages[3] = stages[2];
				stages[2] = stages[1];
				stages[1] = stages[0];
				stages[0] = random::uniform() * 2.0f - 1.0f;

				sampleCounter = 0;
			}

			float out = 0.0f;
			for (int s = 0; s < 4; s++) {
				out += stageWeights[s] * stages[s];
			}

			// NaN/Inf guard
			if (!std::isfinite(out)) {
				out = 0.0f;
			}

			out += (random::uniform() - 0.5f) * 0.01f;

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

	float stages[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	float stageWeights[4] = {1.0f, 0.0f, 0.0f, 0.0f};
	int sampleCounter = 0;
	int samplesPerClock = 441;
};

REGISTER_PLUGIN(ShiftPulse);
