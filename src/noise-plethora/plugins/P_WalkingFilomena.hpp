#pragma once
#include <cmath>

#include "NoisePlethoraPlugin.hpp"

class WalkingFilomena : public NoisePlethoraPlugin {

public:

	WalkingFilomena()
	// : patchCord1(waveform12, 0, mixer3, 3)
	// , patchCord2(waveform16, 0, mixer4, 3)
	// , patchCord3(waveform11, 0, mixer3, 2)
	// , patchCord4(waveform8, 0, mixer2, 3)
	// , patchCord5(waveform15, 0, mixer4, 2)
	// , patchCord6(waveform9, 0, mixer3, 0)
	// , patchCord7(waveform10, 0, mixer3, 1)
	// , patchCord8(waveform13, 0, mixer4, 0)
	// , patchCord9(waveform7, 0, mixer2, 2)
	// , patchCord10(waveform14, 0, mixer4, 1)
	// , patchCord11(waveform4, 0, mixer1, 3)
	// , patchCord12(waveform5, 0, mixer2, 0)
	// , patchCord13(waveform6, 0, mixer2, 1)
	// , patchCord14(waveform3, 0, mixer1, 2)
	// , patchCord15(waveform1, 0, mixer1, 0)
	// , patchCord16(waveform2, 0, mixer1, 1)
	// , patchCord17(mixer2, 0, mixer5, 1)
	// , patchCord18(mixer1, 0, mixer5, 0)
	// , patchCord19(mixer3, 0, mixer5, 2)
	// , patchCord20(mixer4, 0, mixer5, 3)
	//   //patchCord21(mixer5, 0, i2s1, 0);
	{ }
	~WalkingFilomena() override {}

	// delete copy constructors
	WalkingFilomena(const WalkingFilomena&) = delete;
	WalkingFilomena& operator=(const WalkingFilomena&) = delete;

