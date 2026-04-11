#pragma once

#include "NoisePlethoraPlugin.hpp"
#include <cmath>

class LFSRNoise : public NoisePlethoraPlugin {

public:

	LFSRNoise() { }

	~LFSRNoise() override {}

	LFSRNoise(const LFSRNoise&) = delete;
	LFSRNoise& operator=(const LFSRNoise&) = delete;

	void init() override {
		lfsr = 0xACE1;
		clockCounter = 0;
		samplesPerClock = 441;
		currentTaps = taps[0];
	}

	void process(float k1, float k2) override {
		float rate = 5.0f + std::pow(k1, 2) * 495.0f;
		samplesPerClock = (int)(AUDIO_SAMPLE_RATE_EXACT / rate);
		if (samplesPerClock < 1) samplesPerClock = 1;

		int tapIndex = (int)(k2 * 7.99f);
		if (tapIndex < 0) tapIndex = 0;
		if (tapIndex > 7) tapIndex = 7;
		currentTaps = taps[tapIndex];
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {

		for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
			clockCounter++;
			if (clockCounter >= samplesPerClock) {
				uint16_t masked = lfsr & currentTaps;
				uint16_t feedback = __builtin_popcount(masked) & 1;
				lfsr = (lfsr >> 1) | (feedback << 15);
				if (lfsr == 0) lfsr = 0xACE1;  // prevent permanent lockup
				clockCounter = 0;
			}

			// Output top 4 bits: 16 discrete levels
			int16_t quantized = (int16_t)(lfsr >> 12);
			float out = quantized / 7.5f - 1.0f;

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

	static constexpr uint16_t taps[8] = {
		0x002D, 0x0039, 0xD008, 0xB400,
		0x6000, 0xD295, 0xB002, 0xE100
	};

	uint16_t lfsr = 0xACE1;
	int clockCounter = 0;
	int samplesPerClock = 441;
	uint16_t currentTaps = 0x002D;
};

REGISTER_PLUGIN(LFSRNoise);
