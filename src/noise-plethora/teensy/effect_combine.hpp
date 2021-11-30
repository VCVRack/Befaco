/* Copyright (c) 2018 John-Michael Reed
 * bleeplabs.com
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
 *
 * Combine analog signals with bitwise expressions like XOR.
 * Combining two simple oscillators results in interesting new waveforms,
 * Combining white noise or dynamic incoming audio results in aggressive digital distortion.
 */

#ifndef effect_digital_combine_h_
#define effect_digital_combine_h_

#include "audio_core.hpp"

class AudioEffectDigitalCombine : public AudioStream {
public:
	enum combineMode {
		OR    = 0,
		XOR   = 1,
		AND   = 2,
		MODULO = 3,
	};
	AudioEffectDigitalCombine() : AudioStream(2), mode_sel(OR) { }
	void setCombineMode(int mode_in) {
		if (mode_in > 3) {
			mode_in = 3;
		}
		mode_sel = mode_in;
	}

	void update(const audio_block_t* blocka, const audio_block_t* blockb, audio_block_t* output) {
		uint32_t* pa, *pb, *end, *pout;
		uint32_t a12, a34; //, a56, a78;
		uint32_t b12, b34; //, b56, b78;

		//blocka = receiveWritable(0);
		//blockb = receiveReadOnly(1);
		if (!blocka || !blockb) {			
			return;
		}
		pa = (uint32_t*)(blocka->data);
		pb = (uint32_t*)(blockb->data);
		pout = (uint32_t *)(output->data);
		end = pa + AUDIO_BLOCK_SAMPLES / 2;

		while (pa < end) {
			a12 = *pa++;
			a34 = *pa++;
			b12 = *pb++;
			b34 = *pb++;
			if (mode_sel == OR) {
				a12 = a12 | b12;
				a34 = a34 | b34;
			}
			if (mode_sel == XOR) {
				a12 = a12 ^ b12;
				a34 = a34 ^ b34;
			}
			if (mode_sel == AND) {
				a12 = a12 & b12;
				a34 = a34 & b34;
			}
			if (mode_sel == MODULO) {
				a12 = a12 % b12;
				a34 = a34 % b34;
			}
			*pout++ = a12;
			*pout++ = a34;
		}
		//transmit(blocka);
		//release(blocka);
		//release(blockb);
	}

private:
	short mode_sel;
	audio_block_t* inputQueueArray[2];
};

#endif