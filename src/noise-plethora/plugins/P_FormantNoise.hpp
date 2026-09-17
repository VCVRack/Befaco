#pragma once

#include "NoisePlethoraPlugin.hpp"

class FormantNoise : public NoisePlethoraPlugin {

public:

	FormantNoise() { }

	~FormantNoise() override {}

	// delete copy constructors
	FormantNoise(const FormantNoise&) = delete;
	FormantNoise& operator=(const FormantNoise&) = delete;

	void init() override {
		noise1.amplitude(1);

		filter1.resonance(2.0f);
		filter1.octaveControl(0);
		filter2.resonance(2.0f);
		filter2.octaveControl(0);
		filter3.resonance(2.0f);
		filter3.octaveControl(0);

		mixer1.gain(0, 0.5f);
		mixer1.gain(1, 0.5f);
		mixer1.gain(2, 0.5f);
		mixer1.gain(3, 0);
	}

	void process(float k1, float k2) override {
		// Vowel formant table (Hz):
		// A=[730,1090,2440], E=[660,1720,2410], I=[270,2290,3010],
		// O=[570,840,2410],  U=[300,870,2240]
		static const float formants[5][3] = {
			{730.0f,  1090.0f, 2440.0f},  // A
			{660.0f,  1720.0f, 2410.0f},  // E
			{270.0f,  2290.0f, 3010.0f},  // I
			{570.0f,  840.0f,  2410.0f},  // O
			{300.0f,  870.0f,  2240.0f}   // U
		};

		// continuous vowel morph: 0=A, 0.25=E, 0.5=I, 0.75=O, 1.0=U
		float pos = k1 * 4.0f; // 0..4
		int idx = (int)pos;
		if (idx < 0) idx = 0;
		if (idx > 3) idx = 3;
		float frac = pos - (float)idx;
		if (frac < 0.0f) frac = 0.0f;
		if (frac > 1.0f) frac = 1.0f;

		// interpolate F1, F2, F3 between adjacent vowels
		float f1 = formants[idx][0] + frac * (formants[idx + 1][0] - formants[idx][0]);
		float f2 = formants[idx][1] + frac * (formants[idx + 1][1] - formants[idx][1]);
		float f3 = formants[idx][2] + frac * (formants[idx + 1][2] - formants[idx][2]);

		filter1.frequency(f1);
		filter2.frequency(f2);
		filter3.frequency(f3);

		// resonance: q = 1.0 + k2 * 4.0 (range 1-5)
		float q = 1.0f + k2 * 4.0f;
		filter1.resonance(q);
		filter2.resonance(q);
		filter3.resonance(q);
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {
		// generate noise
		noise1.update(&noiseBlock);

		// process through 3 bandpass filters (use bandpass output)
		filter1.update(&noiseBlock, nullptr, &filterLP1, &filterBP1, &filterHP1);
		filter2.update(&noiseBlock, nullptr, &filterLP2, &filterBP2, &filterHP2);
		filter3.update(&noiseBlock, nullptr, &filterLP3, &filterBP3, &filterHP3);

		// mix the 3 bandpass outputs
		mixer1.update(&filterBP1, &filterBP2, &filterBP3, nullptr, &mixerOut);

		blockBuffer.pushBuffer(mixerOut.data, AUDIO_BLOCK_SAMPLES);
	}

	AudioStream& getStream() override {
		return mixer1;
	}
	unsigned char getPort() override {
		return 0;
	}

private:
	AudioSynthNoiseWhite noise1;
	AudioFilterStateVariable filter1;
	AudioFilterStateVariable filter2;
	AudioFilterStateVariable filter3;
	AudioMixer4 mixer1;

	audio_block_t noiseBlock;
	audio_block_t filterLP1, filterBP1, filterHP1;
	audio_block_t filterLP2, filterBP2, filterHP2;
	audio_block_t filterLP3, filterBP3, filterHP3;
	audio_block_t mixerOut;
};

REGISTER_PLUGIN(FormantNoise); // this is important, so that we can include the plugin in a bank
