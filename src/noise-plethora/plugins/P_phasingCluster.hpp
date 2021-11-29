#pragma once

#include "NoisePlethoraPlugin.hpp"

class phasingCluster : public NoisePlethoraPlugin {

public:

	phasingCluster()
		// : patchCord1(modulator13, 0, waveform13, 0),
		//   patchCord2(modulator14, 0, waveform14, 0),
		//   patchCord3(modulator15, 0, waveform15, 0),
		//   patchCord4(modulator2, 0, waveform2, 0),
		//   patchCord5(modulator4, 0, waveform4, 0),
		//   patchCord6(modulator1, 0, waveform1, 0),
		//   patchCord7(modulator16, 0, waveform16, 0),
		//   patchCord8(modulator12, 0, waveform12, 0),
		//   patchCord9(modulator11, 0, waveform11, 0),
		//   patchCord10(modulator3, 0, waveform3, 0),
		//   patchCord11(modulator5, 0, waveform5, 0),
		//   patchCord12(modulator6, 0, waveform6, 0),
		//   patchCord13(modulator7, 0, waveform7, 0),
		//   patchCord14(modulator10, 0, waveform10, 0),
		//   patchCord15(modulator8, 0, waveform8, 0),
		//   patchCord16(modulator9, 0, waveform9, 0),
		//   patchCord17(waveform16, 0, mixer4, 3),
		//   patchCord18(waveform14, 0, mixer4, 1),
		//   patchCord19(waveform15, 0, mixer4, 2),
		//   patchCord20(waveform13, 0, mixer4, 0),
		//   patchCord21(waveform8, 0, mixer2, 3),
		//   patchCord22(waveform6, 0, mixer2, 1),
		//   patchCord23(waveform7, 0, mixer2, 2),
		//   patchCord24(waveform12, 0, mixer3, 3),
		//   patchCord25(waveform5, 0, mixer2, 0),
		//   patchCord26(waveform10, 0, mixer3, 1),
		//   patchCord27(waveform11, 0, mixer3, 2),
		//   patchCord28(waveform9, 0, mixer3, 0),
		//   patchCord29(waveform4, 0, mixer1, 3),
		//   patchCord30(waveform2, 0, mixer1, 1),
		//   patchCord31(waveform3, 0, mixer1, 2),
		//   patchCord32(waveform1, 0, mixer1, 0),
		//   patchCord33(mixer4, 0, mixer5, 3),
		//   patchCord34(mixer3, 0, mixer5, 2),
		//   patchCord35(mixer2, 0, mixer5, 1),
		//   patchCord36(mixer1, 0, mixer5, 0)
	{ }

	~phasingCluster() override {}

	phasingCluster(const phasingCluster&) = delete;
	phasingCluster& operator=(const phasingCluster&) = delete;

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

		WaveformType masterWaveform = WAVEFORM_SQUARE;
		float masterVolume = 0.1;
		WaveformType indexWaveform = WAVEFORM_TRIANGLE;
		float index = 0.005;

		waveform4_1.begin(masterVolume, simd::float_4(794, 647, 524, 444), masterWaveform);
		waveform4_2.begin(masterVolume, simd::float_4(368, 283, 283, 283), masterWaveform);
		waveform4_3.begin(masterVolume, simd::float_4(283, 283, 283, 283), masterWaveform);
		waveform4_4.begin(masterVolume, simd::float_4(283, 283, 283, 283), masterWaveform);

		modulator4_1.begin(index, simd::float_4(10, 11, 15, 1), indexWaveform);
		modulator4_2.begin(index, simd::float_4(1, 3, 17, 14), indexWaveform);
		modulator4_3.begin(index, simd::float_4(0.11, 5, 2, 7), indexWaveform);
		modulator4_4.begin(index, simd::float_4(1, 0.1, 0.7, 0.5), indexWaveform);
	}

	void process(float k1, float k2) override {
		float knob_1 = k1;
		float knob_2 = k2;
		float pitch1 = pow(knob_1, 2);


		float spread = knob_2 * 0.5 + 1;

		float f1 = 30 + pitch1 * 5000;
		float f2 = f1 * spread;
		float f3 = f2 * spread;
		float f4 = f3 * spread;
		float f5 = f4 * spread;
		float f6 = f5 * spread;
		float f7 = f6 * spread;
		float f8 = f7 * spread;
		float f9 = f8 * spread;
		float f10 = f9 * spread;
		float f11 = f10 * spread;
		float f12 = f11 * spread;
		float f13 = f12 * spread;
		float f14 = f13 * spread;
		float f15 = f14 * spread;
		float f16 = f15 * spread;

		waveform4_1.frequency(simd::float_4(f1, f2, f3, f4));
		waveform4_2.frequency(simd::float_4(f5, f6, f7, f8));
		waveform4_3.frequency(simd::float_4(f9, f10, f11, f12));
		waveform4_4.frequency(simd::float_4(f13, f14, f15, f16));
	}

	float processGraph(float sampleTime) override {
		
		float mix1 = mixer1.process(waveform4_1.process(sampleTime, modulator4_1.process(sampleTime)));
		float mix2 = mixer2.process(waveform4_2.process(sampleTime, modulator4_2.process(sampleTime)));
		float mix3 = mixer3.process(waveform4_3.process(sampleTime, modulator4_3.process(sampleTime)));
		float mix4 = mixer4.process(waveform4_4.process(sampleTime, modulator4_4.process(sampleTime)));

		altOutput = mix1;

		return mixer5.process(mix1, mix2, mix3, mix4);
	}

	AudioStream& getStream() override {
		return mixer5;
	}
	unsigned char getPort() override {
		return 0;
	}

