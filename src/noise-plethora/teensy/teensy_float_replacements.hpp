#pragma once

#include <rack.hpp>
#include "audio_core.hpp"
#include "../../plugin.hpp"


template<class T>
struct AudioSynthWaveformT : public AudioStream {

	AudioSynthWaveformT() : AudioStream(0) {  }

	void begin(T t_amp, T t_freq, WaveformType t_type) {
		amplitude(t_amp);
		frequency(t_freq);
		begin(t_type);
	}

	void begin(WaveformType tone_type) {
		tone_type_ = tone_type;
	}
	void frequency(T frequency) {
		frequency_ = simd::fmax(0.f, frequency);
	}
	void amplitude(T amplitude) {
		amplitude_ = simd::clamp(amplitude, 0.f, 1.f);
	}
	void offset(T offset) {
		offset_ = simd::clamp(offset, -1.f, +1.f);
	}
	void pulseWidth(T pulse_width) {
		pulse_width_ = pulse_width;
	}

	// angle in degrees
	void phase(T angle) {
		phase_ = angle / 360.;
	}

	T process(float sampleTime) {

		const T phaseInc = simd::clamp(sampleTime * frequency_, 1e-6f, maxTeensyPhaseInc);
		phase_ += phaseInc;
		phase_ -= simd::floor(phase_);

		T signal = 0.f;
		switch (tone_type_) {
			case WAVEFORM_SINE: {
				signal = sin2pi_pade_05_5_4(phase_);
				break;
			}

			case WAVEFORM_SQUARE: {
				signal = simd::ifelse(phase_ < 0.5, 1.f, -1.f);
				break;
			}

			case WAVEFORM_PULSE: {
				signal = simd::ifelse(phase_ < pulse_width_, 1.f, -1.f);
				break;
			}

			case WAVEFORM_TRIANGLE: {
				T p = phase_ + 0.25f;
				signal = 4.f * simd::fabs(p - simd::round(p)) - 1.f;
				break;
			}

			default: {}
		}

		return simd::clamp(offset_ + amplitude_ * signal, -1.f, +1.f);
	}

private:

	T frequency_ = 20.f;
	T phase_ = 0.f;
	T amplitude_ = 1.f;
	T pulse_width_ = 0.5f;

	WaveformType tone_type_;
	T offset_ = 0.f;
	// BandLimitedWaveform band_limit_waveform ;

	const float maxTeensyPhaseInc = 2147352576. / 4294967295.;
};

typedef AudioSynthWaveformT<float> AudioSynthWaveformFloat;
typedef AudioSynthWaveformT<simd::float_4> AudioSynthWaveformFloat4;





template<class T>
struct AudioSynthWaveformModulatedT : public AudioStream {

	AudioSynthWaveformModulatedT(void) : AudioStream(2) {
	}

	void begin(WaveformType t_type) {
		tone_type_ = t_type;
	}

	void begin(T t_amp, T t_freq, WaveformType t_type) {
		amplitude(t_amp);
		frequency(t_freq);
		begin(t_type);
	}

	void frequency(T frequency) {
		frequency_ = simd::clamp(frequency, 0.f, 44100.0f / 2.0f);
	}
	void amplitude(T amplitude) {
		amplitude_ = simd::clamp(amplitude, 0.f, 1.f);
	}
	void offset(T offset) {
		offset_ = simd::clamp(offset, -1.f, +1.f);
	}

	// specified in octaves
	void frequencyModulation(T frequencyModulation) {
		frequencyModulation_ = simd::clamp(frequencyModulation, 0.1f, 12.f);
	}

	void arbitraryWaveform(const int16_t* data, float maxFreq) {
		// convert to float
		for (int i = 0; i < 256; ++i) {
			arbitraryData[i] = data[i] / 32767.0;
		}
	}

