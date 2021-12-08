#pragma once

#include "NoisePlethoraPlugin.hpp"

#define FLANGE_DELAY_LENGTH (2*AUDIO_BLOCK_SAMPLES)

class Rwalk_SineFMFlange : public NoisePlethoraPlugin {

public:

	Rwalk_SineFMFlange()
	// : patchCord1(waveform4, sine_fm3)
	// , patchCord2(waveform3, sine_fm4)
	// , patchCord3(waveform2, sine_fm2)
	// , patchCord4(waveform1, sine_fm1)
	// , patchCord5(sine_fm3, 0, mixer1, 3)
	// , patchCord6(sine_fm2, 0, mixer1, 1)
	// , patchCord7(sine_fm1, 0, mixer1, 0)
	// , patchCord8(sine_fm4, 0, mixer1, 2)
	// , patchCord9(mixer1, flange1)
	{ }

	~Rwalk_SineFMFlange() override {}

	// delete copy constructors
	Rwalk_SineFMFlange(const Rwalk_SineFMFlange&) = delete;
	Rwalk_SineFMFlange& operator=(const Rwalk_SineFMFlange&) = delete;

	void init() override {

		L = 600; // Size of box: maximum frequency
		v_0 = 30; // speed: size of step in frequency units.

		mixer1.gain(0, 1);
		mixer1.gain(2, 1);

		sine_fm1.amplitude(1);
		sine_fm2.amplitude(1);
		sine_fm3.amplitude(1);
		sine_fm4.amplitude(1);

		flange1.begin(l_delayline, FLANGE_DELAY_LENGTH, s_idx, s_depth, s_freq);

		int masterWaveform = WAVEFORM_PULSE;


		waveform1.pulseWidth(0.2);
		waveform1.begin(1, 794, masterWaveform);

		waveform2.pulseWidth(0.2);
		waveform2.begin(1, 647, masterWaveform);

		waveform3.pulseWidth(0.2);
		waveform3.begin(1, 750, masterWaveform);

		waveform4.pulseWidth(0.2);
		waveform4.begin(1, 200, masterWaveform);


		// random walk initial conditions
		for (int i = 0; i < 4; i++) {
			// velocities: initial conditions in -pi : +pi
			theta = M_PI * (random::uniform() * (2.0) - 1.0);
			vx[i] = cos(theta);
			vy[i] = sin(theta);
			// positions: random in [0,L] x [0, L]
			x[i] = random::uniform() * (L);
			y[i] = random::uniform() * (L);
		}
	}

	void process(float k1, float k2) override {


		float knob_1 = k1;
		float knob_2 = k2;
		float dL;

		dL =  L + 100;

		v_var = v_0;

		snfm = knob_1 * 500 + 10;

		mod_freq = knob_2 * 3;


		// loop to "walk" randomly
		for (int i = 0; i < 4; i++) {
			theta = M_PI * (random::uniform() * (2.0) - 1.0);

			posx = cos(theta);
			vx[i] = posx;

			posy = sin(theta);
			vy[i] = posy;

			// Update new position
			xn = x[i] + v_var * vx[i];
			yin = y[i] + v_var * vy[i];

			// periodic boundary conditions
			if (xn < 50)
				//xn += dL;
				xn += 10;
			else if (xn > dL)
				//xn -= dL;
				xn -= 10;

			if (yin < 0.01)
				yin += dL;
			else if (yin > dL)
				yin -= dL;
			x[i] = xn;
			y[i] = yin;
		}
		sine_fm1.frequency(snfm);
		sine_fm2.frequency(snfm + 55);
		sine_fm3.frequency(snfm + 65);
		sine_fm4.frequency(snfm + 75);

		flange1.voices(s_idx, s_depth, mod_freq);


		waveform1.frequency(x[0]);
		waveform2.frequency(x[1]);
		waveform3.frequency(x[2]);
		waveform4.frequency(x[3]);
	}


	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {

		waveform1.update(&waveformBlock[0]);
		waveform2.update(&waveformBlock[1]);
		waveform3.update(&waveformBlock[2]);
		waveform4.update(&waveformBlock[3]);

		sine_fm1.update(&waveformBlock[0], &sineFMBlock[0]);
		sine_fm2.update(&waveformBlock[1], &sineFMBlock[1]);
		sine_fm3.update(&waveformBlock[2], &sineFMBlock[2]);
		sine_fm4.update(&waveformBlock[3], &sineFMBlock[3]);

		mixer1.update(&sineFMBlock[0], &sineFMBlock[1], &sineFMBlock[2], &sineFMBlock[3], &mixerBlock);
		flange1.update(&mixerBlock, &flangeBlock);

		blockBuffer.pushBuffer(flangeBlock.data, AUDIO_BLOCK_SAMPLES);
	}

	AudioStream& getStream() override {
		return flange1;
	}
	unsigned char getPort() override {
		return 0;
	}

private:

	audio_block_t waveformBlock[4], sineFMBlock[4], flangeBlock, mixerBlock;

	AudioSynthWaveform    waveform1;
	AudioSynthWaveform    waveform2;
	AudioSynthWaveform    waveform3;
	AudioSynthWaveform    waveform4;
	AudioSynthWaveformSineModulated sine_fm3;
	AudioSynthWaveformSineModulated sine_fm2;
	AudioSynthWaveformSineModulated sine_fm1;
	AudioSynthWaveformSineModulated sine_fm4;

	AudioEffectFlange        flange1;
	AudioMixer4              mixer1;


	// AudioConnection          patchCord1;
	// AudioConnection          patchCord2;
	// AudioConnection          patchCord3;
	// AudioConnection          patchCord4;
	// AudioConnection          patchCord5;
	// AudioConnection          patchCord6;
	// AudioConnection          patchCord7;
	// AudioConnection          patchCord8;
	// AudioConnection          patchCord9;


	int L; //, i, t;
	float theta, posx, posy, xn, yin;
	float v_0, v_var, snfm;//pw = pulse width
	float x[4], y[4], vx[4], vy[4]; // number depends on waveforms declared

	/*Variables for flange effect*/
	short l_delayline[FLANGE_DELAY_LENGTH]; //left channel
	int s_idx = 2 * FLANGE_DELAY_LENGTH / 4;
	int s_depth = FLANGE_DELAY_LENGTH / 4;
	double s_freq = 3;
	double mod_freq;

};

REGISTER_PLUGIN(Rwalk_SineFMFlange); // this is important, so that we can include the plugin in a bank
