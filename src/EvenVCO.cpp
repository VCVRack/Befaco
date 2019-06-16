#include "plugin.hpp"
#include "simd_mask.hpp"

#define MAX(a,b) (a>b)?a:b

struct EvenVCO : Module {
	enum ParamIds {
		OCTAVE_PARAM,
		TUNE_PARAM,
		PWM_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		PITCH1_INPUT,
		PITCH2_INPUT,
		FM_INPUT,
		SYNC_INPUT,
		PWM_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		TRI_OUTPUT,
		SINE_OUTPUT,
		EVEN_OUTPUT,
		SAW_OUTPUT,
		SQUARE_OUTPUT,
		NUM_OUTPUTS
	};

	simd::float_4 phase[4];
	simd::float_4 tri[4];

	/** The value of the last sync input */
	float sync = 0.0;
	/** The outputs */
	/** Whether we are past the pulse width already */
	bool halfPhase[PORT_MAX_CHANNELS];

	dsp::MinBlepGenerator<16, 32> triSquareMinBlep[PORT_MAX_CHANNELS];
	dsp::MinBlepGenerator<16, 32> triMinBlep[PORT_MAX_CHANNELS];
	dsp::MinBlepGenerator<16, 32> sineMinBlep[PORT_MAX_CHANNELS];
	dsp::MinBlepGenerator<16, 32> doubleSawMinBlep[PORT_MAX_CHANNELS];
	dsp::MinBlepGenerator<16, 32> sawMinBlep[PORT_MAX_CHANNELS];
	dsp::MinBlepGenerator<16, 32> squareMinBlep[PORT_MAX_CHANNELS];

	dsp::RCFilter triFilter;

	ChannelMask channelMask;

