#pragma once

#include "NoisePlethoraPlugin.hpp"

class clusterSaw_int16 : public NoisePlethoraPlugin {

public:

	clusterSaw_int16()
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

	~clusterSaw_int16() override {}

	clusterSaw_int16(const clusterSaw_int16&) = delete;
	clusterSaw_int16& operator=(const clusterSaw_int16&) = delete;

	void init() override {
		mixer[0].gain(0, 1);
		mixer[0].gain(1, 1);
		mixer[0].gain(2, 1);
		mixer[0].gain(3, 1);
		mixer[1].gain(0, 1);
		mixer[1].gain(1, 1);
		mixer[1].gain(2, 1);
		mixer[1].gain(3, 1);
		mixer[2].gain(0, 1);
		mixer[2].gain(1, 1);
		mixer[2].gain(2, 1);
		mixer[2].gain(3, 1);
		mixer[3].gain(0, 1);
		mixer[3].gain(1, 1);
		mixer[3].gain(2, 1);
		mixer[3].gain(3, 1);
		mixer[4].gain(0, 1);
		mixer[4].gain(1, 1);
		mixer[4].gain(2, 1);
		mixer[4].gain(3, 1);

		WaveformType masterWaveform = WAVEFORM_SAWTOOTH;
		float masterVolume = 0.25;
		waveforms[0].begin(masterVolume, 0, masterWaveform);
		waveforms[1].begin(masterVolume, 0, masterWaveform);
		waveforms[2].begin(masterVolume, 0, masterWaveform);
		waveforms[3].begin(masterVolume, 0, masterWaveform);
		waveforms[4].begin(masterVolume, 0, masterWaveform);
		waveforms[5].begin(masterVolume, 0, masterWaveform);
		waveforms[6].begin(masterVolume, 0, masterWaveform);
		waveforms[7].begin(masterVolume, 0, masterWaveform);
		waveforms[8].begin(masterVolume, 0, masterWaveform);
		waveforms[9].begin(masterVolume, 0, masterWaveform);
		waveforms[10].begin(masterVolume, 0, masterWaveform);
		waveforms[11].begin(masterVolume, 0, masterWaveform);
		waveforms[12].begin(masterVolume, 0, masterWaveform);
		waveforms[13].begin(masterVolume, 0, masterWaveform);
		waveforms[14].begin(masterVolume, 0, masterWaveform);
		waveforms[15].begin(masterVolume, 0, masterWaveform);

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
		waveforms[0].frequency(f1);
		waveforms[1].frequency(f2);
		waveforms[2].frequency(f3);
		waveforms[3].frequency(f4);
		waveforms[4].frequency(f5);
		waveforms[5].frequency(f6);
		waveforms[6].frequency(f7);
		waveforms[7].frequency(f8);
		waveforms[8].frequency(f9);
		waveforms[9].frequency(f10);
		waveforms[10].frequency(f11);
		waveforms[11].frequency(f12);
		waveforms[12].frequency(f13);
		waveforms[13].frequency(f14);
		waveforms[14].frequency(f15);
		waveforms[15].frequency(f16);
	}

	float processGraph(float sampleTime) override {

		if (buffer.empty()) {
			// update waveforms
			for (int i = 0; i < 16; ++i) {
				waveforms[i].update(nullptr, nullptr, &outputs[i]);
			}

			// update mixers			
			for (int i = 0; i < 4; ++i) {
				mixer[i].update(
				  &outputs[(i * 4) + 0],
				  &outputs[(i * 4) + 1],
				  &outputs[(i * 4) + 2],
				  &outputs[(i * 4) + 3],
				  &mixOut[i]);
			}

			mixer[4].update(&mixOut[0], &mixOut[1], &mixOut[2], &mixOut[3], &mixOut[4]);
			buffer.pushBuffer(mixOut[4].data, AUDIO_BLOCK_SAMPLES);
		}

		float mix1Out = int16_to_float_5v(buffer.shift()) / 5.f;

		return mix1Out;
	}

	AudioStream& getStream() override {
		return mixer[4];
	}
	unsigned char getPort() override {
		return 0;
	}

private:
	// AudioSynthWaveformModulated waveform16;     //xy=360.00000381469727,1486.0000076293945
	// AudioSynthWaveformModulated waveform14;     //xy=362.00000381469727,1381.0000076293945
	// AudioSynthWaveformModulated waveform15;     //xy=362.00000381469727,1426.0000076293945
	// AudioSynthWaveformModulated waveform13;     //xy=363.00000381469727,1339.0000076293945
	// AudioSynthWaveformModulated waveform8;      //xy=370.00000381469727,1066.0000076293945
	// AudioSynthWaveformModulated waveform6;      //xy=372.00000381469727,961.0000076293945
	// AudioSynthWaveformModulated waveform7;      //xy=372.00000381469727,1006.0000076293945
	// AudioSynthWaveformModulated waveform12;     //xy=371.00000381469727,1277.0000076293945
	// AudioSynthWaveformModulated waveform5;      //xy=373.00000381469727,919.0000076293945
	// AudioSynthWaveformModulated waveform10;     //xy=373.00000381469727,1172.0000076293945
	// AudioSynthWaveformModulated waveform11;     //xy=373.00000381469727,1217.0000076293945
	// AudioSynthWaveformModulated waveform9;      //xy=374.00000381469727,1130.0000076293945
	// AudioSynthWaveformModulated waveform4;      //xy=378.00000381469727,851.0000076293945
	// AudioSynthWaveformModulated waveform2;      //xy=380.00000381469727,746.0000076293945
	// AudioSynthWaveformModulated waveform3;      //xy=380.00000381469727,791.0000076293945
	// AudioSynthWaveformModulated waveform1;      //xy=381.00000381469727,703.0000076293945

	AudioSynthWaveformModulated waveforms[16];
	AudioMixer4 mixer[5];
	// AudioMixer4              mixer4;         //xy=546.0000038146973,1408.0000381469727
	// AudioMixer4              mixer2;         //xy=554.9999923706055,984.0000381469727
	// AudioMixer4              mixer1;         //xy=556.9999885559082,771.0000076293945
	// AudioMixer4              mixer3;         //xy=557.0000114440918,1196.0000228881836
	// AudioMixer4              mixer5;         //xy=755.0000267028809,1082.9999923706055
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

	audio_block_t mixOut[5];
	audio_block_t outputs[16] = {};
	TeensyBuffer buffer;

	int cycle = 0;
};

REGISTER_PLUGIN(clusterSaw_int16);
