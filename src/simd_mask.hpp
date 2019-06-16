#pragma once

#include "rack.hpp"


struct ChannelMask {

	rack::simd::float_4 mask[4];

    ChannelMask() {
        
		__m128i tmp =  _mm_cmpeq_epi16(_mm_set_epi32(0,0,0,0),_mm_set_epi32(0,0,0,0));
		
		for(int i=0; i<4; i++) {
			mask[3-i] = simd::float_4(_mm_castsi128_ps(tmp));
			tmp = _mm_srli_si128(tmp, 4);
		}
        
    };

    ~ChannelMask() {};

    inline const rack::simd::float_4 operator[](const int c) {return  mask[c];}

	inline void apply(simd::float_4 *vec, int numChannels) {
		int c=numChannels/4;
		vec[c] = vec[c]&mask[numChannels-4*c];
	}

	inline void apply_all(simd::float_4 *vec, int numChannels) {
		int c=numChannels/4;
		vec[c] = vec[c]&mask[numChannels-4*c];
		for(int i=c+1; i<4; i++) vec[i] = simd::float_4::zero();
	}


};

inline void load_input(Input &in, simd::float_4 *v, int numChannels) {
	int inChannels = in.getChannels();
	if(inChannels==1) {
		for(int i=0; i<numChannels; i++) v[i] = simd::float_4(in.getVoltage());
	} else {
		for(int c=0; c<inChannels; c+=4) v[c/4] = simd::float_4::load(in.getVoltages(c));
	}
}

inline void add_input(Input &in, simd::float_4 *v, int numChannels) {
	int inChannels = in.getChannels();
	if(inChannels==1) {
		for(int i=0; i<numChannels; i++) v[i] += simd::float_4(in.getVoltage());
	} else {
		for(int c=0; c<inChannels; c+=4) v[c/4] += simd::float_4::load(in.getVoltages(c));
	}
}

inline simd::float_4 crossfade_4(simd::float_4 a, simd::float_4 b, simd::float_4 p) {
        return a + (b - a) * p;
}
