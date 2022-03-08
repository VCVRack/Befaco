#pragma once

#include "NoisePlethoraPlugin.hpp"

class Rwalk_BitCrushPW : public NoisePlethoraPlugin {

public:

	Rwalk_BitCrushPW()
	//: patchCord4(waveform8, 0, mixer2, 3)
	//, patchCord6(waveform9, 0, mixer3, 0)
	//, patchCord9(waveform7, 0, mixer2, 2)
	//, patchCord11(waveform4, 0, mixer1, 3)
	//, patchCord12(waveform5, 0, mixer2, 0)
	//, patchCord13(waveform6, 0, mixer2, 1)
	//, patchCord14(waveform3, 0, mixer1, 2)
	//, patchCord15(waveform1, 0, mixer1, 0)
	//, patchCord16(waveform2, 0, mixer1, 1)
	//, patchCord17(mixer2, 0, mixer5, 2)
	//, patchCord18(mixer1, 0, mixer5, 1)
	//, patchCord19(mixer3, 0, mixer6, 1)
	//, patchCord21(mixer6, freeverb1)
	//, patchCord22(mixer5, bitcrusher1)
	//, patchCord23(bitcrusher1, 0, mixer7, 1)
	//, patchCord24(freeverb1, 0, mixer7, 2)
	{ }
	~Rwalk_BitCrushPW() override {}

	// delete copy constructors
	Rwalk_BitCrushPW(const Rwalk_BitCrushPW&) = delete;
	Rwalk_BitCrushPW& operator=(const Rwalk_BitCrushPW&) = delete;

	void init() override {

		L = 600; // Size of box: maximum frequency
		v_0 = 30; // speed: size of step in frequency units.

		mixer1.gain(0, 1);
		mixer1.gain(1, 1);
		mixer1.gain(2, 1);
		mixer1.gain(3, 1);
		mixer2.gain(0, 1);
		mixer2.gain(1, 1);
		mixer2.gain(2, 1);
		mixer2.gain(3, 1);
		// mixer3.gain(0, 1);
		// mixer3.gain(1, 1);
		// mixer3.gain(2, 1);
		// mixer3.gain(3, 1);

		//   SINE
		int masterWaveform = WaveformType::WAVEFORM_PULSE;

		waveform1.pulseWidth(0.2);
		waveform1.begin(1, 794, masterWaveform);

		waveform2.pulseWidth(0.2);
		waveform2.begin(1, 647, masterWaveform);

		waveform3.pulseWidth(0.2);
		waveform3.begin(1, 524, masterWaveform);

		waveform4.pulseWidth(0.2);
		waveform4.begin(1, 444, masterWaveform);

		waveform5.pulseWidth(0.2);
		waveform5.begin(1, 368, masterWaveform);

		waveform6.pulseWidth(0.2);
		waveform6.begin(1, 283, masterWaveform);

		waveform7.pulseWidth(0.2);
		waveform7.begin(1, 283, masterWaveform);

		waveform8.pulseWidth(0.2);
		waveform8.begin(1, 283, masterWaveform);

		waveform9.pulseWidth(0.2);
		waveform9.begin(1, 283, masterWaveform);


		// random walk initial conditions
		for (int i = 0; i < 9; i++) {
			// velocities: initial conditions in -pi : +pi
			theta = M_PI * (random::uniform() * 2.0 - 1.0);
			vx[i] = std::cos(theta);
			vy[i] = std::sin(theta);
			// positions: random in [0,L] x [0, L]
			x[i] = random::uniform() * L;
			y[i] = random::uniform() * L;

		}
	}

