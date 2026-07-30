#pragma once

#include "NoisePlethoraPlugin.hpp"

class RunawayFilter : public NoisePlethoraPlugin {

public:

	RunawayFilter() { }

	~RunawayFilter() override {}

	RunawayFilter(const RunawayFilter&) = delete;
	RunawayFilter& operator=(const RunawayFilter&) = delete;

	void init() override {
		noise1.amplitude(0.5f);
		filter1.frequency(1000.0f);
		filter1.resonance(4.8f);
		filter1.octaveControl(3.0f);
	}

	void process(float k1, float k2) override {
		float freq = 30.0f + std::pow(k1, 2.0f) * 10000.0f;
		float noiseAmp = 0.01f + k2 * 0.99f;
		float resonance = 4.5f + (1.0f - k2) * 0.49f;

		noise1.amplitude(noiseAmp);
		filter1.frequency(freq);
		filter1.resonance(resonance);
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {
		noise1.update(&noiseBlock);
		filter1.update(&noiseBlock, nullptr, &lpBlock, &bpBlock, &hpBlock);

		blockBuffer.pushBuffer(bpBlock.data, AUDIO_BLOCK_SAMPLES);
	}

	AudioStream& getStream() override {
		return filter1;
	}
	unsigned char getPort() override {
		return 0;
	}

private:
	AudioSynthNoiseWhite noise1;
	AudioFilterStateVariable filter1;

	audio_block_t noiseBlock;
	audio_block_t lpBlock, bpBlock, hpBlock;
};

REGISTER_PLUGIN(RunawayFilter);
