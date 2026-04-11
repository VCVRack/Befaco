#pragma once

#include "NoisePlethoraPlugin.hpp"

class StochasticPulse : public NoisePlethoraPlugin {

public:

	StochasticPulse() { }

	~StochasticPulse() override {}

	StochasticPulse(const StochasticPulse&) = delete;
	StochasticPulse& operator=(const StochasticPulse&) = delete;

	void init() override {
		density = 50.0f;
		lastPulseSign = false;

		filter1.frequency(1000);
		filter1.resonance(4.5f);
		filter1.octaveControl(1.0f);
	}

	void process(float k1, float k2) override {
		density = 5.0f + pow(k1, 2) * 500.0f;
		float filterFreq = 80.0f + pow(k2, 2) * 8000.0f;
		filter1.frequency(filterFreq);
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {

		// Generate stochastic impulses
		float probability = density / 44100.0f;

		for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
			if (random::uniform() < probability) {
				lastPulseSign = !lastPulseSign;
				impulseBlock.data[i] = lastPulseSign ? 32767 : -32767;
			}
			else {
				impulseBlock.data[i] = 0;
			}
		}

		// Filter the impulses - use bandpass output for resonant pings
		filter1.update(&impulseBlock, nullptr, &lpBlock, &bpBlock, &hpBlock);

		blockBuffer.pushBuffer(bpBlock.data, AUDIO_BLOCK_SAMPLES);
	}

	AudioStream& getStream() override {
		return filter1;
	}
	unsigned char getPort() override {
		return 0;
	}

private:
	AudioFilterStateVariable filter1;

	audio_block_t impulseBlock, lpBlock, bpBlock, hpBlock;

	float density = 50.0f;
	bool lastPulseSign = false;
};

REGISTER_PLUGIN(StochasticPulse);
