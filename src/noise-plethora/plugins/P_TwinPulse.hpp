#pragma once

#include <cmath>
#include "NoisePlethoraPlugin.hpp"

class TwinPulse : public NoisePlethoraPlugin {

public:

	TwinPulse() { }

	~TwinPulse() override {}

	TwinPulse(const TwinPulse&) = delete;
	TwinPulse& operator=(const TwinPulse&) = delete;

	void init() override {
		currentA = random::uniform() * 2.0f - 1.0f;
		targetA = random::uniform() * 2.0f - 1.0f;
		currentB = random::uniform() * 2.0f - 1.0f;
		targetB = random::uniform() * 2.0f - 1.0f;
		counterA = 0;
		counterB = 0;
		alpha = 0.1f;
		correlation = 0.0f;
		samplesPerTarget = 441;
	}

	void process(float k1, float k2) override {
		float rate = 5.0f + std::pow(k1, 2) * 395.0f;
		samplesPerTarget = std::max(1, (int)(44100.0f / rate));

		alpha = 4.0f / (float)samplesPerTarget;
		if (alpha > 0.5f) alpha = 0.5f;

		correlation = k2;
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {

		for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
			counterA++;
			if (counterA >= samplesPerTarget) {
				targetA = random::uniform() * 2.0f - 1.0f;
				counterA = 0;
			}

			counterB++;
			if (counterB >= samplesPerTarget) {
				targetB = random::uniform() * 2.0f - 1.0f;
				counterB = 0;
			}

			currentA = currentA * (1.0f - alpha) + targetA * alpha;
			currentB = currentB * (1.0f - alpha) + targetB * alpha;

			// NaN/Inf guard
			if (!std::isfinite(currentA)) currentA = 0.0f;
			if (!std::isfinite(currentB)) currentB = 0.0f;

			float blendedB = correlation * currentA + (1.0f - correlation) * currentB;
			float out = 0.5f * currentA + 0.5f * blendedB;

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

	float currentA = 0.0f;
	float targetA = 0.0f;
	float currentB = 0.0f;
	float targetB = 0.0f;
	int counterA = 0;
	int counterB = 0;
	int samplesPerTarget = 441;
	float alpha = 0.1f;
	float correlation = 0.0f;
};

REGISTER_PLUGIN(TwinPulse);