	// float specific implementation of WAVEFORM_ARBITRARY
	template <class Q = T>
	typename std::enable_if<std::is_same<Q, float>::value, float>::type processArbitraryData(float phase) {
		// teensy buffer len = 256
		float phaseIndex = phase * 256;
		int index = std::floor(phaseIndex);
		int index2 = index + 1;
		if (index2 >= 256)
			index2 = 0;
		float difference = phaseIndex - index;

		float val1 = *(arbitraryData + index) * difference;
		float val2 = *(arbitraryData + index2) * (1.f - difference);

		return val1 + val2;				
	}

	// simd::float_4 specific implementation of WAVEFORM_ARBITRARY
	template <class Q = T>
	typename std::enable_if<std::is_same<Q, simd::float_4>::value, simd::float_4>::type processArbitraryData(simd::float_4 phase) {

		// teensy buffer len = 256
		simd::float_4 phaseIndex = phase * 256;
		simd::float_4 index = simd::floor(phaseIndex);
		simd::float_4 index2 = index + 1;

		index2 = simd::fmod(index2, 256);
		
		simd::float_4 difference = phaseIndex - index;

		simd::float_4 vals_1(arbitraryData[(int) index[0]], arbitraryData[(int) index[1]], arbitraryData[(int) index[2]], arbitraryData[(int) index[3]]);
		simd::float_4 vals_2(arbitraryData[(int) index2[0]], arbitraryData[(int) index2[1]], arbitraryData[(int) index2[2]], arbitraryData[(int) index2[3]]);

		vals_1 = vals_1 * difference;
		vals_2 = vals_2 * (1.f - difference);

		return vals_1 + vals_2;		
	}

	T process(float sampleTime, T frequencyModulationSignal = 0.f, T pulsewidthModulationSignal = 0.f) {

		const T basePhaseIncrement = simd::clamp(frequency_ * sampleTime, 0.f, maxTeensyPhaseInc);
		const T pitch = frequencyModulation_ * simd::clamp(frequencyModulationSignal, -1.f, 1.f);
		const T dFreq = dsp::approxExp2_taylor5(pitch + 20.f) / std::pow(2.f, 20.f);
		const T modifiedFrequency = dFreq * basePhaseIncrement;

		const T phaseInc = simd::clamp(modifiedFrequency, 0.f, maxTeensyPhaseInc);
		phase += phaseInc;
		phase -= simd::floor(phase);

		T pulse_width_mod = pulse_width_ + 0.5 * clamp(pulsewidthModulationSignal, -1.0, 1.0);
		pulse_width_mod = simd::clamp(pulse_width_mod, minTeensyPulseWidth, 1.f - minTeensyPulseWidth);

		T signal = 0.f;
		switch (tone_type_) {
			case WAVEFORM_SINE: {
				signal = sin2pi_pade_05_5_4(phase);
				break;
			}

			case WAVEFORM_ARBITRARY: {				
				signal = processArbitraryData(phase);
				break;
			}

			case WAVEFORM_SQUARE: {
				signal = simd::ifelse(phase < 0.5, 1.f, -1.f);
				break;
			}

			case WAVEFORM_PULSE: {
				signal = simd::ifelse(phase < pulse_width_mod, 1.f, -1.f);
				break;
			}

			case WAVEFORM_SAWTOOTH: {
				signal = 2.f * phase - 1.f;
				break;
			}

			case WAVEFORM_TRIANGLE: {
				T p = phase + 0.25f;
				signal = 4.f * simd::fabs(p - simd::round(p)) - 1.f;
				break;
			}

			case WAVEFORM_TRIANGLE_VARIABLE: {
				T upslope = 1.f / pulse_width_mod;
				T downslope = 1.f / (1.f - pulse_width_mod);
				signal = simd::ifelse(phase < pulse_width_mod, phase * upslope, 1.f - (phase - pulse_width_mod) * downslope);
				signal = 2.f * signal - 1.f;
				break;
			}

			default: {}

		}

		return simd::clamp(offset_ + amplitude_ * signal, -1.f, +1.f);
	}

private:
	float arbitraryData[256] = {};
	T frequencyModulation_ = 8.f;

