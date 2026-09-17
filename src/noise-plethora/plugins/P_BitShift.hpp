#pragma once

#include "NoisePlethoraPlugin.hpp"

class BitShift : public NoisePlethoraPlugin {

public:

	BitShift() { }

	~BitShift() override {}

	BitShift(const BitShift&) = delete;
	BitShift& operator=(const BitShift&) = delete;

	void init() override {
		// sawtooths produce diverse bit patterns — XOR creates rich digital artifacts
		// (square waves only have 2 values, XOR of ±32767 produces near-silence)
		waveform1.begin(1.0f, 100, WAVEFORM_SAWTOOTH);
		waveform2.begin(1.0f, 200, WAVEFORM_SAWTOOTH);
		combine1.setCombineMode(1); // XOR
	}

	void process(float k1, float k2) override {
		float freq1 = 20.0f + pow(k1, 2) * 5000.0f;
		float freq2 = freq1 * (1.0f + k2 * 7.0f);
		waveform1.frequency(freq1);
		waveform2.frequency(freq2);
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {

		waveform1.update(nullptr, nullptr, &wf1Block);
		waveform2.update(nullptr, nullptr, &wf2Block);
		combine1.update(&wf1Block, &wf2Block, &combineBlock);

		blockBuffer.pushBuffer(combineBlock.data, AUDIO_BLOCK_SAMPLES);
	}

	AudioStream& getStream() override {
		return combine1;
	}
	unsigned char getPort() override {
		return 0;
	}

private:
	audio_block_t wf1Block, wf2Block, combineBlock;

	AudioSynthWaveformModulated waveform1;
	AudioSynthWaveformModulated waveform2;
	AudioEffectDigitalCombine combine1;
};

REGISTER_PLUGIN(BitShift);
