#pragma once

#include "NoisePlethoraPlugin.hpp"
#include <cmath>

// NoiseBells: White noise excites 4 sharp 2-pole resonators at bell-like
// inharmonic ratios. The resonators have very high Q (~500+), creating
// clear ringing tones from noise. Like striking a metal bar or bell.
//
// 2-pole resonator: y[n] = 2*r*cos(w)*y[n-1] - r^2*y[n-2] + (1-r)*x[n]
// r close to 1 = long ring, sharp peak. w = 2*pi*freq/samplerate.

class NoiseBells : public NoisePlethoraPlugin {

public:

	NoiseBells() { }

	~NoiseBells() override {}

	NoiseBells(const NoiseBells&) = delete;
	NoiseBells& operator=(const NoiseBells&) = delete;

	void init() override {
		noise1.amplitude(1.0);

		for (int i = 0; i < NUM_RESONATORS; i++) {
			y1[i] = 0.0f;
			y2[i] = 0.0f;
			coeff_a[i] = 0.0f;
			coeff_b[i] = 0.0f;
			coeff_g[i] = 0.0f;
		}

		// initial resonator setup
		updateResonators(500.0f, 0.5f);
	}

	void process(float k1, float k2) override {
		float freq = 80.0f + pow(k1, 2) * 4000.0f;
		updateResonators(freq, k2);
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {
		noise1.update(&noiseBlock);

		for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
			float input = noiseBlock.data[i] / 32767.0f;

			float mixed = 0.0f;

			for (int r = 0; r < NUM_RESONATORS; r++) {
				// 2-pole resonator
				float out = coeff_a[r] * y1[r] - coeff_b[r] * y2[r] + coeff_g[r] * input;

				y2[r] = y1[r];
				y1[r] = out;

				// safety: reset on NaN/inf
				if (!std::isfinite(out)) {
					y1[r] = 0.0f;
					y2[r] = 0.0f;
					out = 0.0f;
				}

				mixed += out * gains[r];
			}

			// scale down (resonators can be loud)
			mixed *= 0.15f;

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

	static const int NUM_RESONATORS = 4;

	void updateResonators(float fundamental, float inharmonicity) {
		// bell-like inharmonic ratios (circular membrane / bar modes)
		// at inharmonicity=0: nearly harmonic. at 1: very stretched.
		float stretch = 1.0f + inharmonicity * 2.0f;
		float freqs[NUM_RESONATORS];
		freqs[0] = fundamental;
		freqs[1] = fundamental * (1.0f + 0.59f * stretch);   // ~1.59 - 2.77
		freqs[2] = fundamental * (1.0f + 1.33f * stretch);   // ~2.33 - 4.99
		freqs[3] = fundamental * (1.0f + 2.14f * stretch);   // ~3.14 - 7.42

		// r controls ring time. closer to 1 = longer ring = sharper peak
		// higher partials decay faster (lower r)
		float baseR = 0.9992f;

		for (int i = 0; i < NUM_RESONATORS; i++) {
			float f = freqs[i];
			if (f > 20000.0f) f = 20000.0f;
			if (f < 20.0f) f = 20.0f;

			float r = baseR - i * 0.0003f;  // higher partials decay slightly faster
			float w = 2.0f * M_PI * f / 44100.0f;

			coeff_a[i] = 2.0f * r * std::cos(w);
			coeff_b[i] = r * r;
			coeff_g[i] = 1.0f - r;  // input gain scales with (1-r) for normalization
		}

		// amplitude rolloff for higher partials
		gains[0] = 1.0f;
		gains[1] = 0.7f;
		gains[2] = 0.4f;
		gains[3] = 0.25f;
	}

	AudioSynthNoiseWhite noise1;
	AudioSynthWaveformDc dc1;  // dummy for getStream

	audio_block_t noiseBlock, outputBlock;

	// resonator state
	float y1[NUM_RESONATORS] = {};
	float y2[NUM_RESONATORS] = {};

	// resonator coefficients
	float coeff_a[NUM_RESONATORS] = {};
	float coeff_b[NUM_RESONATORS] = {};
	float coeff_g[NUM_RESONATORS] = {};
	float gains[NUM_RESONATORS] = {};
};

REGISTER_PLUGIN(NoiseBells);
