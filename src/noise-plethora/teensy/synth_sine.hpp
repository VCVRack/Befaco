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


// TODO: investigate making a high resolution sine wave
// using Taylor series expansion.
// http://www.musicdsp.org/showone.php?id=13

class AudioSynthWaveformSine : public AudioStream {
public:
	AudioSynthWaveformSine() : AudioStream(0), magnitude(16384) {}
	void frequency(float freq) {
		
		// for reproducibility, max frequency cuts out at 1/2 Teensy sample rate
		// (unless we're running at very low sample rates, in which case use those to limit range)
		const float maxFrequency = std::min(AUDIO_SAMPLE_RATE_EXACT, APP->engine->getSampleRate()) / 2.0f;

		if (freq < 0.0f)
			freq = 0.0;
		else if (freq > maxFrequency)
			freq = maxFrequency;
		phase_increment = freq * (4294967296.0f / APP->engine->getSampleRate());
	}
	void phase(float angle) {
		if (angle < 0.0f)
			angle = 0.0f;
		else if (angle > 360.0f) {
			angle = angle - 360.0f;
			if (angle >= 360.0f)
				return;
		}
		phase_accumulator = angle * (float)(4294967296.0 / 360.0);
	}
	void amplitude(float n) {
		if (n < 0.0f)
			n = 0;
		else if (n > 1.0f)
			n = 1.0f;
		magnitude = n * 65536.0f;
	}
	void update(audio_block_t* block) {
		uint32_t i, ph, inc, index, scale;
		int32_t val1, val2;

		if (magnitude) {
			if (block) {
				ph = phase_accumulator;
				inc = phase_increment;
				for (i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
					index = ph >> 24;
					val1 = AudioWaveformSine[index];
					val2 = AudioWaveformSine[index + 1];
					scale = (ph >> 8) & 0xFFFF;
					val2 *= scale;
					val1 *= 0x10000 - scale;
					block->data[i] = (((val1 + val2) >> 16) * magnitude) >> 16;
					ph += inc;
				}
				phase_accumulator = ph;
				return;
			}
		}
		phase_accumulator += phase_increment * AUDIO_BLOCK_SAMPLES;
	}
private:
	uint32_t phase_accumulator;
	uint32_t phase_increment;
	int32_t magnitude;
};




class AudioSynthWaveformSineModulated : public AudioStream {
public:
	AudioSynthWaveformSineModulated() : AudioStream(1), magnitude(16384) {}
	// maximum unmodulated carrier frequency is 11025 Hz
	// input = +1.0 doubles carrier
	// input = -1.0 DC output
	void frequency(float freq) {

		// for reproducibility, max frequency cuts out at 1/4 Teensy sample rate
		// (unless we're running at very low sample rates, in which case use those to limit range)
		const float maxFrequency = std::min(AUDIO_SAMPLE_RATE_EXACT, APP->engine->getSampleRate()) / 4.0f;

		if (freq < 0.0f)
			freq = 0.0f;
		else if (freq > maxFrequency)
			freq = maxFrequency;
		phase_increment = freq * (4294967296.0f / APP->engine->getSampleRate());
	}
	void phase(float angle) {
		if (angle < 0.0f)
			angle = 0.0;
		else if (angle > 360.0f) {
			angle = angle - 360.0f;
			if (angle >= 360.0f)
				return;
		}
		phase_accumulator = angle * (float)(4294967296.0 / 360.0);
	}

	void amplitude(float n) {
		if (n < 0.0f)
			n = 0;
		else if (n > 1.0f)
			n = 1.0f;
		magnitude = n * 65536.0f;
	}

	void update(const audio_block_t* modinput, audio_block_t* block) {

		uint32_t i, ph, inc, index, scale;
		int32_t val1, val2;
		int16_t mod;

		ph = phase_accumulator;
		inc = phase_increment;
		if (!block) {
			// unable to allocate memory, so we'll send nothing
			return;
		}
		if (modinput) {
			for (i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
				index = ph >> 24;
				val1 = AudioWaveformSine[index];
				val2 = AudioWaveformSine[index + 1];
				scale = (ph >> 8) & 0xFFFF;
				val2 *= scale;
				val1 *= 0x10000 - scale;
				//block->data[i] = (((val1 + val2) >> 16) * magnitude) >> 16;
				block->data[i] = multiply_32x32_rshift32(val1 + val2, magnitude);
				// -32768 = no phase increment
				// 32767 = double phase increment
				mod = modinput->data[i];
				ph += inc + (multiply_32x32_rshift32(inc, mod << 16) << 1);
				//ph += inc + (((int64_t)inc * (mod << 16)) >> 31);
			}
		}
		else {
			ph = phase_accumulator;
			inc = phase_increment;
			for (i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
				index = ph >> 24;
				val1 = AudioWaveformSine[index];
				val2 = AudioWaveformSine[index + 1];
				scale = (ph >> 8) & 0xFFFF;
				val2 *= scale;
				val1 *= 0x10000 - scale;
				block->data[i] = multiply_32x32_rshift32(val1 + val2, magnitude);
				ph += inc;
			}
		}
		phase_accumulator = ph;
	}
private:
	uint32_t phase_accumulator;
	uint32_t phase_increment;
	int32_t magnitude;
};