	T frequency_ = 20.f;
	T phase = 0.f;
	T phase_offset_ = 0.f;
	T amplitude_ = 1.f;
	T pulse_width_ = 0.5f;

	WaveformType tone_type_;
	float offset_ = 0.f;
	// BandLimitedWaveform band_limit_waveform ;

	const float minTeensyPulseWidth = 65535. / 4294967295.;
	const float maxTeensyPhaseInc = 2147352576. / 4294967295.;
};

typedef AudioSynthWaveformModulatedT<float> AudioSynthWaveformModulatedFloat;
typedef AudioSynthWaveformModulatedT<simd::float_4> AudioSynthWaveformModulatedFloat4;


class AudioSynthWaveformSineModulatedFloat : public AudioStream {
public:
	AudioSynthWaveformSineModulatedFloat() : AudioStream(1) {}

	// maximum unmodulated carrier frequency is 11025 Hz
	void frequency(float freq) {
		// use Teensy sample rate to be reproducable
		freq_ = clamp(freq, 0.f, AUDIO_SAMPLE_RATE_EXACT / 4.0f);
	}
	void phase(float angle) {
		if (angle < 0.0f)
			angle = 0.0;
		else if (angle > 360.0f) {
			angle = angle - 360.0f;
			if (angle >= 360.0f)
				return;
		}
		phase_ = angle / 360.0;
	}
	void amplitude(float n) {
		magnitude_ = clamp(n, 0.f, 1.f);
	}

	float process(float sampleTime, float frequencyModulationSignal = 0.f) {

		const float phase_increment = freq_ * sampleTime;

		// FM:
		// frequencyModulationSignal in [-1, +1]
		// * case +1 is doubling of carrier frequency
		// * case -1 is DC
		phase_ += phase_increment + phase_increment * frequencyModulationSignal;
		// wraparound
		phase_ -= std::floor(phase_);

		// use cheaper sin approximant
		return magnitude_ * sin2pi_pade_05_5_4(phase_);
	}

private:
	float freq_ = 0.f;
	float phase_ = 0.f;
	float magnitude_ = 0.2f; 	// teensy default is 0.2
};


class AudioSynthWaveformDcFloat : public AudioStream {
public:
	AudioSynthWaveformDcFloat() : AudioStream(0), magnitude(0) { }
	// immediately jump to the new DC level
	void amplitude(float n) {
		magnitude = clamp(n, -1.f, +1.f);
	}
	// slowly transition to the new DC level
	void amplitude(float n, float milliseconds) {
		// not implemented
	}

	float process() {
		return magnitude;
	}
private:

	// uint8_t  state;     // 0=steady output, 1=transitioning
	float  magnitude; // current output
	// int32_t  target;    // designed output (while transitiong)
	// int32_t  increment; // adjustment per sample (while transitiong)
};




class AudioEffectMultiplyFloat : public AudioStream {
public:
	AudioEffectMultiplyFloat() : AudioStream(2) { }

	float process(float input1, float input2) {
		return clamp(input1 * input2, -1.f, +1.f);
	}
};


class AudioMixer4Float : public AudioStream {

public:
	AudioMixer4Float(void) : AudioStream(4) { }

	float process(float input1, float input2, float input3, float input4) {
		// teensy mixer introduces clippping during each add!

		float result = clamp(input1 * multiplier[0], -1.f, +1.f);
		result = clamp(result + input2 * multiplier[1], -1.f, +1.f);
		result = clamp(result + input3 * multiplier[2], -1.f, +1.f);
		result = clamp(result + input4 * multiplier[3], -1.f, +1.f);

		return result;
	}

	float process(simd::float_4 input) {
		return process(input[0], input[1], input[2], input[3]);
	}

	void gain(unsigned int channel, float gain) {
		if (channel >= 4)
			return;
		multiplier[channel] = clamp(gain, -1.f, +1.f);
	}
private:
	float multiplier[4] = {1.f, 1.f, 1.f, 1.f};

};

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