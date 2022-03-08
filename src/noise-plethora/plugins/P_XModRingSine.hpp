
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

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {

		sine_fm1.update(&sineModOut[1], &sineModOut[0]);
		sine_fm2.update(&sineModOut[0], &sineModOut[1]);

		multiply1.update(&sineModOut[0], &sineModOut[1], &multiplyOut);

		blockBuffer.pushBuffer(multiplyOut.data, AUDIO_BLOCK_SAMPLES);
	}


	AudioStream& getStream() override {
		return multiply1;
	}

	unsigned char getPort() override {
		return 0;
	}

private:

	audio_block_t sineModOut[2] = {}, multiplyOut;

	AudioSynthWaveformSineModulated sine_fm1;       //xy=360,220
	AudioSynthWaveformSineModulated sine_fm2;       //xy=363,404
	AudioEffectMultiply      multiply1;      //xy=569,311

	// AudioConnection          patchCord1;
	// AudioConnection          patchCord2;
	// AudioConnection          patchCord3;
	// AudioConnection          patchCord4;
};

REGISTER_PLUGIN(XModRingSine); // this is important, so that we can include the plugin in a bank
