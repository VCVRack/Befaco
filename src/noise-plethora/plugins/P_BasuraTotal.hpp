#pragma once

#include "NoisePlethoraPlugin.hpp"

class BasuraTotal : public NoisePlethoraPlugin {

public:

	BasuraTotal()
	// : patchCord1(waveformMod1, freeverb1)
	{}

	~BasuraTotal() override {}

	BasuraTotal(const BasuraTotal&) = delete;
	BasuraTotal& operator=(const BasuraTotal&) = delete;

	dsp::Timer timer;

	void init() override {
		
		freeverb1.roomsize(0);
		timer.reset();
		waveformMod1.begin(1, 500, WAVEFORM_SINE);
	}

	void process(float k1, float k2) override {

		float knob_1 = k1;
		float knob_2 = k2;

		float pitch1 = pow(knob_1, 2);
		float pitch2 = pow(knob_2, 2);
		
		float timeMicros = timer.time * 1000000;
		if (timeMicros > 100000 * pitch2) { // Changing this value changes the frequency.
			timer.reset();

			waveformMod1.begin(1, 500, WAVEFORM_SQUARE);
			waveformMod1.frequency(generateNoise() * (200 + (pitch1 * 5000))) ;
			freeverb1.roomsize(1);
		}
	}

	float processGraph(float sampleTime) override {

		timer.process(sampleTime);

		if (w1.empty() || w2.empty()) {					

			waveformMod1.update(nullptr, nullptr, &output1);
			w1.pushBuffer(output1.data, AUDIO_BLOCK_SAMPLES);
			
			freeverb1.update(&output1, &output2);
			w2.pushBuffer(output2.data, AUDIO_BLOCK_SAMPLES);
		}
		
		float waveformMod1Out = int16_to_float_5v(w1.shift()) / 5.f ;
		float freeverbOut = int16_to_float_5v(w2.shift()) / 5.f;

		altOutput = waveformMod1Out;
		return freeverbOut;
	}

	AudioStream& getStream() override { return freeverb1; }
	unsigned char getPort() override { return 0; }

private:

	static unsigned int generateNoise() {
		// See https://en.wikipedia.org/wiki/Linear_feedback_shift_register#Galois_LFSRs
		/* initialize with any 32 bit non-zero  unsigned long value. */
		static unsigned long int lfsr = 0xfeddfaceUL; /* 32 bit init, nonzero */
		static unsigned long mask = ((unsigned long)(1UL << 31 | 1UL << 15 | 1UL << 2 | 1UL << 1));
		/* If the output bit is 1, apply toggle mask.
		 * The value has 1 at bits corresponding
		 * to taps, 0 elsewhere. */

		if (lfsr & 1) {
			lfsr = (lfsr >> 1) ^ mask;
			return (1);
		}
		else {
			lfsr >>= 1;
			return (0);
		}
	}

	TeensyBuffer w1, w2; 
	audio_block_t output1 = {};
	audio_block_t output2 = {};

	AudioSynthWaveformModulated waveformMod1;   //xy=216.88888549804688,217.9999988898635
	AudioEffectFreeverb         freeverb1;      //xy=374.8888854980469,153.88888549804688
	// AudioConnection             patchCord1;
	// unsigned long               lastClick;
};

REGISTER_PLUGIN(BasuraTotal);