	EvenVCO() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);
		configParam(OCTAVE_PARAM, -5.0, 4.0, 0.0, "Octave", "'", 0.5);
		configParam(TUNE_PARAM, -7.0, 7.0, 0.0, "Tune", " semitones");
		configParam(PWM_PARAM, -1.0, 1.0, 0.0, "Pulse width");
		
		for(int i=0; i<4; i++) {
			phase[i] = simd::float_4(0.0f);
			tri[i] = simd::float_4(0.0f);
		}
		for(int c=0; c<PORT_MAX_CHANNELS; c++) halfPhase[c] = false;
	}

	void process(const ProcessArgs &args) override {

        simd::float_4 pitch[4];
		simd::float_4 pitch_1[4];
		simd::float_4 pitch_2[4];
		simd::float_4 pitch_fm[4];
		simd::float_4 freq[4]; 
		simd::float_4 pw[4];
		simd::float_4 pwm[4];
		simd::float_4 deltaPhase[4];
		simd::float_4 oldPhase[4];

		int channels_pitch1 = inputs[PITCH1_INPUT].getChannels();
		int channels_pitch2 = inputs[PITCH2_INPUT].getChannels();
		int channels_fm     = inputs[FM_INPUT].getChannels();
		int channels_pwm	= inputs[PWM_INPUT].getChannels();

		int channels = 1;
		channels = MAX(channels, channels_pitch1);
		channels = MAX(channels, channels_pitch2);
		// channels = MAX(channels, channels_fm);

		float pitch_0 = 1.f + std::round(params[OCTAVE_PARAM].getValue()) + params[TUNE_PARAM].getValue() / 12.f;

		// Compute frequency, pitch is 1V/oct

		for(int c=0; c<channels; c+=4) pitch[c/4] = simd::float_4(pitch_0);

		if(inputs[PITCH1_INPUT].isConnected()) {
			load_input(inputs[PITCH1_INPUT], pitch_1, channels_pitch1);
			channelMask.apply(pitch_1, channels_pitch1);
			for(int c=0; c<channels_pitch1; c+=4) pitch[c/4] += pitch_1[c/4];
		}

		if(inputs[PITCH2_INPUT].isConnected()) {
			load_input(inputs[PITCH2_INPUT], pitch_2, channels_pitch2);
			channelMask.apply(pitch_2, channels_pitch2);
			for(int c=0; c<channels_pitch2; c+=4) pitch[c/4] += pitch_2[c/4];
		}

		if(inputs[FM_INPUT].isConnected()) {
			load_input(inputs[FM_INPUT], pitch_fm, channels_fm);
			channelMask.apply(pitch_fm, channels_fm);
			for(int c=0; c<channels_fm; c+=4) pitch[c/4] += pitch_fm[c/4] / 4.f;
		}

		for(int c=0; c<channels; c+=4) {
			freq[c/4] = dsp::FREQ_C4 * simd::pow(2.f, pitch[c/4]);
			freq[c/4] = clamp(freq[c/4], 0.f, 20000.f);
		}


		// Pulse width

		float pw_0 = params[PWM_PARAM].getValue();

		for(int c=0; c<channels; c+=4) pw[c/4] = simd::float_4(pw_0);
		
		if(inputs[PWM_INPUT].isConnected()) {
			load_input(inputs[PWM_INPUT], pwm, channels_pwm);
			channelMask.apply(pwm, channels_pwm);
			for(int c=0; c<channels_pwm; c+=4) pw[c/4] += pwm[c/4] / 5.f;
		}

		const simd::float_4 minPw_4 = simd::float_4(0.05f);
		const simd::float_4 m_one_4 = simd::float_4(-1.0f);
		const simd::float_4 one_4 = simd::float_4(1.0f);

		for(int c=0; c<channels; c+=4) {
			pw[c/4] = rescale(clamp(pw[c/4], m_one_4, one_4), m_one_4, one_4, minPw_4, one_4 - minPw_4);

			// Advance phase
			deltaPhase[c/4] = clamp(freq[c/4] * args.sampleTime, simd::float_4(1e-6f), simd::float_4(0.5f));
			oldPhase[c/4] = phase[c/4];
			phase[c/4] += deltaPhase[c/4];
		}

		// the next block can't be done with SIMD instructions:

		for(int c=0; c<channels; c++) {

			if (oldPhase[c/4].s[c%4] < 0.5 && phase[c/4].s[c%4] >= 0.5) {
				float crossing = -(phase[c/4].s[c%4] - 0.5) / deltaPhase[c/4].s[c%4];
				triSquareMinBlep[c].insertDiscontinuity(crossing, 2.f);
				doubleSawMinBlep[c].insertDiscontinuity(crossing, -2.f);
			}

			if (!halfPhase[c] && phase[c/4].s[c%4] >= pw[c/4].s[c%4]) {
				float crossing  = -(phase[c/4].s[c%4] - pw[c/4].s[c%4]) / deltaPhase[c/4].s[c%4];
				squareMinBlep[c].insertDiscontinuity(crossing, 2.f);
				halfPhase[c] = true;
			}	

			// Reset phase if at end of cycle
			if (phase[c/4].s[c%4] >= 1.f) {
				phase[c/4].s[c%4] -= 1.f;
				float crossing = -phase[c/4].s[c%4] / deltaPhase[c/4].s[c%4];
				triSquareMinBlep[c].insertDiscontinuity(crossing, -2.f);
				doubleSawMinBlep[c].insertDiscontinuity(crossing, -2.f);
				squareMinBlep[c].insertDiscontinuity(crossing, -2.f);
				sawMinBlep[c].insertDiscontinuity(crossing, -2.f);
				halfPhase[c] = false;
			}
		}

		simd::float_4 triSquareMinBlepOut[4];
		simd::float_4 doubleSawMinBlepOut[4];
		simd::float_4 sawMinBlepOut[4];
		simd::float_4 squareMinBlepOut[4];

		simd::float_4 triSquare[4];
		simd::float_4 sine[4];
		simd::float_4 doubleSaw[4];

		simd::float_4 even[4];
		simd::float_4 saw[4];
		simd::float_4 square[4];
		simd::float_4 triOut[4];

		for(int c=0; c<channels; c++) {
			triSquareMinBlepOut[c/4].s[c%4] = triSquareMinBlep[c].process();
			doubleSawMinBlepOut[c/4].s[c%4] = doubleSawMinBlep[c].process();
			sawMinBlepOut[c/4].s[c%4] = sawMinBlep[c].process();
			squareMinBlepOut[c/4].s[c%4] = squareMinBlep[c].process();
		}

		// Outputs

		outputs[TRI_OUTPUT].setChannels(channels);
		outputs[SINE_OUTPUT].setChannels(channels);
		outputs[EVEN_OUTPUT].setChannels(channels);
		outputs[SAW_OUTPUT].setChannels(channels);
		outputs[SQUARE_OUTPUT].setChannels(channels);

		for(int c=0; c<channels; c+=4) {
			
			triSquare[c/4] = simd::ifelse( (phase[c/4] < 0.5f*one_4), m_one_4, one_4);	
			triSquare[c/4] += triSquareMinBlepOut[c/4];

			// Integrate square for triangle
	
			tri[c/4] += (4.f * triSquare[c/4]) * (freq[c/4] * args.sampleTime);
			tri[c/4] *= (1.f - 40.f * args.sampleTime);
			triOut[c/4] = 5.f * tri[c/4];


			sine[c/4] = 5.f * simd::cos(2*M_PI * phase[c/4]);
		
			doubleSaw[c/4] = simd::ifelse( (phase[c/4] < 0.5),  (-1.f + 4.f*phase[c/4]),  (-1.f + 4.f*(phase[c/4] - 0.5f)));
			doubleSaw[c/4] += doubleSawMinBlepOut[c/4];
			doubleSaw[c/4] *= 5.f;

			even[c/4] = 0.55 * (doubleSaw[c/4] + 1.27 * sine[c/4]);
			saw[c/4] = -1.f + 2.f*phase[c/4];
			saw[c/4] += sawMinBlepOut[c/4];
			saw[c/4] *= 5.f;

			square[c/4] = simd::ifelse( (phase[c/4] < pw[c/4]),  m_one_4, one_4) ;
			square[c/4] += squareMinBlepOut[c/4];
			square[c/4] *= 5.f;

			// Set outputs

			triOut[c/4].store(outputs[TRI_OUTPUT].getVoltages(c));
			sine[c/4].store(outputs[SINE_OUTPUT].getVoltages(c));
			even[c/4].store(outputs[EVEN_OUTPUT].getVoltages(c));
			saw[c/4].store(outputs[SAW_OUTPUT].getVoltages(c));
			square[c/4].store(outputs[SQUARE_OUTPUT].getVoltages(c));
		}
	}
};


