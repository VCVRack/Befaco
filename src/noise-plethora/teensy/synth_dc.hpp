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

// compute (a - b) / c  ... handling 32 bit interger overflow without slow 64 bit math
static inline int32_t substract_int32_then_divide_int32(int32_t a, int32_t b, int32_t c) __attribute__((always_inline, unused));
static inline int32_t substract_int32_then_divide_int32(int32_t a, int32_t b, int32_t c) {
	uint32_t diff;
	uint8_t negative;

	if (a >= 0) {
		if (b >= 0) {
			return (a - b) / c;  // no overflow if both a & b are positive
		}
		else {
			diff = a + (b * -1); // assumes 0x80000000 * -1 == 0x80000000
			negative = 0;
		}
	}
	else {
		if (b >= 0) {
			diff = (a * -1) + b; // assumes 0x80000000 * -1 == 0x80000000
			negative = 1;
		}
		else {
			return (a - b) / c;  // no overflow if both a & b are negative
		}
	}
	if (c >= 0) {
		diff = diff / (uint32_t)c;
	}
	else {
		diff = diff / (uint32_t)(c * -1);
		negative ^= 1;
	}
	if (negative) {
		if (diff > 0x7FFFFFFF)
			return 0x80000000;
		return (int32_t)diff * -1;
	}
	else {
		if (diff > 0x7FFFFFFF)
			return 0x7FFFFFFF;
		return (int32_t)diff;
	}
}


class AudioSynthWaveformDc : public AudioStream {
public:
	AudioSynthWaveformDc() : AudioStream(0), state(0), magnitude(0) {}
	// immediately jump to the new DC level
	void amplitude(float n) {
		if (n > 1.0f)
			n = 1.0f;
		else if (n < -1.0f)
			n = -1.0f;
		int32_t m = (int32_t)(n * 2147418112.0f);
		magnitude = m;
		state = 0;
	}
	// slowly transition to the new DC level
	void amplitude(float n, float milliseconds) {
		if (milliseconds <= 0.0f) {
			amplitude(n);
			return;
		}
		if (n > 1.0f)
			n = 1.0f;
		else if (n < -1.0f)
			n = -1.0f;
		int32_t c = (int32_t)(milliseconds * (AUDIO_SAMPLE_RATE_EXACT / 1000.0f));
		if (c == 0) {
			amplitude(n);
			return;
		}
		int32_t t = (int32_t)(n * 2147418112.0f);
		target = t;
		if (target == magnitude) {
			state = 0;
			return;
		}
		increment = substract_int32_then_divide_int32(target, magnitude, c);
		if (increment == 0) {
			increment = (target > magnitude) ? 1 : -1;
		}
		state = 1;
	}
	float read(void) {
		int32_t m = magnitude;
		return (float)m * (float)(1.0 / 2147418112.0);
	}
	void update(audio_block_t* block) {
		uint32_t* p, *end, val;
		int32_t count, t1, t2, t3, t4;

		if (!block) {
			return;
		}
		p = (uint32_t*)(block->data);
		end = p + AUDIO_BLOCK_SAMPLES / 2;

		if (state == 0) {
			// steady DC output, simply fill the buffer with fixed value
			val = pack_16t_16t(magnitude, magnitude);
			do {
				*p++ = val;
				*p++ = val;
				*p++ = val;
				*p++ = val;
				*p++ = val;
				*p++ = val;
				*p++ = val;
				*p++ = val;
			} while (p < end);
		}
		else {
			// transitioning to a new DC level
			//count = (target - magnitude) / increment;
			count = substract_int32_then_divide_int32(target, magnitude, increment);
			if (count >= AUDIO_BLOCK_SAMPLES) {
				// this update will not reach the target
				do {
					magnitude += increment;
					t1 = magnitude;
					magnitude += increment;
					t1 = pack_16t_16t(magnitude, t1);
					magnitude += increment;
					t2 = magnitude;
					magnitude += increment;
					t2 = pack_16t_16t(magnitude, t2);
					magnitude += increment;
					t3 = magnitude;
					magnitude += increment;
					t3 = pack_16t_16t(magnitude, t3);
					magnitude += increment;
					t4 = magnitude;
					magnitude += increment;
					t4 = pack_16t_16t(magnitude, t4);
					*p++ = t1;
					*p++ = t2;
					*p++ = t3;
					*p++ = t4;
				} while (p < end);
			}
			else {
				// this update reaches the target
				while (count >= 2) {
					count -= 2;
					magnitude += increment;
					t1 = magnitude;
					magnitude += increment;
					t1 = pack_16t_16t(magnitude, t1);
					*p++ = t1;
				}
				if (count) {
					t1 = pack_16t_16t(target, magnitude + increment);
					*p++ = t1;
				}
				magnitude = target;
				state = 0;
				val = pack_16t_16t(magnitude, magnitude);
				while (p < end) {
					*p++ = val;
				}
			}
		}
	}

private:
	uint8_t  state;     // 0=steady output, 1=transitioning
	int32_t  magnitude; // current output
	int32_t  target;    // designed output (while transitiong)
	int32_t  increment; // adjustment per sample (while transitiong)
};
