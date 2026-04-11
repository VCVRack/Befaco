#pragma once

#include "NoisePlethoraPlugin.hpp"
#include <cmath>

class DualAttractor : public NoisePlethoraPlugin {

public:

	DualAttractor() { }

	~DualAttractor() override {}

	DualAttractor(const DualAttractor&) = delete;
	DualAttractor& operator=(const DualAttractor&) = delete;

	void init() override {
		// Lorenz system 1
		x1 = 1.0f;
		y1 = 1.0f;
		z1 = 1.0f;

		// Lorenz system 2
		x2 = -1.0f;
		y2 = 0.0f;
		z2 = 2.0f;

		coupling = 0.0f;
		baseFreq = 100.0f;

		sine1.amplitude(0.7f);
		sine1.frequency(100.0f);
		sine2.amplitude(0.7f);
		sine2.frequency(150.0f);

		mixer.gain(0, 1.0f);
		mixer.gain(1, 1.0f);
		mixer.gain(2, 0.0f);
		mixer.gain(3, 0.0f);
	}

	void process(float k1, float k2) override {
		coupling = k1 * 5.0f;
		baseFreq = 30.0f + std::pow(k2, 2.0f) * 2000.0f;

		// Lorenz parameters
		const float sigma = 10.0f;
		const float rho = 28.0f;
		const float beta = 8.0f / 3.0f;
		const float dt = 0.001f;

		// Run 10 integration steps for stability
		for (int step = 0; step < 10; step++) {
			float dx1 = sigma * (y1 - x1) + coupling * (x2 - x1);
			float dy1 = x1 * (rho - z1) - y1;
			float dz1 = x1 * y1 - beta * z1;

			float dx2 = sigma * (y2 - x2) + coupling * (x1 - x2);
			float dy2 = x2 * (rho - z2) - y2;
			float dz2 = x2 * y2 - beta * z2;

			x1 += dx1 * dt;
			y1 += dy1 * dt;
			z1 += dz1 * dt;

			x2 += dx2 * dt;
			y2 += dy2 * dt;
			z2 += dz2 * dt;
		}

		// Check for NaN/infinity and reset if needed
		if (!std::isfinite(x1) || !std::isfinite(y1) || !std::isfinite(z1)) {
			x1 = 1.0f;
			y1 = 1.0f;
			z1 = 1.0f;
		}
		if (!std::isfinite(x2) || !std::isfinite(y2) || !std::isfinite(z2)) {
			x2 = -1.0f;
			y2 = 0.0f;
			z2 = 2.0f;
		}

		// Map Lorenz x outputs to frequency offsets
		float freq1 = baseFreq + x1 * baseFreq * 0.1f;
		float freq2 = baseFreq * 1.5f + x2 * baseFreq * 0.1f;

		// Clamp frequencies to reasonable range
		if (freq1 < 20.0f) freq1 = 20.0f;
		if (freq1 > 20000.0f) freq1 = 20000.0f;
		if (freq2 < 20.0f) freq2 = 20.0f;
		if (freq2 > 20000.0f) freq2 = 20000.0f;

		sine1.frequency(freq1);
		sine2.frequency(freq2);
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {
		sine1.update(&s1Block);
		sine2.update(&s2Block);
		mixer.update(&s1Block, &s2Block, nullptr, nullptr, &mixBlock);

		blockBuffer.pushBuffer(mixBlock.data, AUDIO_BLOCK_SAMPLES);
	}

	AudioStream& getStream() override {
		return mixer;
	}
	unsigned char getPort() override {
		return 0;
	}

private:
	AudioSynthWaveformSine sine1;
	AudioSynthWaveformSine sine2;
	AudioMixer4 mixer;

	audio_block_t s1Block, s2Block, mixBlock;

	// Lorenz system 1
	float x1 = 1.0f;
	float y1 = 1.0f;
	float z1 = 1.0f;

	// Lorenz system 2
	float x2 = -1.0f;
	float y2 = 0.0f;
	float z2 = 2.0f;

	float coupling = 0.0f;
	float baseFreq = 100.0f;
};

REGISTER_PLUGIN(DualAttractor);