struct EvenVCOWidget : ModuleWidget {
	EvenVCOWidget(EvenVCO *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/EvenVCO.svg")));

		addChild(createWidget<Knurlie>(Vec(15, 0)));
		addChild(createWidget<Knurlie>(Vec(15, 365)));
		addChild(createWidget<Knurlie>(Vec(15*6, 0)));
		addChild(createWidget<Knurlie>(Vec(15*6, 365)));

		addParam(createParam<BefacoBigSnapKnob>(Vec(22, 32), module, EvenVCO::OCTAVE_PARAM));
		addParam(createParam<BefacoTinyKnob>(Vec(73, 131), module, EvenVCO::TUNE_PARAM));
		addParam(createParam<Davies1900hRedKnob>(Vec(16, 230), module, EvenVCO::PWM_PARAM));

		addInput(createInput<PJ301MPort>(Vec(8, 120), module, EvenVCO::PITCH1_INPUT));
		addInput(createInput<PJ301MPort>(Vec(19, 157), module, EvenVCO::PITCH2_INPUT));
		addInput(createInput<PJ301MPort>(Vec(48, 183), module, EvenVCO::FM_INPUT));
		addInput(createInput<PJ301MPort>(Vec(86, 189), module, EvenVCO::SYNC_INPUT));

		addInput(createInput<PJ301MPort>(Vec(72, 236), module, EvenVCO::PWM_INPUT));

		addOutput(createOutput<PJ301MPort>(Vec(10, 283), module, EvenVCO::TRI_OUTPUT));
		addOutput(createOutput<PJ301MPort>(Vec(87, 283), module, EvenVCO::SINE_OUTPUT));
		addOutput(createOutput<PJ301MPort>(Vec(48, 306), module, EvenVCO::EVEN_OUTPUT));
		addOutput(createOutput<PJ301MPort>(Vec(10, 327), module, EvenVCO::SAW_OUTPUT));
		addOutput(createOutput<PJ301MPort>(Vec(87, 327), module, EvenVCO::SQUARE_OUTPUT));
	}
};


Model *modelEvenVCO = createModel<EvenVCO, EvenVCOWidget>("EvenVCO");
