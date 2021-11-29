#pragma once

#include "NoisePlethoraPlugin.hpp"

class clusterSaw : public NoisePlethoraPlugin {

public:

	clusterSaw()
		// : patchCord18(waveform16, 0, mixer4, 3),
		//   patchCord19(waveform14, 0, mixer4, 1),
		//   patchCord20(waveform15, 0, mixer4, 2),
		//   patchCord21(waveform13, 0, mixer4, 0),
		//   patchCord22(waveform8, 0, mixer2, 3),
		//   patchCord23(waveform6, 0, mixer2, 1),
		//   patchCord24(waveform7, 0, mixer2, 2),
		//   patchCord25(waveform12, 0, mixer3, 3),
		//   patchCord26(waveform5, 0, mixer2, 0),
		//   patchCord27(waveform10, 0, mixer3, 1),
		//   patchCord28(waveform11, 0, mixer3, 2),
		//   patchCord29(waveform9, 0, mixer3, 0),
		//   patchCord30(waveform4, 0, mixer1, 3),
		//   patchCord31(waveform2, 0, mixer1, 1),
		//   patchCord32(waveform3, 0, mixer1, 2),
		//   patchCord33(waveform1, 0, mixer1, 0),
		//   patchCord34(mixer4, 0, mixer5, 3),
		//   patchCord35(mixer2, 0, mixer5, 1),
		//   patchCord36(mixer1, 0, mixer5, 0),
		//   patchCord37(mixer3, 0, mixer5, 2)
	{ }

	~clusterSaw() override {}

	clusterSaw(const clusterSaw&) = delete;
	clusterSaw& operator=(const clusterSaw&) = delete;

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
		WaveformType masterWaveform = WAVEFORM_SAWTOOTH;
		float masterVolume = 0.25;
		waveform1.begin(masterVolume, 0, masterWaveform);
		waveform2.begin(masterVolume, 0, masterWaveform);
		waveform3.begin(masterVolume, 0, masterWaveform);
		waveform4.begin(masterVolume, 0, masterWaveform);	
	}

	void process(float k1, float k2) override {
		float knob_1 = k1;
		float knob_2 = k2;
		float pitch1 = pow(knob_1, 2);
		float pitch2 = pow(knob_2, 2);
		float multFactor = 1.01 + (pitch2 * 0.9);
		float f1 = 20 + pitch1 * 1000;
		float f2 = f1 * multFactor;
		float f3 = f2 * multFactor;
		float f4 = f3 * multFactor;
		float f5 = f4 * multFactor;
		float f6 = f5 * multFactor;
		float f7 = f6 * multFactor;
		float f8 = f7 * multFactor;
		float f9 = f8 * multFactor;
		float f10 = f9 * multFactor;
		float f11 = f10 * multFactor;
		float f12 = f11 * multFactor;
		float f13 = f12 * multFactor;
		float f14 = f13 * multFactor;
		float f15 = f14 * multFactor;
		float f16 = f15 * multFactor;
		
		waveform1.frequency(simd::float_4(f1, f2, f3, f4));
		waveform2.frequency(simd::float_4(f5, f6, f7, f8));
		waveform3.frequency(simd::float_4(f9, f10, f11, f12));
		waveform4.frequency(simd::float_4(f13, f14, f15, f16));		
	}

	float processGraph(float sampleTime) override {
		
		const float mix1 = mixer1.process(waveform1.process(sampleTime));		                     
		const float mix2 = mixer2.process(waveform2.process(sampleTime));			                     
		const float mix3 = mixer3.process(waveform3.process(sampleTime));
		const float mix4 = mixer4.process(waveform4.process(sampleTime));
		
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
	// AudioSynthWaveformModulatedFloat waveform16;     //xy=360.00000381469727,1486.0000076293945
	// AudioSynthWaveformModulatedFloat waveform14;     //xy=362.00000381469727,1381.0000076293945
	// AudioSynthWaveformModulatedFloat waveform15;     //xy=362.00000381469727,1426.0000076293945
	// AudioSynthWaveformModulatedFloat waveform13;     //xy=363.00000381469727,1339.0000076293945
	// AudioSynthWaveformModulatedFloat waveform8;      //xy=370.00000381469727,1066.0000076293945
	// AudioSynthWaveformModulatedFloat waveform6;      //xy=372.00000381469727,961.0000076293945
	// AudioSynthWaveformModulatedFloat waveform7;      //xy=372.00000381469727,1006.0000076293945
	// AudioSynthWaveformModulatedFloat waveform12;     //xy=371.00000381469727,1277.0000076293945
	// AudioSynthWaveformModulatedFloat waveform5;      //xy=373.00000381469727,919.0000076293945
	// AudioSynthWaveformModulatedFloat waveform10;     //xy=373.00000381469727,1172.0000076293945
	// AudioSynthWaveformModulatedFloat waveform11;     //xy=373.00000381469727,1217.0000076293945
	// AudioSynthWaveformModulatedFloat waveform9;      //xy=374.00000381469727,1130.0000076293945
	AudioSynthWaveformModulatedFloat4 waveform4;      //xy=378.00000381469727,851.0000076293945
	AudioSynthWaveformModulatedFloat4 waveform2;      //xy=380.00000381469727,746.0000076293945
	AudioSynthWaveformModulatedFloat4 waveform3;      //xy=380.00000381469727,791.0000076293945
	AudioSynthWaveformModulatedFloat4 waveform1;      //xy=381.00000381469727,703.0000076293945
	AudioMixer4Float              mixer4;         //xy=546.0000038146973,1408.0000381469727
	AudioMixer4Float              mixer2;         //xy=554.9999923706055,984.0000381469727
	AudioMixer4Float              mixer1;         //xy=556.9999885559082,771.0000076293945
	AudioMixer4Float              mixer3;         //xy=557.0000114440918,1196.0000228881836
	AudioMixer4Float              mixer5;         //xy=755.0000267028809,1082.9999923706055
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
	// AudioConnection          patchCord37;


};

REGISTER_PLUGIN(clusterSaw);
