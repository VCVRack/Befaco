#pragma once

#include "NoisePlethoraPlugin.hpp"
#include <cmath>

class MetallicNoise : public NoisePlethoraPlugin {

public:

	MetallicNoise() { }

	~MetallicNoise() override {}

	MetallicNoise(const MetallicNoise&) = delete;
	MetallicNoise& operator=(const MetallicNoise&) = delete;

	void init() override {
		for (int i = 0; i < 6; i++) {
			osc[i].begin(0.15f, baseFreqs[i], WAVEFORM_SQUARE);  // 6*0.15=0.9 max sum, no clipping
		}

		mixer1.gain(0, 1.0f);
		mixer1.gain(1, 1.0f);
		mixer1.gain(2, 1.0f);
		mixer1.gain(3, 1.0f);

		mixer2.gain(0, 1.0f);
		mixer2.gain(1, 1.0f);
		mixer2.gain(2, 0.0f);
		mixer2.gain(3, 0.0f);

		mixer3.gain(0, 1.0f);
		mixer3.gain(1, 1.0f);
		mixer3.gain(2, 0.0f);
		mixer3.gain(3, 0.0f);

		filter1.frequency(5000);
		filter1.resonance(0.7f);
		filter1.octaveControl(2.0f);
	}

	void process(float k1, float k2) override {
		float pitchMult = 0.5f + k1 * 2.0f;
		for (int i = 0; i < 6; i++) {
			osc[i].frequency(baseFreqs[i] * pitchMult);
		}

		float filterFreq = 500.0f + std::pow(k2, 2) * 15000.0f;
		filter1.frequency(filterFreq);
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {
		for (int i = 0; i < 6; i++) {
			osc[i].update(nullptr, nullptr, &oscBlock[i]);
		}

		mixer1.update(&oscBlock[0], &oscBlock[1], &oscBlock[2], &oscBlock[3], &mixerBlock[0]);
		mixer2.update(&oscBlock[4], &oscBlock[5], nullptr, nullptr, &mixerBlock[1]);
		mixer3.update(&mixerBlock[0], &mixerBlock[1], nullptr, nullptr, &mixerBlock[2]);

		filter1.update(&mixerBlock[2], nullptr, &filterLP, &filterBP, &filterHP);

		blockBuffer.pushBuffer(filterHP.data, AUDIO_BLOCK_SAMPLES);
	}

	AudioStream& getStream() override {
		return filter1;
	}
	unsigned char getPort() override {
		return 0;
	}

private:
	static constexpr float baseFreqs[6] = {205.3f, 304.4f, 369.6f, 522.7f, 800.6f, 1053.4f};

	AudioSynthWaveformModulated osc[6];
	AudioMixer4 mixer1;
	AudioMixer4 mixer2;
	AudioMixer4 mixer3;
	AudioFilterStateVariable filter1;

	audio_block_t oscBlock[6];
	audio_block_t mixerBlock[3];
	audio_block_t filterLP, filterBP, filterHP;
};

REGISTER_PLUGIN(MetallicNoise);
