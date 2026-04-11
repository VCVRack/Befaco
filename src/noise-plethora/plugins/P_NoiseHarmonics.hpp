#pragma once

#include "NoisePlethoraPlugin.hpp"

class NoiseHarmonics : public NoisePlethoraPlugin {

public:

	NoiseHarmonics()
	// : patchCord1(noise1, 0, wavefolder1, 0)
	// , patchCord2(dc1, 0, wavefolder1, 1)
	// , patchCord3(wavefolder1, 0, filter1, 0)
	{ }

	~NoiseHarmonics() override {}

	// delete copy constructors
	NoiseHarmonics(const NoiseHarmonics&) = delete;
	NoiseHarmonics& operator=(const NoiseHarmonics&) = delete;

	void init() override {
		noise1.amplitude(1.0);
		dc1.amplitude(0.5);

		filter1.frequency(1000);
		filter1.resonance(1.5);       // lower Q = wider, grittier character
		filter1.octaveControl(2.0);
	}

	void process(float k1, float k2) override {
		float dcAmp = 0.1f + pow(k1, 2) * 1.5f;  // wider range, more aggressive folding
		float filterFreq = 60.0f + pow(k2, 2) * 8000.0f;

		dc1.amplitude(dcAmp);
		filter1.frequency(filterFreq);
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {
		noise1.update(&noiseBlock);
		dc1.update(&dcBlock);
		wavefolder1.update(&noiseBlock, &dcBlock, &wfBlock);
		filter1.update(&wfBlock, nullptr, &lpBlock, &bpBlock, &hpBlock);

		// lowpass instead of bandpass — lets all wavefolder harmonics through
		blockBuffer.pushBuffer(lpBlock.data, AUDIO_BLOCK_SAMPLES);
	}

	AudioStream& getStream() override {
		return filter1;
	}
	unsigned char getPort() override {
		return 0;
	}

private:
	audio_block_t noiseBlock, dcBlock, wfBlock;
	audio_block_t lpBlock, bpBlock, hpBlock;

	AudioSynthNoiseWhite     noise1;
	AudioSynthWaveformDc     dc1;
	AudioEffectWaveFolder    wavefolder1;
	AudioFilterStateVariable filter1;
};

REGISTER_PLUGIN(NoiseHarmonics);