	void init() override {

		L = 1800; // Size of box: maximum frequency
		v_0 = 10; // speed: size of step in frequency units.

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

		//   SINE
		WaveformType masterWaveform = WAVEFORM_PULSE;

		waveform1.pulseWidth(0.5);
		waveform1.begin(1, 794, masterWaveform);

		waveform2.pulseWidth(0.5);
		waveform2.begin(1, 647, masterWaveform);

		waveform3.pulseWidth(0.5);
		waveform3.begin(1, 524, masterWaveform);

		waveform4.pulseWidth(0.5);
		waveform4.begin(1, 444, masterWaveform);

		waveform5.pulseWidth(0.5);
		waveform5.begin(1, 368, masterWaveform);

		waveform6.pulseWidth(0.5);
		waveform6.begin(1, 283, masterWaveform);

		waveform7.pulseWidth(0.5);
		waveform7.begin(1, 283, masterWaveform);

		waveform8.pulseWidth(0.5);
		waveform8.begin(1, 283, masterWaveform);

		waveform9.pulseWidth(0.5);
		waveform9.begin(1, 283, masterWaveform);

		waveform10.pulseWidth(0.5);
		waveform10.begin(1, 283, masterWaveform);

		waveform11.pulseWidth(0.5);
		waveform11.begin(1, 283, masterWaveform);

		waveform12.pulseWidth(0.5);
		waveform12.begin(1, 283, masterWaveform);

		waveform13.pulseWidth(0.5);
		waveform13.begin(1, 283, masterWaveform);

		waveform14.pulseWidth(0.5);
		waveform14.begin(1, 283, masterWaveform);

		waveform15.pulseWidth(0.5);
		waveform15.begin(1, 283, masterWaveform);

		waveform16.pulseWidth(0.5);
		waveform16.begin(1, 283, masterWaveform);

		// random walk initial conditions
		for (int i = 0; i < 16; i++) {
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
		float pw;
		float dL;

		dL =  knob_1 * L + 200;

		v_var = v_0;

		pw = (knob_2 - 0) * (0.9 - 0.1) / (1 - 0) + 0.1; // función para recortar intervalo de 0.1 a 0.9

		// loop to "walk" randomly
		for (int i = 0; i < 16; i++) {
			theta = M_PI * (random::uniform() * 2.0 - 1.0);

			posx = std::cos(theta);
			vx[i] = posx;

			posy = std::sin(theta);
			vy[i] = posy;

			// Update new position
			xn = x[i] + v_var * vx[i];
			yin = y[i] + v_var * vy[i];

			// periodic boundary conditions
			if (xn < 100)
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

		waveform1.pulseWidth(pw);
		waveform2.pulseWidth(pw);
		waveform3.pulseWidth(pw);
		waveform4.pulseWidth(pw);
		waveform5.pulseWidth(pw);
		waveform6.pulseWidth(pw);
		waveform7.pulseWidth(pw);
		waveform8.pulseWidth(pw);

		waveform9.pulseWidth(pw);
		waveform10.pulseWidth(pw);
		waveform11.pulseWidth(pw);
		waveform12.pulseWidth(pw);
		waveform13.pulseWidth(pw);
		waveform14.pulseWidth(pw);
		waveform15.pulseWidth(pw);
		waveform16.pulseWidth(pw);

		waveform1.frequency(x[0]);
		waveform2.frequency(x[1]);
		waveform3.frequency(x[2]);
		waveform4.frequency(x[3]);
		waveform5.frequency(x[4]);
		waveform6.frequency(x[5]);
		waveform7.frequency(x[6]);
		waveform8.frequency(x[7]);
		waveform9.frequency(x[8]);
		waveform10.frequency(x[9]);
		waveform11.frequency(x[10]);
		waveform12.frequency(x[11]);
		waveform13.frequency(x[12]);
		waveform14.frequency(x[13]);
		waveform15.frequency(x[14]);
		waveform16.frequency(x[15]);


	}

	void processGraphAsBlock(TeensyBuffer& blockBuffer) override {

		waveform1.update(&waveformBlock[0]);
		waveform2.update(&waveformBlock[1]);
		waveform3.update(&waveformBlock[2]);
		waveform4.update(&waveformBlock[3]);
		waveform5.update(&waveformBlock[4]);
		waveform6.update(&waveformBlock[5]);
		waveform7.update(&waveformBlock[6]);
		waveform8.update(&waveformBlock[7]);
		waveform9.update(&waveformBlock[8]);
		waveform10.update(&waveformBlock[9]);
		waveform11.update(&waveformBlock[10]);
		waveform12.update(&waveformBlock[11]);
		waveform13.update(&waveformBlock[12]);
		waveform14.update(&waveformBlock[13]);
		waveform15.update(&waveformBlock[14]);
		waveform16.update(&waveformBlock[15]);

		mixer1.update(&waveformBlock[0], &waveformBlock[1], &waveformBlock[2], &waveformBlock[3], &mixBlock[0]);
		mixer2.update(&waveformBlock[4], &waveformBlock[5], &waveformBlock[6], &waveformBlock[7], &mixBlock[1]);
		mixer3.update(&waveformBlock[8], &waveformBlock[9], &waveformBlock[10], &waveformBlock[11], &mixBlock[2]);
		mixer4.update(&waveformBlock[12], &waveformBlock[13], &waveformBlock[14], &waveformBlock[15], &mixBlock[3]);

		mixer5.update(&mixBlock[0], &mixBlock[1], &mixBlock[2], &mixBlock[3], &mixBlock[4]);
		blockBuffer.pushBuffer(mixBlock[4].data, AUDIO_BLOCK_SAMPLES);
	}

	AudioStream& getStream() override {
		return mixer5;
	}
	unsigned char getPort() override {
		return 0;
	}

private:

	audio_block_t waveformBlock[16], mixBlock[5];

	/* will be filled in */
	// GUItool: begin automatically generated code
	AudioSynthWaveform       waveform12; //xy=517.75,725.75
	AudioSynthWaveform       waveform16; //xy=517.75,874.75
	AudioSynthWaveform       waveform11; //xy=518.75,689.75
	AudioSynthWaveform       waveform8; //xy=519.75,577.75
	AudioSynthWaveform       waveform15; //xy=518.75,838.75
	AudioSynthWaveform       waveform9; //xy=519.75,614.75
	AudioSynthWaveform       waveform10; //xy=519.75,652.75
	AudioSynthWaveform       waveform13; //xy=519.75,763.75
	AudioSynthWaveform       waveform7; //xy=520.75,541.75
	AudioSynthWaveform       waveform14; //xy=519.75,801.75
	AudioSynthWaveform       waveform4;      //xy=521.75,427.75
	AudioSynthWaveform       waveform5; //xy=521.75,466.75
	AudioSynthWaveform       waveform6; //xy=521.75,504.75
	AudioSynthWaveform       waveform3;      //xy=522.75,391.75
	AudioSynthWaveform       waveform1;      //xy=523.75,316.75
	AudioSynthWaveform       waveform2;      //xy=523.75,354.75
	AudioMixer4              mixer2; //xy=713.75,528.75
	AudioMixer4              mixer1;         //xy=714.75,377.75
	AudioMixer4              mixer3; //xy=716.75,664.75
	AudioMixer4              mixer4; //xy=717.75,792.75
	AudioMixer4              mixer5; //xy=961.75,589.75
	//AudioOutputI2S           i2s1;           //xy=1227.75,604.75
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
	//AudioConnection          patchCord21(mixer5, 0, i2s1, 0);
	//AudioControlSGTL5000     audioOut;     //xy=1016.75,846.75
	int L; //, i, t;
	float theta, posx, posy, xn, yin;
	float v_0, v_var;//pw = pulse width
	float x[16], y[16], vx[16], vy[16]; // number depends on waveforms declared

};

REGISTER_PLUGIN(WalkingFilomena); // this is important, so that we can include the plugin in a bank
