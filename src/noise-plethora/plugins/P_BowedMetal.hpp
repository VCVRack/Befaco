#pragma once

#include "NoisePlethoraPlugin.hpp"
#include <cmath>

// BowedMetal: White noise excites 8 high-Q resonators at dense, inharmonic
// metallic ratios. More resonators + closer spacing than NoiseBells = denser,
// more reverberant metallic character. Like bowing a cymbal or singing bowl.

class BowedMetal : public NoisePlethoraPlugin {

public:

	BowedMetal() { }

	~BowedMetal() override {}

	BowedMetal(const BowedMetal&) = delete;
	BowedMetal& operator=(const BowedMetal&) = delete;

	void init() override {
		noise1.amplitude(0.5f);

		for (int i = 0; i < NUM_RES; i++) {
			y1[i] = 0.0f;
			y2[i] = 0.0f;
		}

		updateResonators(200.0f, 0.5f, 0.5f);
	}

	void process(float k1, float k2) override {
		float freq = 30.0f + pow(k1, 2) * 2000.0f;

		// k2 low = struck (short ring, louder noise burst)
		// k2 high = bowed (long ring, quieter continuous noise)
		float ringTime = 0.9985f + k2 * 0.0013f;  // r: 0.9985 → 0.9998 (higher baseline)
		float noiseLevel = 0.4f - k2 * 0.32f;      // 0.4 → 0.08 (less noise overall)
		noise1.amplitude(noiseLevel);

		updateResonators(freq, k2, ringTime);
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {
		noise1.update(&noiseBlock);

		for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
			float input = noiseBlock.data[i] / 32767.0f;

			float mixed = 0.0f;

			for (int r = 0; r < NUM_RES; r++) {
				float out = coeff_a[r] * y1[r] - coeff_b[r] * y2[r] + coeff_g[r] * input;

				y2[r] = y1[r];
				y1[r] = out;

				if (!std::isfinite(out)) {
					y1[r] = 0.0f;
					y2[r] = 0.0f;
					out = 0.0f;
				}

				mixed += out * gains[r];
			}

			mixed *= 0.1f;

			if (mixed > 1.0f) mixed = 1.0f;
			if (mixed < -1.0f) mixed = -1.0f;
			outputBlock.data[i] = (int16_t)(mixed * 32767.0f);
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

	static const int NUM_RES = 8;

	// Metal plate / cymbal mode ratios (based on circular plate vibration modes)
	// These are denser and more irregular than bell ratios
	static constexpr float modeRatios[NUM_RES] = {
		1.000f, 1.183f, 1.506f, 1.741f,
		2.098f, 2.534f, 2.917f, 3.483f
	};

	void updateResonators(float fundamental, float k2, float ringTime) {
		for (int i = 0; i < NUM_RES; i++) {
			float f = fundamental * modeRatios[i];
			if (f > 19000.0f) f = 19000.0f;
			if (f < 20.0f) f = 20.0f;

			// higher modes decay faster
			float r = ringTime - i * 0.0002f;
			if (r < 0.99f) r = 0.99f;

			float w = 2.0f * M_PI * f / 44100.0f;

			coeff_a[i] = 2.0f * r * std::cos(w);
			coeff_b[i] = r * r;
			coeff_g[i] = 1.0f - r;
		}

		// dense amplitude distribution (all modes audible, slight rolloff)
		gains[0] = 1.0f;
		gains[1] = 0.9f;
		gains[2] = 0.85f;
		gains[3] = 0.75f;
		gains[4] = 0.65f;
		gains[5] = 0.55f;
		gains[6] = 0.45f;
		gains[7] = 0.35f;
	}

	AudioSynthNoiseWhite noise1;
	AudioSynthWaveformDc dc1;

	audio_block_t noiseBlock, outputBlock;

	float y1[NUM_RES] = {};
	float y2[NUM_RES] = {};

	float coeff_a[NUM_RES] = {};
	float coeff_b[NUM_RES] = {};
	float coeff_g[NUM_RES] = {};
	float gains[NUM_RES] = {};
};

REGISTER_PLUGIN(BowedMetal);
