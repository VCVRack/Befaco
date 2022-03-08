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

#define MULTI_UNITYGAIN 256

static void applyGain(int16_t* data, int32_t mult) {
	const int16_t* end = data + AUDIO_BLOCK_SAMPLES;

	do {
		int32_t val = *data * mult;
		*data++ = signed_saturate_rshift(val, 16, 0);
	} while (data < end);
}

static void applyGainThenAdd(int16_t* dst, const int16_t* src, int32_t mult) {
	const int16_t* end = dst + AUDIO_BLOCK_SAMPLES;

	if (mult == MULTI_UNITYGAIN) {
		do {
			int32_t val = *dst + *src++;
			*dst++ = signed_saturate_rshift(val, 16, 0);
		} while (dst < end);
	}
	else {
		do {
			int32_t val = *dst + ((*src++ * mult) >> 8); // overflow possible??
			*dst++ = signed_saturate_rshift(val, 16, 0);
		} while (dst < end);
	}
}

class AudioMixer4 : public AudioStream {
public:
	AudioMixer4(void) : AudioStream(4) {
		for (int i = 0; i < 4; i++)
			multiplier[i] = 256;
	}

	void update(const audio_block_t* in1, const audio_block_t* in2, const audio_block_t* in3, const audio_block_t* in4, audio_block_t* out) {

		if (!out) {
			return;
		}
		else {
			// zero buffer before processing
			out->zeroAudioBlock();
		}

		if (in1) {
			applyGainThenAdd(out->data, in1->data, multiplier[0]);
		}
		if (in2) {
			applyGainThenAdd(out->data, in2->data, multiplier[1]);
		}
		if (in3) {
			applyGainThenAdd(out->data, in3->data, multiplier[2]);
		}
		if (in4) {
			applyGainThenAdd(out->data, in4->data, multiplier[3]);
		}
	}

	void gain(unsigned int channel, float gain) {
		if (channel >= 4)
			return;
		if (gain > 127.0f)
			gain = 127.0f;
		else if (gain < -127.0f)
			gain = -127.0f;
		multiplier[channel] = gain * 256.0f; // TODO: proper roundoff?
	}
private:
	int16_t multiplier[4];	
};


class AudioAmplifier : public AudioStream {
public:
	AudioAmplifier(void) : AudioStream(1), multiplier(65536) {
	}

	// acts in place
	void update(audio_block_t* block) {
		if (!block)	{
			return;
		}

		int32_t mult = multiplier;

		if (mult == 0) {
			// zero gain, discard any input and transmit nothing
			memset(block->data, 0, sizeof(int16_t) * AUDIO_BLOCK_SAMPLES);
		}
		else if (mult == MULTI_UNITYGAIN) {
			// unity gain, pass input to output without any change
			return;
		}
		else {
			// apply gain to signal
			applyGain(block->data, mult);
		}
	}

	void gain(float n) {
		if (n > 32767.0f)
			n = 32767.0f;
		else if (n < -32767.0f)
			n = -32767.0f;
		multiplier = n * 65536.0f;
	}
private:
	int32_t multiplier;
};