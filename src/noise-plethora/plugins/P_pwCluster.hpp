#pragma once

#include "NoisePlethoraPlugin.hpp"

class pwCluster : public NoisePlethoraPlugin {

public:

	pwCluster()
	//:
	//patchCord1(dc1, 0, waveform1, 1),
	//patchCord2(dc1, 0, waveform2, 1),
	//patchCord3(dc1, 0, waveform3, 1),
	//patchCord4(dc1, 0, waveform4, 1),
	//patchCord5(dc1, 0, waveform5, 1),
	//patchCord6(dc1, 0, waveform6, 1),
	//patchCord7(waveform6, 0, mixer2, 1),
	//patchCord8(waveform5, 0, mixer2, 0),
	//patchCord9(waveform4, 0, mixer1, 3),
	//patchCord10(waveform2, 0, mixer1, 1),
	//patchCord11(waveform3, 0, mixer1, 2),
	//patchCord12(waveform1, 0, mixer1, 0),
	//patchCord13(mixer1, 0, mixer5, 0),
	//patchCord14(mixer2, 0, mixer5, 1)
	{
		DEBUG("pwCluster initialised");
	}

	~pwCluster() override {
		DEBUG("pwCluster destroyed");
	}

	pwCluster(const pwCluster&) = delete;
	pwCluster& operator=(const pwCluster&) = delete;

	void init() override {

		mixer1.gain(0, 1);
		mixer1.gain(1, 1);
		mixer1.gain(2, 1);
		mixer1.gain(3, 1);
		mixer2.gain(0, 1);
		mixer2.gain(1, 1);
		mixer2.gain(2, 1);
		mixer2.gain(3, 1);
		mixer5.gain(0, 1);
		mixer5.gain(1, 1);
		mixer5.gain(2, 1);
		mixer5.gain(3, 1);

		int masterWaveform = WAVEFORM_PULSE;
		float masterVolume = 0.7;

		waveform1.begin(masterVolume, 794, masterWaveform);
		waveform2.begin(masterVolume, 647, masterWaveform);
		waveform3.begin(masterVolume, 524, masterWaveform);
		waveform4.begin(masterVolume, 444, masterWaveform);
		waveform5.begin(masterVolume, 368, masterWaveform);
		waveform6.begin(masterVolume, 283, masterWaveform);
	}

	void process(float k1, float k2) override {
		float knob_1 = k1;
		float knob_2 = k2;
		float pitch1 = std::pow(knob_1, 2);

		float f1 = 40 + pitch1 * 8000;
		float f2 = f1 * 1.227;
		float f3 = f2 * 1.24;
		float f4 = f3 * 1.17;
		float f5 = f4 * 1.2;
		float f6 = f5 * 1.3;
		dc1.amplitude(1 - (knob_2 * 0.97));

		waveform1.frequency(f1);
		waveform2.frequency(f2);
		waveform3.frequency(f3);
		waveform4.frequency(f4);
		waveform5.frequency(f5);
		waveform6.frequency(f6);
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {
		dc1.update(&dcOut);

		// pulsewidth from dc1 for the 6 oscillators
		waveform1.update(nullptr, &dcOut, &waveformOut[0]);
		waveform2.update(nullptr, &dcOut, &waveformOut[1]);
		waveform3.update(nullptr, &dcOut, &waveformOut[2]);
		waveform4.update(nullptr, &dcOut, &waveformOut[3]);
		waveform5.update(nullptr, &dcOut, &waveformOut[4]);
		waveform6.update(nullptr, &dcOut, &waveformOut[5]);

		mixer1.update(&waveformOut[0], &waveformOut[1], &waveformOut[2], &waveformOut[3], &mixerOut[0]);
		mixer2.update(&waveformOut[4], &waveformOut[5], nullptr, nullptr, &mixerOut[1]);
		mixer5.update(&mixerOut[0], &mixerOut[1], nullptr, nullptr, &mixerOut[2]);

		blockBuffer.pushBuffer(mixerOut[2].data, AUDIO_BLOCK_SAMPLES);
	}

	AudioStream& getStream() override {
		return mixer5;
	}
	unsigned char getPort() override {
		return 0;
	}

private:
	audio_block_t dcOut, waveformOut[6] = {}, mixerOut[3] = {};

	AudioSynthWaveformDc     dc1;            //xy=305.8888854980469,1069.1111450195312
	AudioSynthWaveformModulated waveform6;      //xy=482.8888854980469,800.1111450195312
	AudioSynthWaveformModulated waveform5;      //xy=483.8888854980469,758.1111450195312
	AudioSynthWaveformModulated waveform4;      //xy=488.8888854980469,690.1111450195312
	AudioSynthWaveformModulated waveform2;      //xy=490.8888854980469,585.1111450195312
	AudioSynthWaveformModulated waveform3;      //xy=490.8888854980469,630.1111450195312
	AudioSynthWaveformModulated waveform1;      //xy=491.8888854980469,542.1111450195312
	AudioMixer4              mixer1;         //xy=708.8888854980469,634.1111450195312
	AudioMixer4              mixer2;         //xy=719.8888854980469,789.1111450195312
	AudioMixer4              mixer5;         //xy=955.8888854980469,846.1111450195312

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

};

REGISTER_PLUGIN(pwCluster);
