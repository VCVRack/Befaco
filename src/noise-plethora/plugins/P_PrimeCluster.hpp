#pragma once

#include "NoisePlethoraPlugin.hpp"

class PrimeCluster : public NoisePlethoraPlugin {

public:

	PrimeCluster()
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

	~PrimeCluster() override {}

	PrimeCluster(const PrimeCluster&) = delete;
	PrimeCluster& operator=(const PrimeCluster&) = delete;

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

		int masterWaveform = WAVEFORM_TRIANGLE_VARIABLE;
		float masterVolume = 0.3;

		waveform1.begin(masterVolume, 200, masterWaveform);
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
		float multfactor = k1 * 10 + 0.5;

		waveform1.frequency(53 * multfactor);
		waveform2.frequency(127 * multfactor);
		waveform3.frequency(199 * multfactor);
		waveform4.frequency(283 * multfactor);
		waveform5.frequency(383 * multfactor);
		waveform6.frequency(467 * multfactor);
		waveform7.frequency(577 * multfactor);
		waveform8.frequency(661 * multfactor);
		waveform9.frequency(769 * multfactor);
		waveform10.frequency(877 * multfactor);
		waveform11.frequency(983 * multfactor);
		waveform12.frequency(1087 * multfactor);
		waveform13.frequency(1193 * multfactor);
		waveform14.frequency(1297 * multfactor);
		waveform15.frequency(1429 * multfactor);
		waveform16.frequency(1523 * multfactor);

		noise1.amplitude(k2 * 0.2);
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

	AudioSynthNoiseWhite     noise1;         //xy=306.20001220703125,530
	AudioSynthWaveformModulated waveform16;     //xy=591.2000122070312,906
	AudioSynthWaveformModulated waveform14;     //xy=593.2000122070312,801
	AudioSynthWaveformModulated waveform15;     //xy=593.2000122070312,846
	AudioSynthWaveformModulated waveform13;     //xy=594.2000122070312,759
	AudioSynthWaveformModulated waveform8;      //xy=601.2000122070312,486
	AudioSynthWaveformModulated waveform6;      //xy=603.2000122070312,381
	AudioSynthWaveformModulated waveform7;      //xy=603.2000122070312,426
	AudioSynthWaveformModulated waveform12;     //xy=602.2000122070312,697
	AudioSynthWaveformModulated waveform5;      //xy=604.2000122070312,339
	AudioSynthWaveformModulated waveform10;     //xy=604.2000122070312,592
	AudioSynthWaveformModulated waveform11;     //xy=604.2000122070312,637
	AudioSynthWaveformModulated waveform9;      //xy=605.2000122070312,550
	AudioSynthWaveformModulated waveform4;      //xy=609.2000122070312,271
	AudioSynthWaveformModulated waveform2;      //xy=611.2000122070312,166
	AudioSynthWaveformModulated waveform3;      //xy=611.2000122070312,211
	AudioSynthWaveformModulated waveform1;      //xy=612.2000122070312,123
	AudioMixer4              mixer4;         //xy=811.2000122070312,850
	AudioMixer4              mixer3;         //xy=822.2000122070312,641
	AudioMixer4              mixer2;         //xy=828.2000122070312,430
	AudioMixer4              mixer1;         //xy=829.2000122070312,215
	AudioMixer4              mixer5;         //xy=1076.2000122070312,427

	// AudioSynthWaveformModulated waveform16; //xy=581.75,1167.5
	// AudioSynthWaveformModulated waveform14; //xy=583.75,1062.5
	// AudioSynthWaveformModulated waveform15; //xy=583.75,1107.5
	// AudioSynthWaveformModulated waveform13; //xy=584.75,1020
	// AudioSynthWaveformModulated waveform8; //xy=591.75,747.5
	// AudioSynthWaveformModulated waveform6; //xy=593.75,642.5
	// AudioSynthWaveformModulated waveform7; //xy=593.75,687.5
	// AudioSynthWaveformModulated waveform12; //xy=592.75,958.5
	// AudioSynthWaveformModulated waveform5; //xy=594.75,600
	// AudioSynthWaveformModulated waveform10; //xy=594.75,853.5
	// AudioSynthWaveformModulated waveform11; //xy=594.75,898.5
	// AudioSynthWaveformModulated waveform9; //xy=595.75,811
	// AudioSynthWaveformModulated waveform4; //xy=599.75,532.25
	// AudioSynthWaveformModulated waveform2; //xy=601.75,427.25
	// AudioSynthWaveformModulated waveform3; //xy=601.75,472.25
	// AudioSynthWaveformModulated waveform1;   //xy=602.75,384.75
	// AudioMixer4              mixer4; //xy=801.75,1111.25
	// AudioMixer4              mixer3; //xy=812.75,902.25
	// AudioMixer4              mixer2; //xy=818.75,691.25
	// AudioMixer4              mixer1;         //xy=819.75,476
	// AudioMixer4              mixer5;         //xy=1066.75,688
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

REGISTER_PLUGIN(PrimeCluster);