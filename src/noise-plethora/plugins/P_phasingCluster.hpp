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

		int masterWaveform = WAVEFORM_SQUARE;
		float masterVolume = 0.1;
		int indexWaveform = WAVEFORM_TRIANGLE;
		float index = 0.005;

		waveform1.begin(masterVolume, 794, masterWaveform);
		waveform2.begin(masterVolume, 647, masterWaveform);
		waveform3.begin(masterVolume, 524, masterWaveform);
		waveform4.begin(masterVolume, 444, masterWaveform);
		waveform5.begin(masterVolume, 368, masterWaveform);
		waveform6.begin(masterVolume, 283, masterWaveform);
		waveform7.begin(masterVolume, 283, masterWaveform);
		waveform8.begin(masterVolume, 283, masterWaveform);
		waveform9.begin(masterVolume, 283, masterWaveform);
		waveform10.begin(masterVolume, 283, masterWaveform);
		waveform11.begin(masterVolume, 283, masterWaveform);
		waveform12.begin(masterVolume, 283, masterWaveform);
		waveform13.begin(masterVolume, 283, masterWaveform);
		waveform14.begin(masterVolume, 283, masterWaveform);
		waveform15.begin(masterVolume, 283, masterWaveform);
		waveform16.begin(masterVolume, 283, masterWaveform);

		modulator1.begin(index, 10, indexWaveform);
		modulator2.begin(index, 11, indexWaveform);
		modulator3.begin(index, 15, indexWaveform);
		modulator4.begin(index, 1, indexWaveform);
		modulator5.begin(index, 1, indexWaveform);
		modulator6.begin(index, 3, indexWaveform);
		modulator7.begin(index, 17, indexWaveform);
		modulator8.begin(index, 14, indexWaveform);
		modulator9.begin(index, 0.11, indexWaveform);
		modulator10.begin(index, 5, indexWaveform);
		modulator11.begin(index, 2, indexWaveform);
		modulator12.begin(index, 7, indexWaveform);
		modulator13.begin(index, 1, indexWaveform);
		modulator14.begin(index, 0.1, indexWaveform);
		modulator15.begin(index, 0.7, indexWaveform);
		modulator16.begin(index, 0.5, indexWaveform);
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

		waveform1.frequency(f1);
		waveform2.frequency(f2);
		waveform3.frequency(f3);
		waveform4.frequency(f4);
		waveform5.frequency(f5);
		waveform6.frequency(f6);
		waveform7.frequency(f7);
		waveform8.frequency(f8);
		waveform9.frequency(f9);
		waveform10.frequency(f10);
		waveform11.frequency(f11);
		waveform12.frequency(f12);
		waveform13.frequency(f13);
		waveform14.frequency(f14);
		waveform15.frequency(f15);
		waveform16.frequency(f16);
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {

		// first update modulators
		modulator1.update(&waveformOut[0]);
		modulator2.update(&waveformOut[1]);
		modulator3.update(&waveformOut[2]);
		modulator4.update(&waveformOut[3]);
		modulator5.update(&waveformOut[4]);
		modulator6.update(&waveformOut[5]);
		modulator7.update(&waveformOut[6]);
		modulator8.update(&waveformOut[7]);
		modulator9.update(&waveformOut[8]);
		modulator10.update(&waveformOut[9]);
		modulator11.update(&waveformOut[10]);
		modulator12.update(&waveformOut[11]);
		modulator13.update(&waveformOut[12]);
		modulator14.update(&waveformOut[13]);
		modulator15.update(&waveformOut[14]);
		modulator16.update(&waveformOut[15]);

		// FM from each of the modulators
		waveform1.update(&waveformOut[0], nullptr, &waveformModOut[0]);
		waveform2.update(&waveformOut[1], nullptr, &waveformModOut[1]);
		waveform3.update(&waveformOut[2], nullptr, &waveformModOut[2]);
		waveform4.update(&waveformOut[3], nullptr, &waveformModOut[3]);
		waveform5.update(&waveformOut[4], nullptr, &waveformModOut[4]);
		waveform6.update(&waveformOut[5], nullptr, &waveformModOut[5]);
		waveform7.update(&waveformOut[6], nullptr, &waveformModOut[6]);
		waveform8.update(&waveformOut[7], nullptr, &waveformModOut[7]);
		waveform9.update(&waveformOut[8], nullptr, &waveformModOut[8]);
		waveform10.update(&waveformOut[9], nullptr, &waveformModOut[9]);
		waveform11.update(&waveformOut[10], nullptr, &waveformModOut[10]);
		waveform12.update(&waveformOut[11], nullptr, &waveformModOut[11]);
		waveform13.update(&waveformOut[12], nullptr, &waveformModOut[12]);
		waveform14.update(&waveformOut[13], nullptr, &waveformModOut[13]);
		waveform15.update(&waveformOut[14], nullptr, &waveformModOut[14]);
		waveform16.update(&waveformOut[15], nullptr, &waveformModOut[15]);

		mixer1.update(&waveformModOut[0], &waveformModOut[1], &waveformModOut[2], &waveformModOut[3], &mixerOut[0]);
		mixer2.update(&waveformModOut[4], &waveformModOut[5], &waveformModOut[6], &waveformModOut[7], &mixerOut[1]);
		mixer3.update(&waveformModOut[8], &waveformModOut[9], &waveformModOut[10], &waveformModOut[11], &mixerOut[2]);
		mixer4.update(&waveformModOut[12], &waveformModOut[13], &waveformModOut[14], &waveformModOut[15], &mixerOut[3]);

		mixer5.update(&mixerOut[0], &mixerOut[1], &mixerOut[2], &mixerOut[3], &mixerOut[4]);

		blockBuffer.pushBuffer(mixerOut[4].data, AUDIO_BLOCK_SAMPLES);
	}

	AudioStream& getStream() override {
		return mixer5;
	}
	unsigned char getPort() override {
		return 0;
	}

