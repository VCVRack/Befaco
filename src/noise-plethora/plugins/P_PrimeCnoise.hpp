
#pragma once

#include "NoisePlethoraPlugin.hpp"

class PrimeCnoise  : public NoisePlethoraPlugin {

public:

	PrimeCnoise()
		// : patchCord1(noise1, 0, waveform1, 0)
		// , patchCord2(noise1, 0, waveform2, 0)
		// , patchCord3(noise1, 0, waveform3, 0)
		// , patchCord4(noise1, 0, waveform4, 0)
		// , patchCord5(noise1, 0, waveform5, 0)
		// , patchCord6(noise1, 0, waveform6, 0)
		// , patchCord7(noise1, 0, waveform7, 0)
		// , patchCord8(noise1, 0, waveform8, 0)
		// , patchCord9(noise1, 0, waveform9, 0)
		// , patchCord10(noise1, 0, waveform10, 0)
		// , patchCord11(noise1, 0, waveform11, 0)
		// , patchCord12(noise1, 0, waveform12, 0)
		// , patchCord13(noise1, 0, waveform13, 0)
		// , patchCord14(noise1, 0, waveform14, 0)
		// , patchCord15(noise1, 0, waveform15, 0)
		// , patchCord16(noise1, 0, waveform16, 0)
		// , patchCord17(waveform16, 0, mixer4, 3)
		// , patchCord18(waveform14, 0, mixer4, 1)
		// , patchCord19(waveform15, 0, mixer4, 2)
		// , patchCord20(waveform13, 0, mixer4, 0)
		// , patchCord21(waveform8, 0, mixer2, 3)
		// , patchCord22(waveform6, 0, mixer2, 1)
		// , patchCord23(waveform7, 0, mixer2, 2)
		// , patchCord24(waveform12, 0, mixer3, 3)
		// , patchCord25(waveform5, 0, mixer2, 0)
		// , patchCord26(waveform10, 0, mixer3, 1)
		// , patchCord27(waveform11, 0, mixer3, 2)
		// , patchCord28(waveform9, 0, mixer3, 0)
		// , patchCord29(waveform4, 0, mixer1, 3)
		// , patchCord30(waveform2, 0, mixer1, 1)
		// , patchCord31(waveform3, 0, mixer1, 2)
		// , patchCord32(waveform1, 0, mixer1, 0)
		// , patchCord33(mixer4, 0, mixer5, 3)
		// , patchCord34(mixer3, 0, mixer5, 2)
		// , patchCord35(mixer2, 0, mixer5, 1)
		// , patchCord36(mixer1, 0, mixer5, 0)
	{}

	~PrimeCnoise() override {}

	// delete copy constructors
	PrimeCnoise(const PrimeCnoise&) = delete;
	PrimeCnoise& operator=(const PrimeCnoise&) = delete;

	void init() override {

		mixer1.gain(0, 1);
		mixer1.gain(1, 1);
		mixer1.gain(2, 1);
		mixer1.gain(3, 1);
		mixer2.gain(0, 1);
		mixer2.gain(1, 1);
		mixer2.gain(2, 1);
		mixer2.gain(3, 1);
		mixer3.gain(0, 1);
		mixer3.gain(1, 1);
		mixer3.gain(2, 1);
		mixer3.gain(3, 1);
		mixer4.gain(0, 1);
		mixer4.gain(1, 1);
		mixer4.gain(2, 1);
		mixer4.gain(3, 1);
		mixer5.gain(0, 1);
		mixer5.gain(1, 1);
		mixer5.gain(2, 1);
		mixer5.gain(3, 1);

		WaveformType masterWaveform = WAVEFORM_TRIANGLE_VARIABLE;
		float masterVolume = 100;

		waveform4_1.begin(masterVolume, simd::float_4(200, 647, 524, 444), masterWaveform);
		waveform4_2.begin(masterVolume, simd::float_4(368, 283, 283, 283), masterWaveform);
		waveform4_3.begin(masterVolume, simd::float_4(283, 283, 283, 283), masterWaveform);
		waveform4_4.begin(masterVolume, simd::float_4(283, 283, 283, 283), masterWaveform);
	}


