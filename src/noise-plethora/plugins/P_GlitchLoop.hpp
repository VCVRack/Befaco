#pragma once

#include "NoisePlethoraPlugin.hpp"

#define GLITCHLOOP_MAX_SAMPLES 8820

class GlitchLoop : public NoisePlethoraPlugin {

public:

	GlitchLoop() { }

	~GlitchLoop() override {}

	GlitchLoop(const GlitchLoop&) = delete;
	GlitchLoop& operator=(const GlitchLoop&) = delete;

	void init() override {
		currentLoopLen = GLITCHLOOP_MAX_SAMPLES;
		loopPos = 0;
		bitDepthAccum = 16.0f;
		bitsToLose = 0.0f;

		// Fill buffer with white noise
		for (int i = 0; i < GLITCHLOOP_MAX_SAMPLES; i++) {
			loopBuffer[i] = random::uniform() * 2.0f - 1.0f;
		}
	}

	void process(float k1, float k2) override {
		currentLoopLen = (int)(441.0f + std::pow(k1, 2.0f) * 8379.0f);
		if (currentLoopLen > GLITCHLOOP_MAX_SAMPLES) {
			currentLoopLen = GLITCHLOOP_MAX_SAMPLES;
		}
		if (currentLoopLen < 1) {
			currentLoopLen = 1;
		}

		bitsToLose = k2 * 0.05f;
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {

		for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
			float sample = loopBuffer[loopPos];

			// Apply bit depth reduction
			int effectiveBits = (int)bitDepthAccum;
			if (effectiveBits < 1) effectiveBits = 1;
			if (effectiveBits > 16) effectiveBits = 16;

			// Bit crush: quantize to fewer bits
			int shift = 16 - effectiveBits;
			if (shift > 0) {
				// Convert to int16 range, shift right then left to lose bits
				int32_t intSample = (int32_t)(sample * 32767.0f);
				intSample = (intSample >> shift) << shift;
				sample = (float)intSample / 32767.0f;
			}

			// Write crushed sample back into buffer for progressive degradation
			loopBuffer[loopPos] = sample;

			int32_t out = (int32_t)(sample * 32767.0f);
			if (out > 32767) out = 32767;
			if (out < -32767) out = -32767;
			outputBlock.data[i] = (int16_t)out;

			loopPos++;
			if (loopPos >= currentLoopLen) {
				loopPos = 0;
				bitDepthAccum -= bitsToLose;

				// When bit depth is exhausted, refill with fresh noise
				if (bitDepthAccum < 1.0f) {
					bitDepthAccum = 16.0f;
					for (int j = 0; j < GLITCHLOOP_MAX_SAMPLES; j++) {
						loopBuffer[j] = random::uniform() * 2.0f - 1.0f;
					}
				}
			}
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

	float loopBuffer[GLITCHLOOP_MAX_SAMPLES];
	int loopPos = 0;
	int currentLoopLen = GLITCHLOOP_MAX_SAMPLES;
	float bitDepthAccum = 16.0f;
	float bitsToLose = 0.0f;
};

REGISTER_PLUGIN(GlitchLoop);
