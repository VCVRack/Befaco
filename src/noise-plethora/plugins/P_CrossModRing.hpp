#pragma once

#include "NoisePlethoraPlugin.hpp"

class CrossModRing : public NoisePlethoraPlugin {

public:

	CrossModRing()
	// :patchCord1(waveformMod3, 0, multiply2, 0),
	//  patchCord2(waveformMod3, 0, waveformMod4, 0),
	//  patchCord3(waveformMod1, 0, multiply1, 0),
	//  patchCord4(waveformMod1, 0, waveformMod2, 0),
	//  patchCord5(waveformMod4, 0, multiply2, 1),
	//  patchCord6(waveformMod4, 0, waveformMod1, 0),
	//  patchCord7(waveformMod2, 0, multiply1, 1),
	//  patchCord8(waveformMod2, 0, waveformMod3, 0),
	//  patchCord9(multiply2, 0, multiply3, 1),
	//  patchCord10(multiply1, 0, multiply3, 0)
	{ }

	~CrossModRing() override {}

	CrossModRing(const CrossModRing&) = delete;
	CrossModRing& operator=(const CrossModRing&) = delete;

	void init() override {
		WaveformType masterWaveform = WAVEFORM_SQUARE;
		float masterVolume = 0.8;

		waveformMod1.begin(masterWaveform);
		waveformMod2.begin(masterWaveform);
		waveformMod3.begin(WAVEFORM_SAWTOOTH);
		waveformMod3.offset(1);

		waveformMod4.begin(masterWaveform);

		waveformMod1.amplitude(masterVolume);
		waveformMod2.amplitude(masterVolume);
		waveformMod3.amplitude(masterVolume);
		waveformMod4.amplitude(masterVolume);


		waveformMod1.frequency(1100);
		waveformMod2.frequency(1367);
		waveformMod3.frequency(345);
		waveformMod4.frequency(686);

	}

	void process(float k1, float k2) override {
		float knob_1 = k1;
		float knob_2 = k2;
		float pitch1 = pow(knob_1, 2);

		waveformMod1.frequency(20 + (pitch1 * 807));
		waveformMod2.frequency(11 + (knob_1 * 21));
		waveformMod3.frequency(1 + (pitch1 * 29));
		waveformMod4.frequency(1 + (knob_1 * 7) * -1);

		float masterOctaves = 8 * knob_2 + 2;

		waveformMod1.frequencyModulation(masterOctaves);
		waveformMod2.frequencyModulation(masterOctaves);
		waveformMod3.frequencyModulation(masterOctaves);
		waveformMod4.frequencyModulation(masterOctaves);

	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {
		// NOTE: buffer is zero indexed, waveformMod names are not!!
		// NOTE: each waveform is modulated by previous
		waveformMod3.update(&waveformModOut[1], nullptr, &waveformModOut[2]);
		waveformMod1.update(&waveformModOut[3], nullptr, &waveformModOut[0]);
		waveformMod4.update(&waveformModOut[2], nullptr, &waveformModOut[3]);
		waveformMod2.update(&waveformModOut[0], nullptr, &waveformModOut[1]);

		// multiply
		multiply1.update(&waveformModOut[0], &waveformModOut[1], &multiplyOut[0]);
		multiply2.update(&waveformModOut[2], &waveformModOut[3], &multiplyOut[1]);
		multiply3.update(&multiplyOut[0], &multiplyOut[1], &multiplyOut[2]);

		blockBuffer.pushBuffer(multiplyOut[2].data, AUDIO_BLOCK_SAMPLES);
	}

	AudioStream& getStream() override {
		return multiply3;
	}
	unsigned char getPort() override {
		return 0;
	}

private:

	audio_block_t waveformModOut[4] = {}, multiplyOut[3] = {};

	AudioSynthWaveformModulated waveformMod3;   //xy=287.99999618530273,420
	AudioSynthWaveformModulated waveformMod1;   //xy=288.99999618530273,197
	AudioSynthWaveformModulated waveformMod4;   //xy=290.99999618530273,541
	AudioSynthWaveformModulated waveformMod2;   //xy=291.99999618530273,318
	AudioEffectMultiply      multiply2;      //xy=484.99999618530273,474
	AudioEffectMultiply      multiply1;      //xy=485.99999618530273,251
	AudioEffectMultiply      multiply3;      //xy=655.9999961853027,356


	// AudioConnection          patchCord1(waveformMod3, 0, multiply2, 0);
	// AudioConnection          patchCord2(waveformMod3, 0, waveformMod4, 0);
	// AudioConnection          patchCord3(waveformMod1, 0, multiply1, 0);
	// AudioConnection          patchCord4(waveformMod1, 0, waveformMod2, 0);
	// AudioConnection          patchCord5(waveformMod4, 0, multiply2, 1);
	// AudioConnection          patchCord6(waveformMod4, 0, waveformMod1, 0);
	// AudioConnection          patchCord7(waveformMod2, 0, multiply1, 1);
	// AudioConnection          patchCord8(waveformMod2, 0, waveformMod3, 0);
	// AudioConnection          patchCord9(multiply2, 0, multiply3, 1);
	// AudioConnection          patchCord10(multiply1, 0, multiply3, 0);

};

REGISTER_PLUGIN(CrossModRing);
