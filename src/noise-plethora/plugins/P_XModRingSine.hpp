
#pragma once

#include "NoisePlethoraPlugin.hpp"


class XModRingSine : public NoisePlethoraPlugin {

public:
	XModRingSine()
		// : patchCord1(sine_fm1, sine_fm2),
		//   patchCord2(sine_fm1, 0, multiply1, 0),
		//   patchCord3(sine_fm2, sine_fm1),
		//   patchCord4(sine_fm2, 0, multiply1, 1)
		//   // patchCord5(multiply1, 0, i2s1, 0)
	{ }
	~XModRingSine() override {}

	// delete copy constructors
	XModRingSine(const XModRingSine&) = delete;
	XModRingSine& operator=(const XModRingSine&) = delete;


	void init() override {
		sine_fm1.frequency(1100);
		sine_fm2.frequency(1367);

		sine_fm1.amplitude(1);
		sine_fm2.amplitude(1);
	}
	void process(float k1, float k2) override {
		// Read CV and knobs,sum them and scale to 0-1.0
		float knob_1 = k1;
		float knob_2 = k2;

		float pitch1 = pow(knob_1, 2);
		float pitch2 = pow(knob_2, 2);


		sine_fm1.frequency(100 + (pitch1 * 8000));
		sine_fm2.frequency(60 + (pitch2 * 3000));


		//Serial.print(knob_2*0.5);
	}

	float processGraph(float sampleTime) override {
		float s1 = sine_fm1.process(sampleTime, sine_fm2_previous);
		float s2 = sine_fm2.process(sampleTime, s1);
		sine_fm2_previous = s2;
		altOutput = s1;

		return multiply1.process(s1, s2);
	}


	AudioStream& getStream() override {
		return multiply1;
	}

	unsigned char getPort() override {
		return 0;
	}

private:

	AudioSynthWaveformSineModulatedFloat sine_fm1;       //xy=360,220
	AudioSynthWaveformSineModulatedFloat sine_fm2;       //xy=363,404
	AudioEffectMultiplyFloat      multiply1;      //xy=569,311

	// AudioConnection          patchCord1;
	// AudioConnection          patchCord2;
	// AudioConnection          patchCord3;
	// AudioConnection          patchCord4;

	float sine_fm2_previous = 0.f;

};

REGISTER_PLUGIN(XModRingSine); // this is important, so that we can include the plugin in a bank
