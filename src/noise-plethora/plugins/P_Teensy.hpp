#pragma once

#include "NoisePlethoraPlugin.hpp"

class Teensy : public NoisePlethoraPlugin {

public:

	Teensy()
	//:patchCord1(waveformMod1, 0, waveformMod2, 1),
	//patchCord3(waveformMod2, 0, waveformMod1, 0)
	{ }

	~Teensy() override {}

	Teensy(const Teensy&) = delete;
	Teensy& operator=(const Teensy&) = delete;

	void init() override {

		if (false) {
			waveformMod1.begin(1.0, 0.3, WAVEFORM_SINE);
			waveformMod2.begin(1.0, 10., WAVEFORM_PULSE);
		}
		else {
			waveformMod1.begin(1.0, 300, WAVEFORM_SQUARE);
			waveformMod2.begin(1.0, 2000., WAVEFORM_SINE);
			waveformMod2.frequencyModulation(2);
		}

	}

	void process(float k1, float k2) override {



	}

	float processGraph(float sampleTime) override {

		/*
		if (false) {
			// waveformMod2 has phase modulation from waveformMod1
			float waveformMod2Out = waveformMod2.process(sampleTime, 0.f, waveformMod1OutPrev);

			// waveformMod1 has FM from waveformMod2
			float waveformMod1Out = waveformMod1.process(sampleTime);

			//DEBUG(string::f("Teensy: %g %g (k1/k2: %g %g)", waveformMod1Out, waveformMod2Out, k1, k2).c_str());

			waveformMod1OutPrev = waveformMod1Out;
			return waveformMod2Out;
		}
		else {
			// waveformMod2 has FM from waveformMod1
			float waveformMod2Out = waveformMod2.process(sampleTime, waveformMod1OutPrev, 0.f);

			// waveformMod1
			float waveformMod1Out = waveformMod1.process(sampleTime);

			altOutput = waveformMod1Out;
			waveformMod1OutPrev = waveformMod1Out;
			return waveformMod2Out;
		}
		*/

		return 0.f;
	}


	AudioStream& getStream() override {
		return waveformMod2;
	}
	unsigned char getPort() override {
		return 0;
	}

private:
	float waveformMod1OutPrev = 0.f;

	//AudioSynthNoiseWhite     noise1;         //xy=374.1999969482422,240.1999969482422
	//AudioSynthWaveform       waveform1;      //xy=383.20001220703125,176.99998474121094
	//AudioEffectMultiply      multiply1;      //xy=625.2000122070312,262
	//AudioMixer4      mixer1;      //xy=625.2000122070312,262
	// AudioSynthWaveformDc dc1;
	//AudioSynthWaveformSineModulated sine1;
	//AudioSynthWaveformSineModulated sine_fm1;
	AudioSynthWaveformModulated waveformMod1;   //xy=334,349
	AudioSynthWaveformModulated waveformMod2; //xy=616,284
	//AudioConnection          patchCord1;
	//AudioConnection          patchCord3;

};

REGISTER_PLUGIN(Teensy);