	void process(float k1, float k2) override {

		float knob_1 = k1;
		float knob_2 = k2;

		float dL;

		dL =  L + 100;

		v_var = v_0;

		bc_01 = (knob_1 - 0) * (0.75 - 0.2) / (1 - 0) + 0.2; // función para recortar intervalo de 0.1 a 0.9


		bitcrusher1.bits(1);

		fv = knob_2;
		freeverb1.roomsize(fv);

		// loop to "walk" randomly
		for (int i = 0; i < 9; i++) {
			theta = M_PI * (random::uniform() * 2.0 - 1.0);

			posx = std::cos(theta);
			vx[i] = posx;

			posy = std::sin(theta);
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



		waveform1.frequency(x[0]);
		waveform2.frequency(x[1]);
		waveform3.frequency(x[2]);
		waveform4.frequency(x[3]);
		waveform5.frequency(x[4]);
		waveform6.frequency(x[5]);
		waveform7.frequency(x[6]);
		waveform8.frequency(x[7]);
		waveform9.frequency(x[8]);

		waveform1.pulseWidth(bc_01);
		waveform2.pulseWidth(bc_01);
		waveform3.pulseWidth(bc_01);
		waveform4.pulseWidth(bc_01);
		waveform5.pulseWidth(bc_01);
		waveform6.pulseWidth(bc_01);
		waveform7.pulseWidth(bc_01);
		waveform8.pulseWidth(bc_01);
		waveform9.pulseWidth(bc_01);

	}


	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {
		// sum first block of oscillators
		waveform1.update(&waveformBlock[0]);
		waveform2.update(&waveformBlock[1]);
		waveform3.update(&waveformBlock[2]);
		waveform4.update(&waveformBlock[3]);
		mixer1.update(&waveformBlock[0], &waveformBlock[1], &waveformBlock[2], &waveformBlock[3], &mixerBlock[0]);

		// sum second block of oscillators
		waveform5.update(&waveformBlock[4]);
		waveform6.update(&waveformBlock[5]);
		waveform7.update(&waveformBlock[6]);
		waveform8.update(&waveformBlock[7]);
		mixer2.update(&waveformBlock[4], &waveformBlock[5], &waveformBlock[6], &waveformBlock[7], &mixerBlock[1]);

		// sum two blocks and feed into bitcrusher
		mixer5.update(&mixerBlock[0], &mixerBlock[1], nullptr, nullptr, &mixerBlock[2]);
		bitcrusher1.update(&mixerBlock[2], &bitcrushBlock);

		waveform9.update(&waveformBlock[8]);
		// mixer3/6 are just one-input + unit gain so just skip to freeverb
		freeverb1.update(&waveformBlock[8], &freeverbBlock);

		// finally sum bitcrush and freeverb
		mixer7.update(&bitcrushBlock, &freeverbBlock, nullptr, nullptr, &mixerBlock[3]);

		blockBuffer.pushBuffer(mixerBlock[3].data, AUDIO_BLOCK_SAMPLES);
	}

	AudioStream& getStream() override {
		return mixer7;
	}
	unsigned char getPort() override {
		return 0;
	}

private:

	audio_block_t waveformBlock[9], mixerBlock[4], bitcrushBlock, freeverbBlock;

	/* will be filled in */
	// GUItool: begin automatically generated code
	AudioSynthWaveform       waveform8; //xy=519.75,577.75
	AudioSynthWaveform       waveform9; //xy=519.75,614.75
	AudioSynthWaveform       waveform7; //xy=520.75,541.75
	AudioSynthWaveform       waveform4;      //xy=521.75,427.75
	AudioSynthWaveform       waveform5; //xy=521.75,466.75
	AudioSynthWaveform       waveform6; //xy=521.75,504.75
	AudioSynthWaveform       waveform3;      //xy=522.75,391.75
	AudioSynthWaveform       waveform1;      //xy=523.75,316.75
	AudioSynthWaveform       waveform2;      //xy=523.75,354.75
	AudioMixer4              mixer2; //xy=713.75,528.75
	AudioMixer4              mixer1;         //xy=714.75,377.75
	//AudioMixer4              mixer3; //xy=716.75,664.75
	//AudioMixer4              mixer6;         //xy=1248,561

	AudioMixer4              mixer5; //xy=961.75,589.75

	AudioEffectBitcrusher    bitcrusher1;    //xy=1439,393
	AudioEffectFreeverb      freeverb1;      //xy=923.5,281.5
	AudioMixer4              mixer7;         //xy=1650,448

	//AudioOutputI2S           i2s1;           //xy=1227.75,604.75
	// AudioConnection          patchCord4(waveform8, 0, mixer2, 3);
	// AudioConnection          patchCord6(waveform9, 0, mixer3, 0);
	// AudioConnection          patchCord9(waveform7, 0, mixer2, 2);
	// AudioConnection          patchCord11(waveform4, 0, mixer1, 3);
	// AudioConnection          patchCord12(waveform5, 0, mixer2, 0);
	// AudioConnection          patchCord13(waveform6, 0, mixer2, 1);
	// AudioConnection          patchCord14(waveform3, 0, mixer1, 2);
	// AudioConnection          patchCord15(waveform1, 0, mixer1, 0);
	// AudioConnection          patchCord16(waveform2, 0, mixer1, 1);
	// AudioConnection          patchCord17(mixer2, 0, mixer5, 2);
	// AudioConnection          patchCord18(mixer1, 0, mixer5, 1);
	// AudioConnection          patchCord19(mixer3, 0, mixer6, 1);
	// AudioConnection          patchCord21(mixer6, freeverb1);
	// AudioConnection          patchCord22(mixer5, bitcrusher1);
	// AudioConnection          patchCord23(bitcrusher1, 0, mixer7, 1);
	// AudioConnection          patchCord24(freeverb1, 0, mixer7, 2);


	int L; //, i, t;
	float theta, posx, posy, xn, yin;
	float v_0, v_var, bc_01, fv;//pw = pulse width
	float x[9], y[9], vx[9], vy[9]; // number depends on waveforms declared

};

REGISTER_PLUGIN(Rwalk_BitCrushPW); // this is important, so that we can include the plugin in a bank
