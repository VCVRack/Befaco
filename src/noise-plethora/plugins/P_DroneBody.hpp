#pragma once

#include "NoisePlethoraPlugin.hpp"

class DroneBody : public NoisePlethoraPlugin {

public:

	DroneBody()
	// : patchCord1(sine_fm1, 0, mixer1, 0)
	// , patchCord2(sine_fm2, 0, mixer1, 1)
	// , patchCord3(mixer1, 0, wavefolder1, 0)
	// , patchCord4(dc1, 0, wavefolder1, 1)
	{ }

	~DroneBody() override {}

	// delete copy constructors
	DroneBody(const DroneBody&) = delete;
	DroneBody& operator=(const DroneBody&) = delete;

	void init() override {
		sine_fm1.frequency(100);
		sine_fm1.amplitude(1.0);

		sine_fm2.frequency(101);
		sine_fm2.amplitude(1.0);

		dc1.amplitude(0.3);

		mixer1.gain(0, 0.5);
		mixer1.gain(1, 0.5);

		// Zero out the feedback blocks
		prevSine1Block.zeroAudioBlock();
		prevSine2Block.zeroAudioBlock();
	}

	void process(float k1, float k2) override {
		float freq = 20.0f + pow(k1, 2) * 500.0f;
		float detune_ratio = 1.0f + k2 * 0.05f;
		float dc_fold = 0.1f + k2 * 0.5f;

		sine_fm1.frequency(freq);
		sine_fm2.frequency(freq * detune_ratio);
		dc1.amplitude(dc_fold);
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {
		// Cross-modulation: each sine is modulated by the previous block's output of the other
		sine_fm1.update(&prevSine2Block, &sine1Block);
		sine_fm2.update(&prevSine1Block, &sine2Block);

		// Save copies for next block's cross-feedback
		memcpy(prevSine1Block.data, sine1Block.data, sizeof(int16_t) * AUDIO_BLOCK_SAMPLES);
		memcpy(prevSine2Block.data, sine2Block.data, sizeof(int16_t) * AUDIO_BLOCK_SAMPLES);

		// Mix both sines
		mixer1.update(&sine1Block, &sine2Block, nullptr, nullptr, &mixBlock);

		// Wavefold with DC bias
		dc1.update(&dcBlock);
		wavefolder1.update(&mixBlock, &dcBlock, &wfBlock);

		blockBuffer.pushBuffer(wfBlock.data, AUDIO_BLOCK_SAMPLES);
	}

	AudioStream& getStream() override {
		return dc1;
	}
	unsigned char getPort() override {
		return 0;
	}

private:
	audio_block_t sine1Block, sine2Block, mixBlock, dcBlock, wfBlock;
	audio_block_t prevSine1Block, prevSine2Block;

	AudioSynthWaveformSineModulated sine_fm1;
	AudioSynthWaveformSineModulated sine_fm2;
	AudioSynthWaveformDc     dc1;
	AudioEffectWaveFolder    wavefolder1;
	AudioMixer4              mixer1;
};

REGISTER_PLUGIN(DroneBody);
