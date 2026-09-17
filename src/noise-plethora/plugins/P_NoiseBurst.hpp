#pragma once

#include "NoisePlethoraPlugin.hpp"
#include <cmath>

class NoiseBurst : public NoisePlethoraPlugin {

public:

	NoiseBurst() { }

	~NoiseBurst() override {}

	NoiseBurst(const NoiseBurst&) = delete;
	NoiseBurst& operator=(const NoiseBurst&) = delete;

	void init() override {
		noise1.amplitude(1.0f);
		burstCounter = 0;
		currentBurstLen = 200;
		burstProb = 0.001f;
	}

	void process(float k1, float k2) override {
		burstProb = 0.00002f + std::pow(k1, 2) * 0.003f;
		currentBurstLen = (int)(44 + k2 * 4400);
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {
		noise1.update(&noiseBlock);

		for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
			if (burstCounter > 0) {
				// Envelope fade-out on last 20 samples
				float gain = 1.0f;
				if (burstCounter < 20) {
					gain = burstCounter / 20.0f;
				}
				outputBlock.data[i] = (int16_t)(noiseBlock.data[i] * gain);
				burstCounter--;
			}
			else {
				if (random::uniform() < burstProb) {
					burstCounter = currentBurstLen;
					// Output this sample as part of the burst
					float gain = 1.0f;
					if (burstCounter < 20) {
						gain = burstCounter / 20.0f;
					}
					outputBlock.data[i] = (int16_t)(noiseBlock.data[i] * gain);
					burstCounter--;
				}
				else {
					outputBlock.data[i] = 0;
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
	AudioSynthNoiseWhite noise1;

	audio_block_t noiseBlock, outputBlock;

	int burstCounter = 0;
	int currentBurstLen = 200;
	float burstProb = 0.001f;
};

REGISTER_PLUGIN(NoiseBurst);