	void process(float k1, float k2) override {

		float knob_1 = k1;
		float knob_2 = k2;

		float pitch1 = pow(knob_1, 2);

		float multfactor = pitch1 * 12 + 0.5;

		waveform4_1.frequency(simd::float_4(53 * multfactor, 127 * multfactor, 199 * multfactor, 283 * multfactor));
		waveform4_2.frequency(simd::float_4(383 * multfactor, 467 * multfactor, 577 * multfactor, 661 * multfactor));
		waveform4_3.frequency(simd::float_4(769 * multfactor, 877 * multfactor, 983 * multfactor, 1087 * multfactor));
		waveform4_4.frequency(simd::float_4(1193 * multfactor, 1297 * multfactor, 1429 * multfactor, 1523 * multfactor));

		noise1.amplitude(knob_2 * 0.2);
	}

	float processGraph(float sampleTime) override {
		float noise = noise1.process();

		float mix1 = mixer1.process(waveform4_1.process(sampleTime, noise));
		float mix2 = mixer2.process(waveform4_2.process(sampleTime, noise));
		float mix3 = mixer3.process(waveform4_3.process(sampleTime, noise));
		float mix4 = mixer4.process(waveform4_4.process(sampleTime, noise));

		altOutput = noise;

		return mixer5.process(mix1, mix2, mix3, mix4);
	}

	AudioStream& getStream() override {
		return mixer5;
	}
	unsigned char getPort() override {
		return 0;
	}

private:

	AudioSynthNoiseWhiteFloat     noise1;
	AudioSynthWaveformModulatedFloat4 waveform4_1;
	AudioSynthWaveformModulatedFloat4 waveform4_2;
	AudioSynthWaveformModulatedFloat4 waveform4_3;
	AudioSynthWaveformModulatedFloat4 waveform4_4;
	AudioMixer4Float       mixer4;
	AudioMixer4Float       mixer3;
	AudioMixer4Float       mixer2;
	AudioMixer4Float       mixer1;
	AudioMixer4Float       mixer5;

	// AudioSynthWaveformModulated waveform16;
	// AudioSynthWaveformModulated waveform14;
	// AudioSynthWaveformModulated waveform15;
	// AudioSynthWaveformModulated waveform13;
	// AudioSynthWaveformModulated waveform8;
	// AudioSynthWaveformModulated waveform6;
	// AudioSynthWaveformModulated waveform7;
	// AudioSynthWaveformModulated waveform12;
	// AudioSynthWaveformModulated waveform5;
	// AudioSynthWaveformModulated waveform10;
	// AudioSynthWaveformModulated waveform11;
	// AudioSynthWaveformModulated waveform9;
	// AudioSynthWaveformModulated waveform4;
	// AudioSynthWaveformModulated waveform2;
	// AudioSynthWaveformModulated waveform3;
	// AudioSynthWaveformModulated waveform1;
	// AudioMixer4       mixer4;
	// AudioMixer4       mixer3;
	// AudioMixer4       mixer2;
	// AudioMixer4       mixer1;
	// AudioMixer4       mixer5;
	//
	//
	// AudioConnection          patchCord1;
	// AudioConnection          patchCord2;
	// AudioConnection          patchCord3;
	// AudioConnection          patchCord4;
	// AudioConnection          patchCord5;
	// AudioConnection          patchCord6;
	// AudioConnection          patchCord7;
	// AudioConnection          patchCord8;
	// AudioConnection          patchCord9;
	// AudioConnection          patchCord10;
	// AudioConnection          patchCord11;
	// AudioConnection          patchCord12;
	// AudioConnection          patchCord13;
	// AudioConnection          patchCord14;
	// AudioConnection          patchCord15;
	// AudioConnection          patchCord16;
	// AudioConnection          patchCord17;
	// AudioConnection          patchCord18;
	// AudioConnection          patchCord19;
	// AudioConnection          patchCord20;
	// AudioConnection          patchCord21;
	// AudioConnection          patchCord22;
	// AudioConnection          patchCord23;
	// AudioConnection          patchCord24;
	// AudioConnection          patchCord25;
	// AudioConnection          patchCord26;
	// AudioConnection          patchCord27;
	// AudioConnection          patchCord28;
	// AudioConnection          patchCord29;
	// AudioConnection          patchCord30;
	// AudioConnection          patchCord31;
	// AudioConnection          patchCord32;
	// AudioConnection          patchCord33;
	// AudioConnection          patchCord34;
	// AudioConnection          patchCord35;
	// AudioConnection          patchCord36;
};

REGISTER_PLUGIN(PrimeCnoise);
