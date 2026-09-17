#pragma once

#include "NoisePlethoraPlugin.hpp"

class HenonDust : public NoisePlethoraPlugin {

public:

	HenonDust() { }

	~HenonDust() override {}

	HenonDust(const HenonDust&) = delete;
	HenonDust& operator=(const HenonDust&) = delete;

	void init() override {
		hx = 0.1f;
		hy = 0.1f;
		a = 1.4f;
		b = 0.3f;
	}

	void process(float k1, float k2) override {
		// focus k1 on the interesting chaos transition zone (1.15-1.4)
		a = 1.15f + k1 * 0.25f;
		b = 0.15f + k2 * 0.2f;
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {

		for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
			float new_x = 1.0f - a * hx * hx + hy;
			float new_y = b * hx;
			hx = new_x;
			hy = new_y;

			// Reset on divergence or NaN
			if (std::isnan(hx) || std::isinf(hx) || std::isnan(hy) || std::isinf(hy)) {
				hx = random::uniform() - 0.5f;
				hy = random::uniform() - 0.5f;
			}

			// Normalize hx from [-1.5, 1.5] to [-1, 1]
			float out = hx;
			if (out > 1.5f) out = 1.5f;
			if (out < -1.5f) out = -1.5f;
			out /= 1.5f;

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

	float hx = 0.1f;
	float hy = 0.1f;
	float a = 1.4f;
	float b = 0.3f;
};

REGISTER_PLUGIN(HenonDust);