private:
	AudioSynthWaveformFloat4 modulator4_1;
	AudioSynthWaveformFloat4 modulator4_2;
	AudioSynthWaveformFloat4 modulator4_3;
	AudioSynthWaveformFloat4 modulator4_4;
		
	AudioSynthWaveformModulatedFloat4 waveform4_1;
	AudioSynthWaveformModulatedFloat4 waveform4_2;
	AudioSynthWaveformModulatedFloat4 waveform4_3;
	AudioSynthWaveformModulatedFloat4 waveform4_4;
	AudioMixer4Float       mixer4;
	AudioMixer4Float       mixer3;
	AudioMixer4Float       mixer2;
	AudioMixer4Float       mixer1;
	AudioMixer4Float       mixer5;


	// AudioSynthWaveform       modulator13; //xy=331.3333435058594,888.6666870117188
	// AudioSynthWaveform       modulator14; //xy=331.3333435058594,935.6666870117188
	// AudioSynthWaveform       modulator15; //xy=331.3333435058594,976.6666870117188
	// AudioSynthWaveform       modulator2; //xy=335.3333435058594,243.6666717529297
	// AudioSynthWaveform       modulator4; //xy=336.3333435058594,349.6666564941406
	// AudioSynthWaveform       modulator1;     //xy=337.33335876464844,198.6666660308838
	// AudioSynthWaveform       modulator16; //xy=334.3333435058594,1026.6666259765625
	// AudioSynthWaveform       modulator12; //xy=335.3333435058594,823.6666870117188
	// AudioSynthWaveform       modulator11;  //xy=337.3333435058594,753.6666870117188
	// AudioSynthWaveform       modulator3; //xy=339.3333435058594,293.6666564941406
	// AudioSynthWaveform       modulator5; //xy=339.3333435058594,414.6666564941406
	// AudioSynthWaveform       modulator6; //xy=341.3333435058594,467.6666564941406
	// AudioSynthWaveform       modulator7; //xy=344.3333435058594,522.6666870117188
	// AudioSynthWaveform       modulator10; //xy=344.3333435058594,687.6666870117188
	// AudioSynthWaveform       modulator8; //xy=346.3333435058594,569.6666870117188
	// AudioSynthWaveform       modulator9; //xy=350.3333435058594,624.6666870117188
	// AudioSynthWaveformModulated waveform16;     //xy=532.3333282470703,986.8888931274414
	// AudioSynthWaveformModulated waveform14;     //xy=534.3333282470703,881.8888931274414
	// AudioSynthWaveformModulated waveform15;     //xy=534.3333282470703,926.8888931274414
	// AudioSynthWaveformModulated waveform13;     //xy=535.3333282470703,839.8888931274414
	// AudioSynthWaveformModulated waveform8;      //xy=542.3333282470703,566.8888931274414
	// AudioSynthWaveformModulated waveform6;      //xy=544.3333282470703,461.8888931274414
	// AudioSynthWaveformModulated waveform7;      //xy=544.3333282470703,506.8888931274414
	// AudioSynthWaveformModulated waveform12;     //xy=543.3333282470703,777.8888931274414
	// AudioSynthWaveformModulated waveform5;      //xy=545.3333282470703,419.8888931274414
	// AudioSynthWaveformModulated waveform10;     //xy=545.3333282470703,672.8888931274414
	// AudioSynthWaveformModulated waveform11;     //xy=545.3333282470703,717.8888931274414
	// AudioSynthWaveformModulated waveform9;      //xy=546.3333282470703,630.8888931274414
	// AudioSynthWaveformModulated waveform4;      //xy=550.3333282470703,351.8888931274414
	// AudioSynthWaveformModulated waveform2;      //xy=552.3333282470703,246.8888931274414
	// AudioSynthWaveformModulated waveform3;      //xy=552.3333282470703,291.8888931274414
	// AudioSynthWaveformModulated waveform1;      //xy=553.3333282470703,203.8888931274414
	// AudioMixer4              mixer4;         //xy=752.3333282470703,930.8888931274414
	// AudioMixer4              mixer3;         //xy=763.3333282470703,721.8888931274414
	// AudioMixer4              mixer2;         //xy=769.3333282470703,510.8888931274414
	// AudioMixer4              mixer1;         //xy=770.3333282470703,295.8888931274414
	// AudioMixer4              mixer5;         //xy=1017.3333282470703,507.8888931274414
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

REGISTER_PLUGIN(phasingCluster);
