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

		waveform1.begin(1, 100, WAVEFORM_PULSE);
		waveform1.offset(1);
		waveform1.pulseWidth(0.5);
		waveform2.begin(0, 77, WAVEFORM_PULSE);
		waveform2.offset(1);
		waveform2.pulseWidth(0.5);
		waveform3.begin(0, 77, WAVEFORM_PULSE);
		waveform3.offset(1);
		waveform3.pulseWidth(0.5);
		noise1.amplitude(1);
	}

	void process(float k1, float k2) override {

		float knob_1 = k1;
		float knob_2 = k2;
		float pitch = pow(knob_1, 2);
		waveform1.frequency(pitch * 100 + 10);
		waveform1.pulseWidth(knob_2 * 0.95);
		noise1.amplitude(knob_2 * -1 + 2);
		waveform2.frequency(pitch * 0.1);
		waveform2.pulseWidth(knob_2 * 0.5 + 0.2);
		waveform3.frequency(pitch * 0.7 - 500);
		waveform3.pulseWidth(knob_2 * 0.5);
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {

		noise1.update(&noiseOut);

		// NOTE: 2,3 not actually used, as volume is 0 in init()!
		// waveform3.update(&waveformOut[2]);
		// waveform2.update(&waveformOut[1]);
		waveform1.update(&waveformOut[0]);

		multiply1.update(&noiseOut, &waveformOut[0], &multiplyOut[0]);
		// NOTE: 2,3 not actually used, as volume is 0 in init()!
		// multiply2.update(&noiseOut, &waveformOut[1], &multiplyOut[1]);
		// multiply3.update(&noiseOut, &waveformOut[2], &multiplyOut[2]);

		// NOTE: 2,3 not actually used, as volume is 0 in init()!
		// mixer1.update(&multiplyOut[0], &multiplyOut[1], &multiplyOut[2], nullptr, &mixerOut);

		blockBuffer.pushBuffer(multiplyOut[0].data, AUDIO_BLOCK_SAMPLES);
	}

	AudioStream& getStream() override {
		return mixer1;
	}
	unsigned char getPort() override {
		return 0;
	}

private:

	audio_block_t noiseOut, waveformOut[3] = {}, multiplyOut[3] = {};

	AudioSynthNoiseWhite      noise1;         //xy=240,621
	AudioSynthWaveform       waveform3; //xy=507,823
	AudioSynthWaveform       waveform2;      //xy=522,657
	AudioSynthWaveform       waveform1;      //xy=545,475
	AudioEffectMultiply      multiply2;      //xy=615,596
	AudioEffectMultiply      multiply1;      //xy=634,340
	AudioEffectMultiply      multiply3; //xy=635,753
	AudioMixer4              mixer1;         //xy=863,612

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
