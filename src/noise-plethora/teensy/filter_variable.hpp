/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#include "audio_core.hpp"

class AudioFilterStateVariable: public AudioStream {
public:
	AudioFilterStateVariable() : AudioStream(2) {
		frequency(1000);
		octaveControl(1.0); // default values
		resonance(0.707);
		state_inputprev = 0;
		state_lowpass = 0;
		state_bandpass = 0;
	}
	void frequency(float freq) {
		// for reproducibility, max frequency cuts out at 2/5 Teensy sample rate 
		// (unless we're running at very low sample rates, in which case make sure we don't allow unstable f_c)
		const float minFrequency = 20.f;
		const float maxFrequency = std::min(AUDIO_SAMPLE_RATE_EXACT, APP->engine->getSampleRate()) / 2.5f;

		if (freq < minFrequency) {
			freq = minFrequency;
		}
		else if (freq > maxFrequency) {		
			freq = maxFrequency;
		}
		setting_fcenter = (freq * (3.141592654f / (APP->engine->getSampleRate() * 2.0f)))
		                  * 2147483647.0f;
		// TODO: should we use an approximation when freq is not a const,
		// so the sinf() function isn't linked?
		setting_fmult = sinf(freq * (3.141592654f / (APP->engine->getSampleRate() * 2.0f)))
		                * 2147483647.0f;
	}
	void resonance(float q) {
		if (q < 0.7f)
			q = 0.7f;
		else if (q > 5.0f)
			q = 5.0f;
		// TODO: allow lower Q when frequency is lower
		setting_damp = (1.0f / q) * 1073741824.0f;
	}
	void octaveControl(float n) {
		// filter's corner frequency is Fcenter * 2^(control * N)
		// where "control" ranges from -1.0 to +1.0
		// and "N" allows the frequency to change from 0 to 7 octaves
		if (n < 0.0f)
			n = 0.0f;
		else if (n > 6.9999f)
			n = 6.9999f;
		setting_octavemult = n * 4096.0f;
	}

	void update(const audio_block_t* input_block, const audio_block_t* control_block,
	            audio_block_t* lowpass_block, audio_block_t* bandpass_block, audio_block_t* highpass_block) {

		if (control_block) {
			update_variable(input_block->data,
			                control_block->data,
			                lowpass_block->data,
			                bandpass_block->data,
			                highpass_block->data);
		}
		else {
			update_fixed(input_block->data,
			             lowpass_block->data,
			             bandpass_block->data,
			             highpass_block->data);
		}
		return;
	}

private:
	void update_fixed(const int16_t* in, int16_t* lp, int16_t* bp, int16_t* hp);
	void update_variable(const int16_t* in, const int16_t* ctl, int16_t* lp, int16_t* bp, int16_t* hp);
	int32_t setting_fcenter;
	int32_t setting_fmult;
	int32_t setting_octavemult;
	int32_t setting_damp;
	int32_t state_inputprev;
	int32_t state_lowpass;
	int32_t state_bandpass;
};
