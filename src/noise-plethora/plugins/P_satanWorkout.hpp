#pragma once

#include "NoisePlethoraPlugin.hpp"

class satanWorkout : public NoisePlethoraPlugin {

public:

	satanWorkout()
	//: patchCord1(pink1, pwm1)
	//, patchCord2(pwm1, freeverb1)
	{ }

	~satanWorkout() override {}

	// delete copy constructors
	satanWorkout(const satanWorkout&) = delete;
	satanWorkout& operator=(const satanWorkout&) = delete;

	void init() override {

		pink1.amplitude(1);
		pwm1.amplitude(1);
		freeverb1.damping(-5);

	}

	void process(float k1, float k2) override {


		float knob_1 = k1;
		float knob_2 = k2;

		float pitch1 = pow(knob_1, 2);
		// float pitch2 = pow(knob_2, 2);

		pwm1.frequency(8 + pitch1 * 6000);

		freeverb1.roomsize(0.001 + knob_2 * 4);


		//Serial.print(pitch1 * 10000 + 20);
		//Serial.println();
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {
		pink1.update(&noiseBlock);
		pwm1.update(&noiseBlock, &pwmBlock);
		freeverb1.update(&pwmBlock, &freeverbBlock);

		blockBuffer.pushBuffer(freeverbBlock.data, AUDIO_BLOCK_SAMPLES);
	}

	AudioStream& getStream() override {
		return freeverb1;
	}
	unsigned char getPort() override {
		return 0;
	}

private:
	audio_block_t noiseBlock, pwmBlock, freeverbBlock;

	// GUItool: begin automatically generated code
	AudioSynthNoisePink      pink1;          //xy=927.6667137145996,1712.0000095367432
	AudioSynthWaveformPWM    pwm1;           //xy=1065.6666564941406,1795.666675567627
	AudioEffectFreeverb      freeverb1;      //xy=1152.6666564941406,1739.6666746139526
	//AudioConnection          patchCord1;
	//AudioConnection          patchCord2;


};

REGISTER_PLUGIN(satanWorkout); // this is important, so that we can include the plugin in a bank
