#pragma once

#include "NoisePlethoraPlugin.hpp"

class PluckCloud : public NoisePlethoraPlugin {

public:

	PluckCloud() { }

	~PluckCloud() override {}

	// delete copy constructors
	PluckCloud(const PluckCloud&) = delete;
	PluckCloud& operator=(const PluckCloud&) = delete;

	void init() override {
		for (int v = 0; v < 6; v++) {
			writePos[v] = 0;
			filterState[v] = 0.0f;
			triggered[v] = false;
			for (int i = 0; i < 1024; i++) {
				buf[v][i] = 0.0f;
			}
		}
		damping = 0.5f;
		baseDelay = 100.0f;
	}

	void process(float k1, float k2) override {
		float freq = 60.0f + pow(k1, 2) * 2940.0f;
		baseDelay = 44100.0f / freq;
		damping = 0.98f - k2 * 0.58f;  // k2=0: heavy LP (dull thud), k2=1: light LP (bright ring)

		// random retrigger: each voice has ~2% chance per process() call
		for (int v = 0; v < 6; v++) {
			if (random::uniform() < 0.02f) {
				triggerVoice(v);
			}
		}
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {
		// voice detune ratios
		static const float detuneRatio[6] = {1.0f, 1.005f, 0.995f, 1.01f, 0.99f, 1.015f};

		for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
			float mixed = 0.0f;

			for (int v = 0; v < 6; v++) {
				float voiceDelay = baseDelay * detuneRatio[v];
				int delay = (int)voiceDelay;
				if (delay < 1) delay = 1;
				if (delay > 1023) delay = 1023;

				int readPos = writePos[v] - delay;
				if (readPos < 0) readPos += 1024;

				// read from delay and apply one-pole LP filter
				float delayOut = buf[v][readPos];
				float filtered = damping * filterState[v] + (1.0f - damping) * delayOut;
				filterState[v] = filtered;

				// write filtered output back into delay line
				buf[v][writePos[v]] = filtered;
				writePos[v] = (writePos[v] + 1) & 1023; // mod 1024

				mixed += filtered;
			}

			// scale by number of voices
			mixed *= (1.0f / 6.0f);

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

	void triggerVoice(int v) {
		// fill delay buffer BEHIND writePos so it's immediately read
		float voiceDelay = baseDelay;
		int delay = (int)voiceDelay;
		if (delay < 1) delay = 1;
		if (delay > 1023) delay = 1023;

		for (int i = 0; i < delay; i++) {
			int pos = (writePos[v] - delay + i + 1024) & 1023;
			buf[v][pos] = random::uniform() * 2.0f - 1.0f;
		}
		filterState[v] = 0.0f;
		triggered[v] = true;
	}

	AudioSynthWaveformDc dc1; // dummy for getStream()

	audio_block_t outputBlock;

	float buf[6][1024] = {};
	int writePos[6] = {};
	float filterState[6] = {};
	bool triggered[6] = {};
	float damping = 0.5f;
	float baseDelay = 100.0f;
};

REGISTER_PLUGIN(PluckCloud); // this is important, so that we can include the plugin in a bank
