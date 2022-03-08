#pragma once

#include "NoisePlethoraPlugin.hpp"

class radioOhNo : public NoisePlethoraPlugin {

public:

	radioOhNo()
		//: patchCord1(dc1, 0, waveformMod1, 1),
		//  patchCord2(dc1, 0, waveformMod2, 1),
		//  patchCord3(dc1, 0, waveformMod3, 1),
		//  patchCord4(dc1, 0, waveformMod4, 1),
		//  patchCord5(waveformMod2, 0, waveformMod1, 0),
		//  patchCord6(waveformMod1, 0, waveformMod2, 0),
		//  patchCord7(waveformMod1, 0, mixer1, 0),
		//  patchCord8(waveformMod3, 0, waveformMod4, 0),
		//  patchCord9(waveformMod3, 0, mixer1, 1),
		//  patchCord10(waveformMod4, 0, waveformMod3, 0)
	{ }

	~radioOhNo() override {}

	radioOhNo(const radioOhNo&) = delete;
	radioOhNo& operator=(const radioOhNo&) = delete;

	void init() override {
		waveformMod1.begin(0.8, 500, WAVEFORM_PULSE);
		waveformMod2.begin(0.8, 1500, WAVEFORM_PULSE);
		waveformMod3.begin(0.8, 600, WAVEFORM_PULSE);
		waveformMod4.begin(0.8, 1600, WAVEFORM_PULSE);
	}

	void process(float k1, float k2) override {
		float knob_1 = k1;
		float knob_2 = k2;
		float pitch1 = pow(knob_1, 2);
		// float pitch2 = pow(knob_2, 2);


		waveformMod1.frequency(2500 * pitch1 + 20);
		waveformMod2.frequency(1120 - (1100 * pitch1));
		waveformMod3.frequency(2900 * pitch1 + 20);
		waveformMod4.frequency(8020 - (8000 * pitch1 + 20));
		waveformMod1.frequencyModulation(5);
		waveformMod2.frequencyModulation(5);
		waveformMod1.frequencyModulation(5);
		waveformMod2.frequencyModulation(5);
		dc1.amplitude(knob_2);

	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {
		dc1.update(&dcPrevious);

		waveformMod2.update(&waveformModPrevious[1], &dcPrevious, &waveformModPrevious[2]);
		waveformMod1.update(&waveformModPrevious[2], &dcPrevious, &waveformModPrevious[1]);

		waveformMod3.update(&waveformModPrevious[4], &dcPrevious, &waveformModPrevious[3]);
		waveformMod4.update(&waveformModPrevious[3], &dcPrevious, &waveformModPrevious[4]);

		mixer1.update(&waveformModPrevious[1], &waveformModPrevious[3], nullptr, nullptr, &mixerOut);

		blockBuffer.pushBuffer(mixerOut.data, AUDIO_BLOCK_SAMPLES);

	}

	AudioStream& getStream() override {
		return mixer1;
	}
	unsigned char getPort() override {
		return 0;
	}

private:

	audio_block_t waveformModPrevious[5];
	audio_block_t dcPrevious;
	audio_block_t mixerOut;

	AudioSynthWaveformDc     dc1;            //xy=179.75,710.75
	AudioSynthWaveformModulated waveformMod2; //xy=464.75,614.75
	AudioSynthWaveformModulated waveformMod1;   //xy=466.75,490.75
	AudioSynthWaveformModulated waveformMod3; //xy=468.75,729.75
	AudioSynthWaveformModulated waveformMod4; //xy=470.75,848.75
	AudioMixer4              mixer1;         //xy=676.750057220459,680.7500286102295
	// AudioConnection          patchCord1;
	// AudioConnection          patchCord2;
	// AudioConnection          patchCord3;
	// AudioConnection          patchCord4;
	// AudioConnection          patchCord5;
	// AudioConnection          patchCord6;
	// AudioConnection          patchCord7;
	// AudioConnection          patchCord8;
	// AudioConnection          patchCord9;
	// AudioConnection          patchCord10;
};

REGISTER_PLUGIN(radioOhNo);
