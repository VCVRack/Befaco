#pragma once

#include "NoisePlethoraPlugin.hpp"
#include <cmath>

class DualPulse : public NoisePlethoraPlugin {

public:

	DualPulse() { }

	~DualPulse() override {}

	DualPulse(const DualPulse&) = delete;
	DualPulse& operator=(const DualPulse&) = delete;

	void init() override {
		value1 = 0.0f;
		value2 = 0.0f;
		counter1 = 0;
		counter2 = 0;
		samplesPerClock1 = 441;
		samplesPerClock2 = 441;
	}

	void process(float k1, float k2) override {
		float rate1 = 5.0f + std::pow(k1, 2) * 495.0f;
		float rate2 = 5.0f + std::pow(k2, 2) * 495.0f;
		samplesPerClock1 = (int)(AUDIO_SAMPLE_RATE_EXACT / rate1);
		if (samplesPerClock1 < 1) samplesPerClock1 = 1;
		samplesPerClock2 = (int)(AUDIO_SAMPLE_RATE_EXACT / rate2);
		if (samplesPerClock2 < 1) samplesPerClock2 = 1;
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {

		for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
			counter1++;
			if (counter1 >= samplesPerClock1) {
				value1 = random::uniform() * 2.0f - 1.0f;
				counter1 = 0;
			}

			counter2++;
			if (counter2 >= samplesPerClock2) {
				value2 = random::uniform() * 2.0f - 1.0f;
				counter2 = 0;
			}

			float out = 0.5f * value1 + 0.5f * value2;

			// Add dither
			out += (random::uniform() - 0.5f) * 0.01f;

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

	audio_block_t outputBlock;

	float value1 = 0.0f;
	float value2 = 0.0f;
	int counter1 = 0;
	int counter2 = 0;
	int samplesPerClock1 = 441;
	int samplesPerClock2 = 441;
};

REGISTER_PLUGIN(DualPulse);
