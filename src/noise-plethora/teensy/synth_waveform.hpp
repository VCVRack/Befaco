#pragma once

#include "audio_core.hpp"

class AudioSynthWaveform : public AudioStream {
public:
	AudioSynthWaveform(void) : AudioStream(0),
		phase_accumulator(0), phase_increment(0), phase_offset(0),
		magnitude(0), pulse_width(0x40000000),
		arbdata(NULL), sample(0), tone_type(WAVEFORM_SINE),
		tone_offset(0) {
	}

	void frequency(float freq) {

		// for reproducibility, max frequency cuts out at 1/2 Teensy sample rate
		// (unless we're running at very low sample rates, in which case use those to limit range)
		const float maxFrequency = std::min(AUDIO_SAMPLE_RATE_EXACT, APP->engine->getSampleRate()) / 2.0f;

		if (freq < 0.0f) {
			freq = 0.0;
		}
		else if (freq > maxFrequency) {
			freq = maxFrequency;
		}
		phase_increment = freq * (4294967296.0f / APP->engine->getSampleRate());
		if (phase_increment > 0x7FFE0000u)
			phase_increment = 0x7FFE0000;
	}
	void phase(float angle) {
		if (angle < 0.0f) {
			angle = 0.0;
		}
		else if (angle > 360.0f) {
			angle = angle - 360.0f;
			if (angle >= 360.0f)
				return;
		}
		phase_offset = angle * (float)(4294967296.0 / 360.0);
	}
	void amplitude(float n) {	// 0 to 1.0
		if (n < 0) {
			n = 0;
		}
		else if (n > 1.0f) {
			n = 1.0;
		}
		magnitude = n * 65536.0f;
	}
	void offset(float n) {
		if (n < -1.0f) {
			n = -1.0f;
		}
		else if (n > 1.0f) {
			n = 1.0f;
		}
		tone_offset = n * 32767.0f;
	}
	void pulseWidth(float n) {	// 0.0 to 1.0
		if (n < 0) {
			n = 0;
		}
		else if (n > 1.0f) {
			n = 1.0f;
		}
		pulse_width = n * 4294967296.0f;
	}
	void begin(short t_type) {
		phase_offset = 0;
		tone_type = t_type;
	}
	void begin(float t_amp, float t_freq, short t_type) {
		amplitude(t_amp);
		frequency(t_freq);
		phase_offset = 0;
		begin(t_type);
	}

	void arbitraryWaveform(const int16_t* data, float maxFreq) {
		arbdata = data;
	}

