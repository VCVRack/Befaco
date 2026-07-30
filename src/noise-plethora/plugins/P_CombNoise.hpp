#pragma once

#include "NoisePlethoraPlugin.hpp"

class CombNoise : public NoisePlethoraPlugin {

public:

	CombNoise() { }

	~CombNoise() override {}

	// delete copy constructors
	CombNoise(const CombNoise&) = delete;
	CombNoise& operator=(const CombNoise&) = delete;

	void init() override {
		noise1.amplitude(1);

		for (int c = 0; c < 4; c++) {
			writePos[c] = 0;
			for (int i = 0; i < 2048; i++) {
				delayBuffer[c][i] = 0.0f;
			}
		}
	}

	void process(float k1, float k2) override {
		float delayInSamples = 44100.0f / (40.0f + pow(k1, 2) * 4960.0f);
		feedback = k2 * 0.95f;

		// 4 combs at delay ratios 1.0, 0.7, 0.5, 0.35
		delaySamples[0] = delayInSamples * 1.0f;
		delaySamples[1] = delayInSamples * 0.7f;
		delaySamples[2] = delayInSamples * 0.5f;
		delaySamples[3] = delayInSamples * 0.35f;

		// clamp delay lengths to valid buffer range
		for (int c = 0; c < 4; c++) {
			if (delaySamples[c] < 1.0f) delaySamples[c] = 1.0f;
			if (delaySamples[c] > 2047.0f) delaySamples[c] = 2047.0f;
		}
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {
		// generate noise block
		noise1.update(&noiseBlock);

		for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
			float input = noiseBlock.data[i] / 32767.0f;
			float mixed = 0.0f;

			for (int c = 0; c < 4; c++) {
				int delay = (int)delaySamples[c];
				if (delay < 1) delay = 1;
				if (delay > 2047) delay = 2047;

				int readPos = writePos[c] - delay;
				if (readPos < 0) readPos += 2048;

				float out = input + feedback * delayBuffer[c][readPos];
				delayBuffer[c][writePos[c]] = out;

				writePos[c] = (writePos[c] + 1) & 2047; // mod 2048

				mixed += out;
			}

			// mix 4 combs equally (scale by 0.25)
			mixed *= 0.25f;

			// clamp and convert to int16
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
	AudioSynthNoiseWhite noise1;
	AudioSynthWaveformDc dc1; // dummy for getStream()

	audio_block_t noiseBlock, outputBlock;

	float delayBuffer[4][2048] = {};
	int writePos[4] = {};
	float delaySamples[4] = {100.0f, 70.0f, 50.0f, 35.0f};
	float feedback = 0.0f;
};

REGISTER_PLUGIN(CombNoise); // this is important, so that we can include the plugin in a bank
