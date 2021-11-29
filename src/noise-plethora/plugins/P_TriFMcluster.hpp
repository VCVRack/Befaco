#pragma once

#include "NoisePlethoraPlugin.hpp"

class TriFMcluster : public NoisePlethoraPlugin {

public:

	TriFMcluster()
	// : patchCord1(modulator1, 0, waveform1, 0),
	//   patchCord2(modulator3, 0, waveform3, 0),
	//   patchCord3(modulator2, 0, waveform2, 0),
	//   patchCord4(modulator5, 0, waveform5, 0),
	//   patchCord5(modulator4, 0, waveform4, 0),
	//   patchCord6(modulator6, 0, waveform6, 0),
	//   patchCord7(waveform1, 0, mixer1, 0),
	//   patchCord8(waveform4, 0, mixer1, 3),
	//   patchCord9(waveform2, 0, mixer1, 1),
	//   patchCord10(waveform3, 0, mixer1, 2),
	//   patchCord11(waveform6, 0, mixer2, 1),
	//   patchCord12(waveform5, 0, mixer2, 0),
	//   patchCord13(mixer2, 0, mixer5, 1),
	//   patchCord14(mixer1, 0, mixer5, 0)
	{ }

	~TriFMcluster() override {}

	TriFMcluster(const TriFMcluster&) = delete;
	TriFMcluster& operator=(const TriFMcluster&) = delete;

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

		WaveformType masterWaveform = WAVEFORM_TRIANGLE;
		float masterVolume = 0.25;

		waveform4_1.begin(simd::float_4(masterVolume), simd::float_4(794, 647, 524, 444), masterWaveform);
		waveform4_2.begin(simd::float_4(masterVolume, masterVolume, 0.f, 0.f), simd::float_4(368, 283, 0, 0), masterWaveform);

		modulator4_1.begin(1, 1000, WAVEFORM_SINE);
		modulator4_2.begin(1, 1000, WAVEFORM_SINE);
	}

	void process(float k1, float k2) override {
		float knob_1 = k1;
		float knob_2 = k2;
		float pitch1 = pow(knob_1, 2);


		float f1 = 300 + pitch1 * 8000;
		float f2 = f1 * 1.227;
		float f3 = f2 * 1.24;
		float f4 = f3 * 1.17;
		float f5 = f4 * 1.2;
		float f6 = f5 * 1.3;


		float index = knob_2 * 0.9 + 0.1;
		float indexFreq = 0.07;


		modulator4_1.amplitude(index);
		modulator4_2.amplitude(index);

		modulator4_1.frequency(simd::float_4(f1 * indexFreq, f2 * indexFreq, f3 * indexFreq, f4 * indexFreq));
		modulator4_2.frequency(simd::float_4(f5 * indexFreq, f6 * indexFreq, 0.f, 0.f));

		waveform4_1.frequency(simd::float_4(f1, f2, f3, f4));
		waveform4_2.frequency(simd::float_4(f5, f6, 0.f, 0.f));
	}

	float processGraph(float sampleTime) override {

		float mix1 = mixer1.process(waveform4_1.process(sampleTime, modulator4_1.process(sampleTime)));
		float mix2 = mixer2.process(waveform4_2.process(sampleTime, modulator4_2.process(sampleTime)));

		altOutput = mix1;

		return mixer5.process(mix1, mix2, 0.f, 0.f);
	}

	AudioStream& getStream() override {
		return mixer5;
	}
	unsigned char getPort() override {
		return 0;
	}

private:

	AudioSynthWaveformFloat4       modulator4_1;
	AudioSynthWaveformFloat4       modulator4_2;

	AudioSynthWaveformModulatedFloat4 waveform4_1;
	AudioSynthWaveformModulatedFloat4 waveform4_2;

	// AudioSynthWaveformFloat       modulator1;      //xy=236.88888549804688,262.55556869506836
	// AudioSynthWaveformFloat       modulator3; //xy=238.88890075683594,366.555606842041
	// AudioSynthWaveformFloat       modulator2; //xy=239.88890075683594,313.5556392669678
	// AudioSynthWaveformFloat       modulator5; //xy=239.88890075683594,485.5555810928345
	// AudioSynthWaveformFloat       modulator4; //xy=240.88890075683594,428.55560970306396
	// AudioSynthWaveformFloat       modulator6; //xy=242.8888931274414,541.5555143356323
	// AudioSynthWaveformModulatedFloat waveform1;      //xy=459.88890075683594,268.6667013168335
	// AudioSynthWaveformModulatedFloat waveform4;      //xy=459.8888854980469,433.6666889190674
	// AudioSynthWaveformModulatedFloat waveform2;      //xy=461.88890838623047,322.66671657562256
	// AudioSynthWaveformModulatedFloat waveform3;      //xy=461.8888854980469,373.6666889190674
	// AudioSynthWaveformModulatedFloat waveform6;      //xy=461.88890838623047,546.6666688919067
	// AudioSynthWaveformModulatedFloat waveform5;      //xy=462.88888931274414,492.66670417785645
	AudioMixer4Float              mixer2;         //xy=663.8888053894043,520.6666412353516
	AudioMixer4Float              mixer1;         //xy=667.888916015625,349.6667251586914
	AudioMixer4Float              mixer5;         //xy=853.8889122009277,448.6667900085449
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

REGISTER_PLUGIN(TriFMcluster);
