#pragma once

#include "NoisePlethoraPlugin.hpp"

class CellularSynth : public NoisePlethoraPlugin {

public:

	CellularSynth() { }

	~CellularSynth() override {}

	CellularSynth(const CellularSynth&) = delete;
	CellularSynth& operator=(const CellularSynth&) = delete;

	void init() override {
		// Initialize CA with single cell in center
		for (int i = 0; i < 32; i++) {
			cells[i] = false;
		}
		cells[16] = true;

		clockCounter = 0;
		clockSamples = 128;
		rule = 110;

		float fundamental = 55.0f;
		float masterVolume = 0.2f;

		for (int i = 0; i < 8; i++) {
			osc[i].begin(masterVolume, fundamental * (i + 1), WAVEFORM_SAWTOOTH);
			osc[i].frequencyModulation(0.1f);
		}

		// Mixer gains: each sub-mixer has 4 inputs
		for (int ch = 0; ch < 4; ch++) {
			mixerA.gain(ch, 0.5f);
			mixerB.gain(ch, 0.5f);
		}
		// Master mixer combines two sub-mixers
		masterMixer.gain(0, 0.7f);
		masterMixer.gain(1, 0.7f);
		masterMixer.gain(2, 0.0f);
		masterMixer.gain(3, 0.0f);

		// Set initial amplitudes from CA state
		updateOscAmplitudes();
	}

	void process(float k1, float k2) override {
		rule = (int)(k1 * 255.0f);
		clockSamples = (int)(128 + (1.0f - k2) * 128.0f * 50.0f);

		clockCounter += AUDIO_BLOCK_SAMPLES;
		if (clockCounter >= clockSamples) {
			evolveCA();
			updateOscAmplitudes();
			clockCounter = 0;
		}
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {

		// Update all 8 oscillators (no modulation input)
		for (int i = 0; i < 8; i++) {
			osc[i].update(nullptr, nullptr, &oscBlock[i]);
		}

		// Mix through hierarchy: 4 oscs per sub-mixer
		mixerA.update(&oscBlock[0], &oscBlock[1], &oscBlock[2], &oscBlock[3], &mixerABlock);
		mixerB.update(&oscBlock[4], &oscBlock[5], &oscBlock[6], &oscBlock[7], &mixerBBlock);

		// Master mix
		masterMixer.update(&mixerABlock, &mixerBBlock, nullptr, nullptr, &masterBlock);

		blockBuffer.pushBuffer(masterBlock.data, AUDIO_BLOCK_SAMPLES);
	}

	AudioStream& getStream() override {
		return masterMixer;
	}
	unsigned char getPort() override {
		return 0;
	}

private:

	void evolveCA() {
		bool newCells[32];
		for (int i = 0; i < 32; i++) {
			int left = (i - 1 + 32) % 32;
			int right = (i + 1) % 32;

			int pattern = (cells[left] ? 4 : 0)
			            | (cells[i]    ? 2 : 0)
			            | (cells[right] ? 1 : 0);

			newCells[i] = ((rule >> pattern) & 1) != 0;
		}
		for (int i = 0; i < 32; i++) {
			cells[i] = newCells[i];
		}
	}

	void updateOscAmplitudes() {
		// 32 cells grouped into 8 groups of 4
		for (int g = 0; g < 8; g++) {
			int count = 0;
			for (int c = 0; c < 4; c++) {
				if (cells[g * 4 + c]) count++;
			}
			float amp = count * 0.25f; // 0, 0.25, 0.5, 0.75, 1.0
			osc[g].amplitude(amp);
		}
	}

	bool cells[32] = {};
	int clockCounter = 0;
	int clockSamples = 128;
	int rule = 110;

	audio_block_t oscBlock[8] = {};
	audio_block_t mixerABlock, mixerBBlock, masterBlock;

	AudioSynthWaveformModulated osc[8];
	AudioMixer4 mixerA;
	AudioMixer4 mixerB;
	AudioMixer4 masterMixer;
};

REGISTER_PLUGIN(CellularSynth);
