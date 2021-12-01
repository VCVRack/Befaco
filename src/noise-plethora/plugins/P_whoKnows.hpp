#pragma once

#include "NoisePlethoraPlugin.hpp"

class whoKnows : public NoisePlethoraPlugin {

public:

	whoKnows()
	// : patchCord1(waveform1, 0, filter1, 0)
	// , patchCord2(waveform1, 0, filter2, 0)
	// , patchCord3(waveform1, 0, filter3, 0)
	// , patchCord4(waveform1, 0, filter4, 0)
	// , patchCord5(waveformMod1, 0, filter1, 1)
	// , patchCord6(waveformMod2, 0, filter2, 1)
	// , patchCord7(waveformMod4, 0, filter4, 1)
	// , patchCord8(waveformMod3, 0, filter3, 1)
	// , patchCord9(filter1, 1, mixer1, 0)
	// , patchCord10(filter2, 1, mixer1, 1)
	// , patchCord11(filter3, 1, mixer1, 2)
	// , patchCord12(filter4, 1, mixer1, 3)
	{ }

	~whoKnows() override {}

	// delete copy constructors
	whoKnows(const whoKnows&) = delete;
	whoKnows& operator=(const whoKnows&) = delete;

	void init() override {

		//noise1.amplitude(1);

		mixer1.gain(0, 0.8);
		mixer1.gain(1, 0.8);
		mixer1.gain(2, 0.8);
		mixer1.gain(3, 0.8);


		waveformMod1.begin(1, 21, WAVEFORM_TRIANGLE);
		waveformMod2.begin(1, 70, WAVEFORM_TRIANGLE);
		waveformMod3.begin(1, 90, WAVEFORM_TRIANGLE);
		waveformMod4.begin(1, 77, WAVEFORM_TRIANGLE);

		waveform1.begin(1, 5, WAVEFORM_PULSE);
		waveform1.pulseWidth(0.1);

		filter1.resonance(7);
		filter1.octaveControl(7);

		filter2.resonance(7);
		filter2.octaveControl(7);

		filter3.resonance(7);
		filter3.octaveControl(7);

		filter4.resonance(7);
		filter4.octaveControl(7);
	}

	void process(float k1, float k2) override {


		float knob_1 = k1;
		float knob_2 = k2;

		float pitch1 = pow(knob_1, 2);
		// float pitch2 = pow(knob_2, 2);


		waveform1.frequency(15 + (pitch1 * 500));

		float octaves = knob_2 * 6 + 0.3;

		/*filter1.resonance(resonanceLevel);
		filter2.resonance(resonanceLevel);
		filter3.resonance(resonanceLevel);
		filter4.resonance(resonanceLevel);*/

		filter1.octaveControl(octaves);
		filter2.octaveControl(octaves);
		filter3.octaveControl(octaves);
		filter4.octaveControl(octaves);

	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {

		// waveform to filter
		waveform1.update(&waveformOut);

		waveformMod1.update(nullptr, nullptr, &waveformModOut[0]);
		waveformMod2.update(nullptr, nullptr, &waveformModOut[1]);
		waveformMod3.update(nullptr, nullptr, &waveformModOut[2]);
		waveformMod4.update(nullptr, nullptr, &waveformModOut[3]);

		filter1.update(&waveformOut, &waveformModOut[0], &filterOutLP[0], &filterOutBP[0], &filterOutHP[0]);
		filter2.update(&waveformOut, &waveformModOut[1], &filterOutLP[1], &filterOutBP[1], &filterOutHP[1]);
		filter3.update(&waveformOut, &waveformModOut[2], &filterOutLP[2], &filterOutBP[2], &filterOutHP[2]);
		filter4.update(&waveformOut, &waveformModOut[3], &filterOutLP[3], &filterOutBP[3], &filterOutHP[3]);

		// sum up
		mixer1.update(&filterOutBP[0], &filterOutBP[1], &filterOutBP[2], &filterOutBP[3], &mixerOut);

		blockBuffer.pushBuffer(mixerOut.data, AUDIO_BLOCK_SAMPLES);
	}


	AudioStream& getStream() override {
		return mixer1;
	}
	unsigned char getPort() override {
		return 0;
	}

private:


	audio_block_t filterOutLP[4], filterOutBP[4], filterOutHP[4], waveformModOut[4], waveformOut, mixerOut;


	// GUItool: begin automatically generated code
	AudioSynthWaveform       waveform1;      //xy=283,566
	AudioSynthWaveformModulated waveformMod1;   //xy=363.888916015625,321.2221794128418
	AudioSynthWaveformModulated waveformMod2; //xy=368,380.3332633972168
	AudioSynthWaveformModulated waveformMod4; //xy=374,689.3332633972168
	AudioSynthWaveformModulated waveformMod3; //xy=378,638.3332633972168
	AudioFilterStateVariable filter1;        //xy=578.888916015625,420.8889274597168
	AudioFilterStateVariable filter2; //xy=579,486.0000114440918
	AudioFilterStateVariable filter3; //xy=581,549.0000114440918
	AudioFilterStateVariable filter4; //xy=581,614.0000114440918
	AudioMixer4              mixer1;         //xy=752,520.0000114440918
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

};

REGISTER_PLUGIN(whoKnows); // this is important, so that we can include the plugin in a bank
