#pragma once

#include "NoisePlethoraPlugin.hpp"

class SubHarmonic : public NoisePlethoraPlugin {

public:

	SubHarmonic() { }

	~SubHarmonic() override {}

	SubHarmonic(const SubHarmonic&) = delete;
	SubHarmonic& operator=(const SubHarmonic&) = delete;

	void init() override {
		source.begin(1.0f, 440.0f, WAVEFORM_SQUARE);

		for (int i = 0; i < 4; i++) {
			divCounters[i] = 0;
			divStates[i] = false;
		}

		divWeights[0] = 1.0f;
		divWeights[1] = 0.0f;
		divWeights[2] = 0.0f;
		divWeights[3] = 0.0f;

		prevSamplePositive = true;
	}

	void process(float k1, float k2) override {
		float freq = 100.0f + std::pow(k1, 2.0f) * 4000.0f;
		source.frequency(freq);

		// Crossfade subharmonic levels based on k2
		divWeights[0] = 1.0f; // div2 always active
		divWeights[1] = std::min(1.0f, std::max(0.0f, k2 * 3.0f));       // div3
		divWeights[2] = std::min(1.0f, std::max(0.0f, k2 * 3.0f - 1.0f)); // div5
		divWeights[3] = std::min(1.0f, std::max(0.0f, k2 * 3.0f - 2.0f)); // div7
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {
		source.update(&srcBlock);

		for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
			int16_t srcSample = srcBlock.data[i];
			bool currentPositive = (srcSample >= 0);

			// Detect zero crossing (sign change)
			if (currentPositive != prevSamplePositive) {
				for (int d = 0; d < 4; d++) {
					divCounters[d]++;
					if (divCounters[d] >= divRatios[d]) {
						divStates[d] = !divStates[d];
						divCounters[d] = 0;
					}
				}
			}
			prevSamplePositive = currentPositive;

			// Mix: source quieter, subharmonics louder so you clearly hear them come in
			float srcFloat = (float)srcSample / 32767.0f;
			float mix = 0.15f * srcFloat;

			for (int d = 0; d < 4; d++) {
				float divValue = divStates[d] ? 0.5f : -0.5f;
				mix += divWeights[d] * divValue;
			}

			int32_t out = (int32_t)(mix * 32767.0f);
			if (out > 32767) out = 32767;
			if (out < -32767) out = -32767;
			outputBlock.data[i] = (int16_t)out;
		}

		blockBuffer.pushBuffer(outputBlock.data, AUDIO_BLOCK_SAMPLES);
	}

	AudioStream& getStream() override {
		return source;
	}
	unsigned char getPort() override {
		return 0;
	}

private:
	AudioSynthWaveform source;

	audio_block_t srcBlock;
	audio_block_t outputBlock;

	int divRatios[4] = {2, 3, 5, 7};
	int divCounters[4] = {0, 0, 0, 0};
	bool divStates[4] = {false, false, false, false};
	float divWeights[4] = {1.0f, 0.0f, 0.0f, 0.0f};

	bool prevSamplePositive = true;
};

REGISTER_PLUGIN(SubHarmonic);
