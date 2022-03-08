#pragma once

#include <rack.hpp>
#include "dspinst.h"

class AudioStream {
public:
	AudioStream(int num_inputs_) : num_inputs(num_inputs_) {}
	const int num_inputs;
};

#define AUDIO_BLOCK_SAMPLES  128

// even if rack sample rate is different, we don't want Teensy to behave differently
// w.r.t. aliasing etc - this generally used to put upper bounds on frequencies etc
#define AUDIO_SAMPLE_RATE_EXACT 44100.0f

typedef rack::dsp::RingBuffer<int16_t, AUDIO_BLOCK_SAMPLES> TeensyBuffer;

typedef struct audio_block_struct {
	// uint8_t  ref_count;
	// uint8_t  reserved1;
	// uint16_t memory_pool_index;
	int16_t  data[AUDIO_BLOCK_SAMPLES] = {};

	// initialises data to zeroes
	void zeroAudioBlock() {
		memset(data, 0, sizeof(int16_t) * AUDIO_BLOCK_SAMPLES);
	}

	static void copyBlock(const audio_block_struct* src, audio_block_struct* dst) {
		if (src && dst) {
			memcpy(&(dst->data), &(src->data), AUDIO_BLOCK_SAMPLES);
		}
	}
} audio_block_t;

enum WaveformType {
	WAVEFORM_SINE,
	WAVEFORM_SAWTOOTH,
	WAVEFORM_SQUARE,
	WAVEFORM_TRIANGLE,
	WAVEFORM_ARBITRARY,
	WAVEFORM_PULSE,
	WAVEFORM_SAWTOOTH_REVERSE,
	WAVEFORM_SAMPLE_HOLD,
	WAVEFORM_TRIANGLE_VARIABLE,
	WAVEFORM_BANDLIMIT_SAWTOOTH,
	WAVEFORM_BANDLIMIT_SAWTOOTH_REVERSE,
	WAVEFORM_BANDLIMIT_SQUARE,
	WAVEFORM_BANDLIMIT_PULSE,
};

const int16_t AudioWaveformSine[257] = {
	0,   804,  1608,  2410,  3212,  4011,  4808,  5602,  6393,  7179,
	7962,  8739,  9512, 10278, 11039, 11793, 12539, 13279, 14010, 14732,
	15446, 16151, 16846, 17530, 18204, 18868, 19519, 20159, 20787, 21403,
	22005, 22594, 23170, 23731, 24279, 24811, 25329, 25832, 26319, 26790,
	27245, 27683, 28105, 28510, 28898, 29268, 29621, 29956, 30273, 30571,
	30852, 31113, 31356, 31580, 31785, 31971, 32137, 32285, 32412, 32521,
	32609, 32678, 32728, 32757, 32767, 32757, 32728, 32678, 32609, 32521,
	32412, 32285, 32137, 31971, 31785, 31580, 31356, 31113, 30852, 30571,
	30273, 29956, 29621, 29268, 28898, 28510, 28105, 27683, 27245, 26790,
	26319, 25832, 25329, 24811, 24279, 23731, 23170, 22594, 22005, 21403,
	20787, 20159, 19519, 18868, 18204, 17530, 16846, 16151, 15446, 14732,
	14010, 13279, 12539, 11793, 11039, 10278,  9512,  8739,  7962,  7179,
	6393,  5602,  4808,  4011,  3212,  2410,  1608,   804,     0,  -804,
	-1608, -2410, -3212, -4011, -4808, -5602, -6393, -7179, -7962, -8739,
	-9512, -10278, -11039, -11793, -12539, -13279, -14010, -14732, -15446, -16151,
	-16846, -17530, -18204, -18868, -19519, -20159, -20787, -21403, -22005, -22594,
	-23170, -23731, -24279, -24811, -25329, -25832, -26319, -26790, -27245, -27683,
	-28105, -28510, -28898, -29268, -29621, -29956, -30273, -30571, -30852, -31113,
	-31356, -31580, -31785, -31971, -32137, -32285, -32412, -32521, -32609, -32678,
	-32728, -32757, -32767, -32757, -32728, -32678, -32609, -32521, -32412, -32285,
	-32137, -31971, -31785, -31580, -31356, -31113, -30852, -30571, -30273, -29956,
	-29621, -29268, -28898, -28510, -28105, -27683, -27245, -26790, -26319, -25832,
	-25329, -24811, -24279, -23731, -23170, -22594, -22005, -21403, -20787, -20159,
	-19519, -18868, -18204, -17530, -16846, -16151, -15446, -14732, -14010, -13279,
	-12539, -11793, -11039, -10278, -9512, -8739, -7962, -7179, -6393, -5602,
	-4808, -4011, -3212, -2410, -1608,  -804,     0
};


namespace teensy {

static uint32_t seed;

inline int32_t random_teensy(void) {
	int32_t hi, lo, x;

	// the algorithm used in avr-libc 1.6.4
	x = seed;
	if (x == 0)
		x = 123459876;
	hi = x / 127773;
	lo = x % 127773;
	x = 16807 * lo - 2836 * hi;
	if (x < 0)
		x += 0x7FFFFFFF;
	seed = x;
	return x;
}

inline uint32_t random_teensy(uint32_t howbig) {
	if (howbig == 0)
		return 0;
	return random_teensy() % howbig;
}

inline int32_t random_teensy(int32_t howsmall, int32_t howbig) {
	if (howsmall >= howbig)
		return howsmall;
	int32_t diff = howbig - howsmall;
	return random_teensy(diff) + howsmall;
}
}




class AudioSynthNoiseWhiteFloat : public AudioStream {
public:
	AudioSynthNoiseWhiteFloat() : AudioStream(0) { }

	void amplitude(float level) {
		level_ = level;
	}

	// uniform on [-1, 1]
	float process() {
		return level_ * ((rand31pm_next() / 2147483647.0) * 2.f - 1.f);
	}

	// uniform on [0, 1]
	float processNonnegative() {
		return level_ * (rand31pm_next() / 2147483647.0);
	}

	long unsigned int rand31pm_next() {
		double const a = 16807;
		double const m = 2147483647.0  ;

		return (seed31pm = (long)(fmod((seed31pm * a), m)));
	}

private:
	float level_ = 1.0;
	long unsigned int seed31pm  = 1;
};


class AudioSynthNoiseGritFloat : public AudioStream {
public:
	AudioSynthNoiseGritFloat() : AudioStream(0) { }

	void setDensity(float density) {
		density_ = density;
	}

	float process(float sampleTime) {

		float threshold = density_ * sampleTime;
		float scale = threshold > 0.f ? 2.f / threshold : 0.f;

		float z = white.processNonnegative();

		if (z < threshold) {
			return z * scale - 1.0f;
		}
		else {
			return 0.f;
		}
	}

private:

	float density_ = 0.f;

	AudioSynthNoiseWhiteFloat white;
};