	void update(audio_block_t* block) {

		int16_t* bp, *end;
		int32_t val1, val2;
		int16_t magnitude15;
		uint32_t i, ph, index, index2, scale;
		const uint32_t inc = phase_increment;

		ph = phase_accumulator + phase_offset;
		if (magnitude == 0) {
			phase_accumulator += inc * AUDIO_BLOCK_SAMPLES;
			return;
		}

		if (!block) {
			phase_accumulator += inc * AUDIO_BLOCK_SAMPLES;
			return;
		}
		bp = block->data;

		switch (tone_type) {
			case WAVEFORM_SINE:
				for (i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
					index = ph >> 24;
					val1 = AudioWaveformSine[index];
					val2 = AudioWaveformSine[index + 1];
					scale = (ph >> 8) & 0xFFFF;
					val2 *= scale;
					val1 *= 0x10000 - scale;
					*bp++ = multiply_32x32_rshift32(val1 + val2, magnitude);
					ph += inc;
				}
				break;

			case WAVEFORM_ARBITRARY:
				if (!arbdata) {
					phase_accumulator += inc * AUDIO_BLOCK_SAMPLES;
					return;
				}
				// len = 256
				for (i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
					index = ph >> 24;
					index2 = index + 1;
					if (index2 >= 256)
						index2 = 0;
					val1 = *(arbdata + index);
					val2 = *(arbdata + index2);
					scale = (ph >> 8) & 0xFFFF;
					val2 *= scale;
					val1 *= 0x10000 - scale;
					*bp++ = multiply_32x32_rshift32(val1 + val2, magnitude);
					ph += inc;
				}
				break;

			case WAVEFORM_SQUARE:
				magnitude15 = signed_saturate_rshift(magnitude, 16, 1);
				for (i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
					if (ph & 0x80000000) {
						*bp++ = -magnitude15;
					}
					else {
						*bp++ = magnitude15;
					}
					ph += inc;
				}
				break;

			case WAVEFORM_SAWTOOTH:
				for (i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
					*bp++ = signed_multiply_32x16t(magnitude, ph);
					ph += inc;
				}
				break;

			case WAVEFORM_SAWTOOTH_REVERSE:
				for (i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
					*bp++ = signed_multiply_32x16t(0xFFFFFFFFu - magnitude, ph);
					ph += inc;
				}
				break;

			case WAVEFORM_TRIANGLE:
				for (i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
					uint32_t phtop = ph >> 30;
					if (phtop == 1 || phtop == 2) {
						*bp++ = ((0xFFFF - (ph >> 15)) * magnitude) >> 16;
					}
					else {
						*bp++ = (((int32_t)ph >> 15) * magnitude) >> 16;
					}
					ph += inc;
				}
				break;

			case WAVEFORM_TRIANGLE_VARIABLE:
				do {
					uint32_t rise = 0xFFFFFFFF / (pulse_width >> 16);
					uint32_t fall = 0xFFFFFFFF / (0xFFFF - (pulse_width >> 16));
					for (i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
						if (ph < pulse_width / 2) {
							uint32_t n = (ph >> 16) * rise;
							*bp++ = ((n >> 16) * magnitude) >> 16;
						}
						else if (ph < 0xFFFFFFFF - pulse_width / 2) {
							uint32_t n = 0x7FFFFFFF - (((ph - pulse_width / 2) >> 16) * fall);
							*bp++ = (((int32_t)n >> 16) * magnitude) >> 16;
						}
						else {
							uint32_t n = ((ph + pulse_width / 2) >> 16) * rise + 0x80000000;
							*bp++ = (((int32_t)n >> 16) * magnitude) >> 16;
						}
						ph += inc;
					}
				} while (0);
				break;

			case WAVEFORM_PULSE:
				magnitude15 = signed_saturate_rshift(magnitude, 16, 1);
				for (i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
					if (ph < pulse_width) {
						*bp++ = magnitude15;
					}
					else {
						*bp++ = -magnitude15;
					}
					ph += inc;
				}
				break;

			case WAVEFORM_SAMPLE_HOLD:
				for (i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
					*bp++ = sample;
					uint32_t newph = ph + inc;
					if (newph < ph) {
						sample = teensy::random_teensy(magnitude) - (magnitude >> 1);
					}
					ph = newph;
				}
				break;
		}
		phase_accumulator = ph - phase_offset;

		if (tone_offset) {
			bp = block->data;
			end = bp + AUDIO_BLOCK_SAMPLES;
			do {
				val1 = *bp;
				*bp++ = signed_saturate_rshift(val1 + tone_offset, 16, 0);
			} while (bp < end);
		}
	}

private:
	uint32_t phase_accumulator;
	uint32_t phase_increment;
	uint32_t phase_offset;
	int32_t  magnitude;
	uint32_t pulse_width;
	const int16_t* arbdata;
	int16_t  sample; // for WAVEFORM_SAMPLE_HOLD
	short    tone_type;
	int16_t  tone_offset;
};

class AudioSynthWaveformModulated : public AudioStream {
public:
	AudioSynthWaveformModulated(void) : AudioStream(2),
		phase_accumulator(0), phase_increment(0), modulation_factor(32768),
		magnitude(0), arbdata(NULL), sample(0), tone_offset(0),
		tone_type(WAVEFORM_SINE), modulation_type(0) {
	}

