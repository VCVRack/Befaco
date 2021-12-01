#pragma once

#include "NoisePlethoraPlugin.hpp"

class partialCluster : public NoisePlethoraPlugin {

public:

	partialCluster()
	// :
	// patchCord1(noise1, 0, waveform1, 0),
	// patchCord2(noise1, 0, waveform2, 0),
	// patchCord3(noise1, 0, waveform3, 0),
	// patchCord4(noise1, 0, waveform4, 0),
	// patchCord5(noise1, 0, waveform5, 0),
	// patchCord6(noise1, 0, waveform6, 0),
	// patchCord7(noise1, 0, waveform7, 0),
	// patchCord8(noise1, 0, waveform8, 0),
	// patchCord9(noise1, 0, waveform9, 0),
	// patchCord10(noise1, 0, waveform10, 0),
	// patchCord11(noise1, 0, waveform11, 0),
	// patchCord12(noise1, 0, waveform12, 0),
	// patchCord13(noise1, 0, waveform13, 0),
	// patchCord14(noise1, 0, waveform14, 0),
	// patchCord15(noise1, 0, waveform15, 0),
	// patchCord16(noise1, 0, waveform16, 0),
	// patchCord17(waveform16, 0, mixer4, 3),
	// patchCord18(waveform14, 0, mixer4, 1),
	// patchCord19(waveform15, 0, mixer4, 2),
	// patchCord20(waveform13, 0, mixer4, 0),
	// patchCord21(waveform8, 0, mixer2, 3),
	// patchCord22(waveform6, 0, mixer2, 1),
	// patchCord23(waveform7, 0, mixer2, 2),
	// patchCord24(waveform12, 0, mixer3, 3),
	// patchCord25(waveform5, 0, mixer2, 0),
	// patchCord26(waveform10, 0, mixer3, 1),
	// patchCord27(waveform11, 0, mixer3, 2),
	// patchCord28(waveform9, 0, mixer3, 0),
	// patchCord29(waveform4, 0, mixer1, 3),
	// patchCord30(waveform2, 0, mixer1, 1),
	// patchCord31(waveform3, 0, mixer1, 2),
	// patchCord32(waveform1, 0, mixer1, 0),
	// patchCord33(mixer4, 0, mixer5, 3),
	// patchCord34(mixer3, 0, mixer5, 2),
	// patchCord35(mixer2, 0, mixer5, 1),
	// patchCord36(mixer1, 0, mixer5, 0)
	{ }

	~partialCluster() override {}

	partialCluster(const partialCluster&) = delete;
	partialCluster& operator=(const partialCluster&) = delete;

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

		int masterWaveform = WAVEFORM_SAWTOOTH;
		float masterVolume = 0.25;

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

	}

	void process(float k1, float k2) override {
		float knob_1 = k1;
		float knob_2 = k2;
		float pitch1 = pow(knob_1, 2);
		// float pitch2 = pow(knob_2, 2);

		float spread = knob_2 * 1.1 + 1.01;

		float fundamental = 50 + pitch1 * 1000;

		float f1 = 1;
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


		waveform1.frequency(fundamental);
		waveform2.frequency(f2 * fundamental);
		waveform3.frequency(f3 * fundamental);
		waveform4.frequency(f4 * fundamental);
		waveform5.frequency(f5 * fundamental);
		waveform6.frequency(f6 * fundamental);
		waveform7.frequency(f7 * fundamental);
		waveform8.frequency(f8 * fundamental);
		waveform9.frequency(f9 * fundamental);
		waveform10.frequency(f10 * fundamental);
		waveform11.frequency(f11 * fundamental);
		waveform12.frequency(f12 * fundamental);
		waveform13.frequency(f13 * fundamental);
		waveform14.frequency(f14 * fundamental);
		waveform15.frequency(f15 * fundamental);
		waveform16.frequency(f16 * fundamental);

	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {

		noise1.update(&noiseOut);

		// FM from single noise source
		waveform1.update(&noiseOut, nullptr, &waveformOut[0]);
		waveform2.update(&noiseOut, nullptr, &waveformOut[1]);
		waveform3.update(&noiseOut, nullptr, &waveformOut[2]);
		waveform4.update(&noiseOut, nullptr, &waveformOut[3]);
		waveform5.update(&noiseOut, nullptr, &waveformOut[4]);
		waveform6.update(&noiseOut, nullptr, &waveformOut[5]);
		waveform7.update(&noiseOut, nullptr, &waveformOut[6]);
		waveform8.update(&noiseOut, nullptr, &waveformOut[7]);
		waveform9.update(&noiseOut, nullptr, &waveformOut[8]);
		waveform10.update(&noiseOut, nullptr, &waveformOut[9]);
		waveform11.update(&noiseOut, nullptr, &waveformOut[10]);
		waveform12.update(&noiseOut, nullptr, &waveformOut[11]);
		waveform13.update(&noiseOut, nullptr, &waveformOut[12]);
		waveform14.update(&noiseOut, nullptr, &waveformOut[13]);
		waveform15.update(&noiseOut, nullptr, &waveformOut[14]);
		waveform16.update(&noiseOut, nullptr, &waveformOut[15]);

		mixer1.update(&waveformOut[0], &waveformOut[1], &waveformOut[2], &waveformOut[3], &mixerOut[0]);
		mixer2.update(&waveformOut[4], &waveformOut[5], &waveformOut[6], &waveformOut[7], &mixerOut[1]);
		mixer3.update(&waveformOut[8], &waveformOut[9], &waveformOut[10], &waveformOut[11], &mixerOut[2]);
		mixer4.update(&waveformOut[12], &waveformOut[13], &waveformOut[14], &waveformOut[15], &mixerOut[3]);

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

	audio_block_t noiseOut, waveformOut[16] = {}, mixerOut[5] = {};

	AudioSynthNoiseWhite     noise1;         //xy=296.75,791.75
	AudioSynthWaveformModulated waveform16; //xy=581.75,1167.5
	AudioSynthWaveformModulated waveform14; //xy=583.75,1062.5
	AudioSynthWaveformModulated waveform15; //xy=583.75,1107.5
	AudioSynthWaveformModulated waveform13; //xy=584.75,1020
	AudioSynthWaveformModulated waveform8; //xy=591.75,747.5
	AudioSynthWaveformModulated waveform6; //xy=593.75,642.5
	AudioSynthWaveformModulated waveform7; //xy=593.75,687.5
	AudioSynthWaveformModulated waveform12; //xy=592.75,958.5
	AudioSynthWaveformModulated waveform5; //xy=594.75,600
	AudioSynthWaveformModulated waveform10; //xy=594.75,853.5
	AudioSynthWaveformModulated waveform11; //xy=594.75,898.5
	AudioSynthWaveformModulated waveform9; //xy=595.75,811
	AudioSynthWaveformModulated waveform4; //xy=599.75,532.25
	AudioSynthWaveformModulated waveform2; //xy=601.75,427.25
	AudioSynthWaveformModulated waveform3; //xy=601.75,472.25
	AudioSynthWaveformModulated waveform1;   //xy=602.75,384.75
	AudioMixer4              mixer4; //xy=801.75,1111.25
	AudioMixer4              mixer3; //xy=812.75,902.25
	AudioMixer4              mixer2; //xy=818.75,691.25
	AudioMixer4              mixer1;         //xy=819.75,476
	AudioMixer4              mixer5;         //xy=1066.75,688
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

REGISTER_PLUGIN(partialCluster);
