#pragma once

#include "rack.hpp"


inline void load_input(Input &in, simd::float_4 *v, int numChannels) {
	int inChannels = in.getChannels();
	if (inChannels == 1) {
		for (int i = 0; i < numChannels; i++)
			v[i] = simd::float_4(in.getVoltage());
	}
	else {
		for (int c = 0; c < inChannels; c += 4)
			v[c / 4] = simd::float_4::load(in.getVoltages(c));
	}
}

inline void add_input(Input &in, simd::float_4 *v, int numChannels) {
	int inChannels = in.getChannels();
	if (inChannels == 1) {
		for (int i = 0; i < numChannels; i++)
			v[i] += simd::float_4(in.getVoltage());
	}
	else {
		for (int c = 0; c < inChannels; c += 4)
			v[c / 4] += simd::float_4::load(in.getVoltages(c));
	}
}