	void frequency(float freq) {

		// for reproducibility, max frequency cuts out at 1/2 Teensy sample rate
		// (unless we're running at very low sample rates, in which case use those to limit range)
		const float maxFrequency = std::min(AUDIO_SAMPLE_RATE_EXACT, APP->engine->getSampleRate()) / 2.0f;

		if (freq < 0.0f) {
			freq = 0.0;
		}
		else if (freq > maxFrequency) {
			freq = maxFrequency;
		}
		phase_increment = freq * (4294967296.0f / APP->engine->getSampleRate());
		if (phase_increment > 0x7FFE0000u)
			phase_increment = 0x7FFE0000;
	}
	void amplitude(float n) {	// 0 to 1.0
		if (n < 0) {
			n = 0;
		}
		else if (n > 1.0f) {
			n = 1.0f;
		}
		magnitude = n * 65536.0f;
	}
	void offset(float n) {
		if (n < -1.0f) {
			n = -1.0f;
		}
		else if (n > 1.0f) {
			n = 1.0f;
		}
		tone_offset = n * 32767.0f;
	}
	void begin(short t_type) {
		tone_type = t_type;
		// band-limited waveforms not used
	}
	void begin(float t_amp, float t_freq, short t_type) {
		amplitude(t_amp);
		frequency(t_freq);
		begin(t_type) ;
	}
	void arbitraryWaveform(const int16_t* data, float maxFreq) {
		arbdata = data;
	}
	void frequencyModulation(float octaves) {
		if (octaves > 12.0f) {
			octaves = 12.0f;
		}
		else if (octaves < 0.1f) {
			octaves = 0.1f;
		}
		modulation_factor = octaves * 4096.0f;
		modulation_type = 0;
	}
	void phaseModulation(float degrees) {
		if (degrees > 9000.0f) {
			degrees = 9000.0f;
		}
		else if (degrees < 30.0f) {
			degrees = 30.0f;
		}
		modulation_factor = degrees * (float)(65536.0 / 180.0);
		modulation_type = 1;
	}

