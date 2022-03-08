#pragma once

#include "NoisePlethoraPlugin.hpp"

class TeensyAlt : public NoisePlethoraPlugin {

public:

	TeensyAlt()
	//:patchCord1(waveformMod1, 0, waveformMod2, 1),
	//patchCord3(waveformMod2, 0, waveformMod1, 0)
	{ }

	~TeensyAlt() override {}

	TeensyAlt(const TeensyAlt&) = delete;
	TeensyAlt& operator=(const TeensyAlt&) = delete;

	void init() override {

		waveform1.begin(1, 200, WAVEFORM_SAMPLE_HOLD);
		filter1.resonance(5);

		DEBUG("init finished");
	}

	void process(float k1, float k2) override {

		float knob_1 = k1;
		float knob_2 = k2;
		(void) knob_2;

		float pitch1 = pow(knob_1, 2);
		// float pitch2 = pow(knob_2, 2);

		waveform1.frequency(50 + (pitch1 * 5000));
	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {

		waveform1.update(&output1);
		filter1.update(&output1, nullptr, &filterOutLP, &filterOutBP, &filterOutHP);

		blockBuffer.pushBuffer(filterOutBP.data, AUDIO_BLOCK_SAMPLES);
	}

	AudioStream& getStream() override {
		return filter1;
	}
	unsigned char getPort() override {
		return 0;
	}

private:
	audio_block_t output1 = {};
	audio_block_t filterOutLP, filterOutBP, filterOutHP;


	AudioSynthWaveform       waveform1;      //xy=829.0908737182617,499.54540252685547
	AudioFilterStateVariable filter1;        //xy=1062.2726001739502,460.8181266784668

};

REGISTER_PLUGIN(TeensyAlt);
