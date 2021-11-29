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

		WaveformType masterWaveform = WAVEFORM_PULSE;
		float masterVolume = 0.7;

		waveform1.begin(simd::float_4(masterVolume), simd::float_4(794, 647, 524, 444), masterWaveform);
		waveform2.begin(simd::float_4(masterVolume, masterVolume, 0.f, 0.f), simd::float_4(368, 283, 0, 0), masterWaveform);
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

		waveform1.frequency(simd::float_4(f1, f2, f3, f4));
		waveform2.frequency(simd::float_4(f5, f6, 0.f, 0.f));
	}

	float processGraph(float sampleTime) override {
		float dc = dc1.process();
		simd::float_4 w1 = waveform1.process(sampleTime, 0.f, dc);
		simd::float_4 w2 = waveform2.process(sampleTime, 0.f, dc);

		float mix1 = mixer1.process(w1);
		float mix2 = mixer2.process(w2);

		altOutput = mix1;

		return mixer5.process(mix1, mix2, 0.f, 0.f);
	}

	AudioStream& getStream() override {
		return mixer5;
	}
	unsigned char getPort() override {
		return 0;
	}

private:
	AudioSynthWaveformDcFloat     dc1;            //xy=305.8888854980469,1069.1111450195312
	
	AudioSynthWaveformModulatedFloat4 waveform1;
	AudioSynthWaveformModulatedFloat4 waveform2;
	AudioMixer4Float mixer1;         //xy=708.8888854980469,634.1111450195312
	AudioMixer4Float mixer2;         //xy=719.8888854980469,789.1111450195312
	AudioMixer4Float mixer5;         //xy=955.8888854980469,846.1111450195312

	// AudioSynthWaveformModulated waveform6;      //xy=482.8888854980469,800.1111450195312
	// AudioSynthWaveformModulated waveform5;      //xy=483.8888854980469,758.1111450195312
	// AudioSynthWaveformModulated waveform4;      //xy=488.8888854980469,690.1111450195312
	// AudioSynthWaveformModulated waveform2;      //xy=490.8888854980469,585.1111450195312
	// AudioSynthWaveformModulated waveform3;      //xy=490.8888854980469,630.1111450195312
	// AudioSynthWaveformModulated waveform1;      //xy=491.8888854980469,542.1111450195312
	// AudioMixer4              mixer1;         //xy=708.8888854980469,634.1111450195312
	// AudioMixer4              mixer2;         //xy=719.8888854980469,789.1111450195312
	// AudioMixer4              mixer5;         //xy=955.8888854980469,846.1111450195312
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