	void update(audio_block_t* moddata, audio_block_t* shapedata, audio_block_t* block) {

		int16_t* bp, *end;
		int32_t val1, val2;
		int16_t magnitude15;
		uint32_t i, ph, index, index2, scale, priorphase;
		const uint32_t inc = phase_increment;

		if (!block) {
			return;
		}

		// Pre-compute the phase angle for every output sample of this update
		ph = phase_accumulator;
		priorphase = phasedata[AUDIO_BLOCK_SAMPLES - 1];
		if (moddata && modulation_type == 0) {
			// Frequency Modulation
			bp = moddata->data;
			for (i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
				int32_t n = (*bp++) * modulation_factor; // n is # of octaves to mod
				int32_t ipart = n >> 27; // 4 integer bits
				n &= 0x7FFFFFF;          // 27 fractional bits
#ifdef IMPROVE_EXPONENTIAL_ACCURACY
				// exp2 polynomial suggested by Stefan Stenzel on "music-dsp"
				// mail list, Wed, 3 Sep 2014 10:08:55 +0200
				int32_t x = n << 3;
				n = multiply_accumulate_32x32_rshift32_rounded(536870912, x, 1494202713);
				int32_t sq = multiply_32x32_rshift32_rounded(x, x);
				n = multiply_accumulate_32x32_rshift32_rounded(n, sq, 1934101615);
				n = n + (multiply_32x32_rshift32_rounded(sq,
				         multiply_32x32_rshift32_rounded(x, 1358044250)) << 1);
				n = n << 1;
#else
				// exp2 algorithm by Laurent de Soras
				// https://www.musicdsp.org/en/latest/Other/106-fast-exp2-approximation.html
				n = (n + 134217728) << 3;

				n = multiply_32x32_rshift32_rounded(n, n);
				n = multiply_32x32_rshift32_rounded(n, 715827883) << 3;
				n = n + 715827882;
#endif
				uint32_t scale = n >> (14 - ipart);
				uint64_t phstep = (uint64_t)inc * scale;
				uint32_t phstep_msw = phstep >> 32;
				if (phstep_msw < 0x7FFE) {
					ph += phstep >> 16;
				}
				else {
					ph += 0x7FFE0000;
				}
				phasedata[i] = ph;
			}
		}
		else if (moddata) {
			// Phase Modulation
			bp = moddata->data;
			for (i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
				// more than +/- 180 deg shift by 32 bit overflow of "n"
				uint32_t n = ((uint32_t)(*bp++)) * modulation_factor;
				phasedata[i] = ph + n;
				ph += inc;
			}
		}
		else {
			// No Modulation Input
			for (i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
				phasedata[i] = ph;
				ph += inc;
			}
		}
		phase_accumulator = ph;

		bp = block->data;

		// Now generate the output samples using the pre-computed phase angles
		switch (tone_type) {
			case WAVEFORM_SINE:
				for (i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
					ph = phasedata[i];
					index = ph >> 24;
					val1 = AudioWaveformSine[index];
					val2 = AudioWaveformSine[index + 1];
					scale = (ph >> 8) & 0xFFFF;
					val2 *= scale;
					val1 *= 0x10000 - scale;
					*bp++ = multiply_32x32_rshift32(val1 + val2, magnitude);
				}
				break;

			case WAVEFORM_ARBITRARY:
				if (!arbdata) {
					block->zeroAudioBlock();
					return;
				}
				// len = 256
				for (i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
					ph = phasedata[i];
					index = ph >> 24;
					index2 = index + 1;
					if (index2 >= 256)
						index2 = 0;
					val1 = *(arbdata + index);
					val2 = *(arbdata + index2);
					scale = (ph >> 8) & 0xFFFF;
					val2 *= scale;
					val1 *= 0x10000 - scale;
					*bp++ = multiply_32x32_rshift32(val1 + val2, magnitude);
				}
				break;

			case WAVEFORM_PULSE:
				if (shapedata) {
					magnitude15 = signed_saturate_rshift(magnitude, 16, 1);
					for (i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
						uint32_t width = ((shapedata->data[i] + 0x8000) & 0xFFFF) << 16;
						if (phasedata[i] < width) {
							*bp++ = magnitude15;
						}
						else {
							*bp++ = -magnitude15;
						}
					}
					break;
				} // else fall through to orginary square without shape modulation
			// fall through
			case WAVEFORM_SQUARE:
				magnitude15 = signed_saturate_rshift(magnitude, 16, 1);
				for (i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
					if (phasedata[i] & 0x80000000) {
						*bp++ = -magnitude15;
					}
					else {
						*bp++ = magnitude15;
					}
				}
				break;

			case WAVEFORM_SAWTOOTH:
				for (i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
					*bp++ = signed_multiply_32x16t(magnitude, phasedata[i]);
				}
				break;

			case WAVEFORM_SAWTOOTH_REVERSE:
				for (i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
					*bp++ = signed_multiply_32x16t(0xFFFFFFFFu - magnitude, phasedata[i]);
				}
				break;

			case WAVEFORM_TRIANGLE_VARIABLE:
				if (shapedata) {
					for (i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
						uint32_t width = (shapedata->data[i] + 0x8000) & 0xFFFF;
						uint32_t rise = 0xFFFFFFFF / width;
						uint32_t fall = 0xFFFFFFFF / (0xFFFF - width);
						uint32_t halfwidth = width << 15;
						uint32_t n;
						ph = phasedata[i];
						if (ph < halfwidth) {
							n = (ph >> 16) * rise;
							*bp++ = ((n >> 16) * magnitude) >> 16;
						}
						else if (ph < 0xFFFFFFFF - halfwidth) {
							n = 0x7FFFFFFF - (((ph - halfwidth) >> 16) * fall);
							*bp++ = (((int32_t)n >> 16) * magnitude) >> 16;
						}
						else {
							n = ((ph + halfwidth) >> 16) * rise + 0x80000000;
							*bp++ = (((int32_t)n >> 16) * magnitude) >> 16;
						}
						ph += inc;
					}
					break;
				} // else fall through to orginary triangle without shape modulation
			// fall through
			case WAVEFORM_TRIANGLE:
				for (i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
					ph = phasedata[i];
					uint32_t phtop = ph >> 30;
					if (phtop == 1 || phtop == 2) {
						*bp++ = ((0xFFFF - (ph >> 15)) * magnitude) >> 16;
					}
					else {
						*bp++ = (((int32_t)ph >> 15) * magnitude) >> 16;
					}
				}
				break;
			case WAVEFORM_SAMPLE_HOLD:
				for (i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
					ph = phasedata[i];
					if (ph < priorphase) { // does not work for phase modulation
						sample = teensy::random_teensy(magnitude) - (magnitude >> 1);
					}
					priorphase = ph;
					*bp++ = sample;
				}
				break;
		}

		if (tone_offset) {
			bp = block->data;
			end = bp + AUDIO_BLOCK_SAMPLES;
			do {
				val1 = *bp;
				*bp++ = signed_saturate_rshift(val1 + tone_offset, 16, 0);
			} while (bp < end);
		}
		/*
		if (shapedata)
			release(shapedata);
		transmit(block, 0);
		release(block);
		*/
	}

private:

	uint32_t phase_accumulator;
	uint32_t phase_increment;
	uint32_t modulation_factor;
	int32_t  magnitude;
	const int16_t* arbdata;
	uint32_t phasedata[AUDIO_BLOCK_SAMPLES];

	int16_t  sample; // for WAVEFORM_SAMPLE_HOLD
	int16_t  tone_offset;
	uint8_t  tone_type;
	uint8_t  modulation_type;

};

