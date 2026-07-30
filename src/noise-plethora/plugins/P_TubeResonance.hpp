#pragma once

#include "NoisePlethoraPlugin.hpp"

class TubeResonance : public NoisePlethoraPlugin {

public:

	TubeResonance() { }

	~TubeResonance() override {}

	// delete copy constructors
	TubeResonance(const TubeResonance&) = delete;
	TubeResonance& operator=(const TubeResonance&) = delete;

	void init() override {
		noise1.amplitude(1);

		for (int c = 0; c < 4; c++) {
			writePos[c] = 0;
			for (int i = 0; i < 2048; i++) {
				buf[c][i] = 0.0f;
			}
		}
		fundamentalFreq = 200.0f;
		feedback = 0.85f;
		excitation = 0.5f;
	}

	void process(float k1, float k2) override {
		fundamentalFreq = 40.0f + pow(k1, 2) * 960.0f;
		excitation = k2;
		feedback = 0.7f + (1.0f - k2) * 0.25f;

		// set noise amplitude proportional to excitation
		noise1.amplitude(excitation);

		// compute delay lengths for 4 harmonics
		for (int c = 0; c < 4; c++) {
			float harmonic = (float)(c + 1);
			float freq = fundamentalFreq * harmonic;
			float delaySamples = 44100.0f / freq;
			delayLen[c] = (int)delaySamples;
			if (delayLen[c] < 1) delayLen[c] = 1;
			if (delayLen[c] > 2047) delayLen[c] = 2047;

			// feedback per harmonic scaled by 1/harmonic_number
			harmonicFeedback[c] = feedback / harmonic;
		}
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {
		// generate noise block
		noise1.update(&noiseBlock);

		for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
			float input = noiseBlock.data[i] / 32767.0f;
			float mixed = 0.0f;

			// process 4 comb filters in parallel
			for (int c = 0; c < 4; c++) {
				int readPos = writePos[c] - delayLen[c];
				if (readPos < 0) readPos += 2048;

				float delayOut = buf[c][readPos];
				float out = input + harmonicFeedback[c] * delayOut;
				buf[c][writePos[c]] = out;

				writePos[c] = (writePos[c] + 1) & 2047; // mod 2048

				mixed += out;
			}

			// mix 4 combs equally
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

	float buf[4][2048] = {};
	int writePos[4] = {};
	int delayLen[4] = {100, 50, 33, 25};
	float harmonicFeedback[4] = {};
	float fundamentalFreq = 200.0f;
	float feedback = 0.85f;
	float excitation = 0.5f;
};

REGISTER_PLUGIN(TubeResonance); // this is important, so that we can include the plugin in a bank
