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

		WaveformType masterWaveform = WAVEFORM_SINE;
		float masterVolume = 0.2;

		waveform4_1.begin(simd::float_4(masterVolume), simd::float_4(794, 647, 524, 444), masterWaveform);
		waveform4_2.begin(simd::float_4(masterVolume, masterVolume, 0, 0), simd::float_4(368, 283, 0, 0), masterWaveform);		
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

		waveform4_1.frequency(simd::float_4(f1, f2, f3, f4));
		waveform4_2.frequency(simd::float_4(f5, f6, 0.f, 0.f));
	}

	float processGraph(float sampleTime) override {
		float modSignal = modulator.process(sampleTime);

		altOutput = modSignal;

		float mix1 = mixer1.process(waveform4_1.process(sampleTime, modSignal));
		float mix2 = mixer2.process(waveform4_2.process(sampleTime, modSignal));

		return mixer5.process(mix1, mix2, 0.f, 0.f);
	}

	AudioStream& getStream() override { return mixer5; }
	unsigned char getPort() override { return 0; }

private:
	AudioSynthWaveformFloat     modulator;      //xy=135.88888549804688,405.55555725097656
	AudioSynthWaveformModulatedFloat4 waveform4_1; 
	AudioSynthWaveformModulatedFloat4 waveform4_2; 

	// AudioSynthWaveformModulatedFloat waveform6;      //xy=453.8888854980469,543.6666889190674
	// AudioSynthWaveformModulatedFloat waveform5;      //xy=454.8888854980469,501.6666889190674
	// AudioSynthWaveformModulatedFloat waveform4;      //xy=459.8888854980469,433.6666889190674
	// AudioSynthWaveformModulatedFloat waveform2;      //xy=461.8888854980469,328.6666889190674
	// AudioSynthWaveformModulatedFloat waveform3;      //xy=461.8888854980469,373.6666889190674
	// AudioSynthWaveformModulatedFloat waveform1;      //xy=462.8888854980469,285.6666889190674
	AudioMixer4Float              mixer2;         //xy=678.8888778686523,495.66669845581055
	AudioMixer4Float              mixer1;         //xy=679.8888854980469,377.6666889190674
	AudioMixer4Float              mixer5;         //xy=883.8888854980469,446.66675567626953
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
