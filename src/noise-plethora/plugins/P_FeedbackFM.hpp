#pragma once

#include "NoisePlethoraPlugin.hpp"
#include <cmath>

class FeedbackFM : public NoisePlethoraPlugin {

public:

	FeedbackFM() { }

	~FeedbackFM() override {}

	FeedbackFM(const FeedbackFM&) = delete;
	FeedbackFM& operator=(const FeedbackFM&) = delete;

	void init() override {
		phase = 0.0f;
		prevOut = 0.0f;
		currentFreq = 440.0f;
		currentFeedback = 0.0f;
	}

	void process(float k1, float k2) override {
		currentFreq = 20.0f + std::pow(k1, 2.0f) * 5000.0f;
		currentFeedback = k2 * 2.5f;
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {
		const float twoPi = 2.0f * 3.14159265358979323846f;
		const float phaseInc = twoPi * currentFreq / 44100.0f;

		for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
			phase += phaseInc;
			phase = std::fmod(phase, twoPi);

			float out = std::sin(phase + currentFeedback * prevOut);
			prevOut = out;

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

	float phase = 0.0f;
	float prevOut = 0.0f;
	float currentFreq = 440.0f;
	float currentFeedback = 0.0f;
};

REGISTER_PLUGIN(FeedbackFM);
