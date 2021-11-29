/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Jonathan Payne (jon@jonnypayne.com)
 * Based on Effect_Fade by Paul Stoffregen

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

class AudioEffectBitcrusher : public AudioStream {
public:
	AudioEffectBitcrusher(void)
		: AudioStream(1) {}
	void bits(uint8_t b) {
		if (b > 16)
			b = 16;
		else if (b == 0)
			b = 1;
		crushBits = b;
	}
	void sampleRate(float hz) {
		// modification to account for Rack sample rate
		int n = (APP->engine->getSampleRate() / hz) + 0.5f;
		if (n < 1)
			n = 1;
		else if (n > 64)
			n = 64;
		sampleStep = n;
	}
	virtual void update(const audio_block_t* inputBlock, audio_block_t* block);

private:
	uint8_t crushBits; // 16 = off
	uint8_t sampleStep; // the number of samples to double up. This simple technique only allows a few stepped positions.
};

