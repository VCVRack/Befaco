#pragma once

#include "NoisePlethoraPlugin.hpp"

class crCluster2 : public NoisePlethoraPlugin {

public:

	crCluster2() // :
	// patchCord1(modulator, 0, waveform1, 0),
	// patchCord2(modulator, 0, waveform2, 0),
	// patchCord3(modulator, 0, waveform3, 0),
	// patchCord4(modulator, 0, waveform4, 0),
	// patchCord5(modulator, 0, waveform5, 0),
	// patchCord6(modulator, 0, waveform6, 0),
	// patchCord7(waveform6, 0, mixer2, 1),
	// patchCord8(waveform5, 0, mixer2, 0),
	// patchCord9(waveform4, 0, mixer1, 3),
	// patchCord10(waveform2, 0, mixer1, 1),
	// patchCord11(waveform3, 0, mixer1, 2),
	// patchCord12(waveform1, 0, mixer1, 0),
	// patchCord13(mixer2, 0, mixer5, 1),
	// patchCord14(mixer1, 0, mixer5, 0)
	{ }

	~crCluster2() override {}

	crCluster2(const crCluster2&) = delete;
	crCluster2& operator=(const crCluster2&) = delete;

	void init() override {
		mixer1.gain(0, 1);
		mixer1.gain(1, 1);
		mixer1.gain(2, 1);
		mixer1.gain(3, 1);
		mixer2.gain(0, 1);
		mixer2.gain(1, 1);
		mixer2.gain(2, 1);
		mixer2.gain(3, 1);
		mixer5.gain(0, 1);
		mixer5.gain(1, 1);
		mixer5.gain(2, 1);
		mixer5.gain(3, 1);

		int masterWaveform = WAVEFORM_SINE;
		float masterVolume = 0.2;

		waveform1.begin(masterVolume, 794, masterWaveform);
		waveform2.begin(masterVolume, 647, masterWaveform);
		waveform3.begin(masterVolume, 524, masterWaveform);
		waveform4.begin(masterVolume, 444, masterWaveform);
		waveform5.begin(masterVolume, 368, masterWaveform);
		waveform6.begin(masterVolume, 283, masterWaveform);

		modulator.begin(1, 1000, WAVEFORM_SINE);
	}

	void process(float k1, float k2) override {
		float knob_1 = k1;
		float knob_2 = k2;
		float pitch1 = pow(knob_1, 2);

		float f1 = 40 + pitch1 * 8000;
		float f2 = f1 * 1.227;
		float f3 = f2 * 1.24;
		float f4 = f3 * 1.17;
		float f5 = f4 * 1.2;
		float f6 = f5 * 1.3;

		modulator.amplitude(knob_2);
		modulator.frequency(f1 * 2.7);

		waveform1.frequency(f1);
		waveform2.frequency(f2);
		waveform3.frequency(f3);
		waveform4.frequency(f4);
		waveform5.frequency(f5);
		waveform6.frequency(f6);
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {
		modulator.update(&waveformOut);

		// FM from modulator for the 6 oscillators
		waveform1.update(&waveformOut, nullptr, &waveformModOut[0]);
		waveform2.update(&waveformOut, nullptr, &waveformModOut[1]);
		waveform3.update(&waveformOut, nullptr, &waveformModOut[2]);
		waveform4.update(&waveformOut, nullptr, &waveformModOut[3]);
		waveform5.update(&waveformOut, nullptr, &waveformModOut[4]);
		waveform6.update(&waveformOut, nullptr, &waveformModOut[5]);

		mixer1.update(&waveformModOut[0], &waveformModOut[1], &waveformModOut[2], &waveformModOut[3], &mixerOut[0]);
		mixer2.update(&waveformModOut[4], &waveformModOut[5], nullptr, nullptr, &mixerOut[1]);
		mixer5.update(&mixerOut[0], &mixerOut[1], nullptr, nullptr, &mixerOut[2]);

		blockBuffer.pushBuffer(mixerOut[2].data, AUDIO_BLOCK_SAMPLES);
	}

	AudioStream& getStream() override {
		return mixer5;
	}
	unsigned char getPort() override {
		return 0;
	}

private:
	audio_block_t waveformOut, waveformModOut[6] = {}, mixerOut[3] = {};

	AudioSynthWaveform       modulator;      //xy=135.88888549804688,405.55555725097656
	AudioSynthWaveformModulated waveform6;      //xy=453.8888854980469,543.6666889190674
	AudioSynthWaveformModulated waveform5;      //xy=454.8888854980469,501.6666889190674
	AudioSynthWaveformModulated waveform4;      //xy=459.8888854980469,433.6666889190674
	AudioSynthWaveformModulated waveform2;      //xy=461.8888854980469,328.6666889190674
	AudioSynthWaveformModulated waveform3;      //xy=461.8888854980469,373.6666889190674
	AudioSynthWaveformModulated waveform1;      //xy=462.8888854980469,285.6666889190674
	AudioMixer4              mixer2;         //xy=678.8888778686523,495.66669845581055
	AudioMixer4              mixer1;         //xy=679.8888854980469,377.6666889190674
	AudioMixer4              mixer5;         //xy=883.8888854980469,446.66675567626953
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
	// AudioConnection          patchCord11;
	// AudioConnection          patchCord12;
	// AudioConnection          patchCord13;
	// AudioConnection          patchCord14;
};

REGISTER_PLUGIN(crCluster2);
