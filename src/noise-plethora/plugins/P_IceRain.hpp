#pragma once

#include "NoisePlethoraPlugin.hpp"

class IceRain : public NoisePlethoraPlugin {

public:

	IceRain()
	// : patchCord1(waveformMod1, freeverb1)
	// , patchCord2(waveformMod1, 0, mixer1, 0)
	// , patchCord3(freeverb1, 0, mixer1, 1)
	{ }

	~IceRain() override {}

	// delete copy constructors
	IceRain(const IceRain&) = delete;
	IceRain& operator=(const IceRain&) = delete;

	void init() override {
		waveformMod1.begin(1.0, 5, WAVEFORM_SAMPLE_HOLD);
		waveformMod1.frequencyModulation(10);

		freeverb1.roomsize(0.8);
		freeverb1.damping(0.8);

		mixer1.gain(0, 0.3);
		mixer1.gain(1, 1.0);
	}

	void process(float k1, float k2) override {
		float freq = 0.5f + pow(k1, 2) * 200.0f;
		float roomsize = 0.3f + k2 * 0.7f;

		waveformMod1.frequency(freq);
		freeverb1.roomsize(roomsize);

		mixer1.gain(0, 0.3f * (1.0f - k2));
		mixer1.gain(1, k2 * 2.0f);
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {
		waveformMod1.update(nullptr, nullptr, &shBlock);
		freeverb1.update(&shBlock, &reverbBlock);
		mixer1.update(&shBlock, &reverbBlock, nullptr, nullptr, &mixBlock);

		blockBuffer.pushBuffer(mixBlock.data, AUDIO_BLOCK_SAMPLES);
	}

	AudioStream& getStream() override {
		return mixer1;
	}
	unsigned char getPort() override {
		return 0;
	}

private:
	audio_block_t shBlock, reverbBlock, mixBlock;

	AudioSynthWaveformModulated waveformMod1;
	AudioEffectFreeverb      freeverb1;
	AudioMixer4              mixer1;
};

REGISTER_PLUGIN(IceRain);
