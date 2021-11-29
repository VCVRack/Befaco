#pragma once

#include "NoisePlethoraPlugin.hpp"

class basurilla : public NoisePlethoraPlugin {

public:

	basurilla()
		// : patchCord1(noise1, 0, multiply1, 0),
		//   patchCord2(noise1, 0, multiply2, 0),
		//   patchCord3(noise1, 0, multiply3, 0),
		//   patchCord6(waveform3, 0, multiply3, 1),
		//   patchCord7(waveform2, 0, multiply2, 1),
		//   patchCord8(waveform1, 0, multiply1, 1),
		//   patchCord9(multiply2, 0, mixer1, 1),
		//   patchCord10(multiply1, 0, mixer1, 0),
		//   patchCord11(multiply3, 0, mixer1, 2)
	{}


	~basurilla() override {}

	basurilla(const basurilla&) = delete;
	basurilla& operator=(const basurilla&) = delete;

	void init() override {

		noise1.amplitude(1);

		waveform4_1.begin(simd::float_4(1, 1, 1, 0), simd::float_4(100, 77, 77, 0), WAVEFORM_PULSE);
		waveform4_1.offset(simd::float_4(1, 1, 1, 0));
		waveform4_1.pulseWidth(simd::float_4(0.5, 0.5, 0.5, 0.5));

		DEBUG("basurilla setup complete");
	}

	void process(float k1, float k2) override {

		float knob_1 = k1;
		float knob_2 = k2;
		float pitch = pow(knob_1, 2);

		// smid version
		waveform4_1.frequency(simd::float_4(20 + pitch * 10, 20 + pitch * 7, 20 + pitch * 20, 0.f));
		waveform4_1.pulseWidth(simd::float_4(knob_2 * 0.5 + 0.2, knob_2 * 0.5 + 0.2, knob_2 * 0.5 + 0.2, 0.5));
	}

	float processGraph(float sampleTime) override {

		simd::float_4 waveform = waveform4_1.process(sampleTime);
		float noise = noise1.process();

		float mixerInput1 = multiply1.process(waveform[0], noise);
		float mixerInput2 = multiply2.process(waveform[1], noise);
		float mixerInput3 = multiply3.process(waveform[2], noise);
		
		return mixer1.process(mixerInput1, mixerInput2, mixerInput3, 0.f);
	}

	AudioStream& getStream() override {
		return mixer1;
	}
	unsigned char getPort() override {
		return 0;
	}

private:
	AudioSynthNoiseWhiteFloat      noise1;         //xy=240,621
	//AudioSynthWaveform       waveform3; //xy=507,823
	//AudioSynthWaveform       waveform2;      //xy=522,657
	AudioSynthWaveformFloat4       waveform4_1;      //xy=545,475
	AudioEffectMultiplyFloat      multiply2;      //xy=615,596
	AudioEffectMultiplyFloat      multiply1;      //xy=634,340
	AudioEffectMultiplyFloat      multiply3; //xy=635,753
	AudioMixer4Float              mixer1;         //xy=863,612

	// AudioConnection          patchCord1;
	// AudioConnection          patchCord2;
	// AudioConnection          patchCord3;
	// AudioConnection          patchCord6;
	// AudioConnection          patchCord7;
	// AudioConnection          patchCord8;
	// AudioConnection          patchCord9;
	// AudioConnection          patchCord10;
	// AudioConnection          patchCord11;
// 

};

REGISTER_PLUGIN(basurilla);