private:

	audio_block_t waveformOut[16] = {}, waveformModOut[16] = {}, mixerOut[5] = {};

	AudioSynthWaveform       modulator13; //xy=331.3333435058594,888.6666870117188
	AudioSynthWaveform       modulator14; //xy=331.3333435058594,935.6666870117188
	AudioSynthWaveform       modulator15; //xy=331.3333435058594,976.6666870117188
	AudioSynthWaveform       modulator2; //xy=335.3333435058594,243.6666717529297
	AudioSynthWaveform       modulator4; //xy=336.3333435058594,349.6666564941406
	AudioSynthWaveform       modulator1;     //xy=337.33335876464844,198.6666660308838
	AudioSynthWaveform       modulator16; //xy=334.3333435058594,1026.6666259765625
	AudioSynthWaveform       modulator12; //xy=335.3333435058594,823.6666870117188
	AudioSynthWaveform       modulator11;  //xy=337.3333435058594,753.6666870117188
	AudioSynthWaveform       modulator3; //xy=339.3333435058594,293.6666564941406
	AudioSynthWaveform       modulator5; //xy=339.3333435058594,414.6666564941406
	AudioSynthWaveform       modulator6; //xy=341.3333435058594,467.6666564941406
	AudioSynthWaveform       modulator7; //xy=344.3333435058594,522.6666870117188
	AudioSynthWaveform       modulator10; //xy=344.3333435058594,687.6666870117188
	AudioSynthWaveform       modulator8; //xy=346.3333435058594,569.6666870117188
	AudioSynthWaveform       modulator9; //xy=350.3333435058594,624.6666870117188
	AudioSynthWaveformModulated waveform16;     //xy=532.3333282470703,986.8888931274414
	AudioSynthWaveformModulated waveform14;     //xy=534.3333282470703,881.8888931274414
	AudioSynthWaveformModulated waveform15;     //xy=534.3333282470703,926.8888931274414
	AudioSynthWaveformModulated waveform13;     //xy=535.3333282470703,839.8888931274414
	AudioSynthWaveformModulated waveform8;      //xy=542.3333282470703,566.8888931274414
	AudioSynthWaveformModulated waveform6;      //xy=544.3333282470703,461.8888931274414
	AudioSynthWaveformModulated waveform7;      //xy=544.3333282470703,506.8888931274414
	AudioSynthWaveformModulated waveform12;     //xy=543.3333282470703,777.8888931274414
	AudioSynthWaveformModulated waveform5;      //xy=545.3333282470703,419.8888931274414
	AudioSynthWaveformModulated waveform10;     //xy=545.3333282470703,672.8888931274414
	AudioSynthWaveformModulated waveform11;     //xy=545.3333282470703,717.8888931274414
	AudioSynthWaveformModulated waveform9;      //xy=546.3333282470703,630.8888931274414
	AudioSynthWaveformModulated waveform4;      //xy=550.3333282470703,351.8888931274414
	AudioSynthWaveformModulated waveform2;      //xy=552.3333282470703,246.8888931274414
	AudioSynthWaveformModulated waveform3;      //xy=552.3333282470703,291.8888931274414
	AudioSynthWaveformModulated waveform1;      //xy=553.3333282470703,203.8888931274414
	AudioMixer4              mixer4;         //xy=752.3333282470703,930.8888931274414
	AudioMixer4              mixer3;         //xy=763.3333282470703,721.8888931274414
	AudioMixer4              mixer2;         //xy=769.3333282470703,510.8888931274414
	AudioMixer4              mixer1;         //xy=770.3333282470703,295.8888931274414
	AudioMixer4              mixer5;         //xy=1017.3333282470703,507.8888931274414

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
