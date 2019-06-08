#include "plugin.hpp"

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

	float phase = 0.0;
	/** The value of the last sync input */
	float sync = 0.0;
	/** The outputs */
	float tri = 0.0;
	/** Whether we are past the pulse width already */
	bool halfPhase = false;

	dsp::MinBlepGenerator<16, 32> triSquareMinBlep;
	dsp::MinBlepGenerator<16, 32> triMinBlep;
	dsp::MinBlepGenerator<16, 32> sineMinBlep;
	dsp::MinBlepGenerator<16, 32> doubleSawMinBlep;
	dsp::MinBlepGenerator<16, 32> sawMinBlep;
	dsp::MinBlepGenerator<16, 32> squareMinBlep;

	dsp::RCFilter triFilter;

	EvenVCO() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);
		configParam(OCTAVE_PARAM, -5.0, 4.0, 0.0, "Octave", "'", 0.5);
		configParam(TUNE_PARAM, -7.0, 7.0, 0.0, "Tune", " semitones");
		configParam(PWM_PARAM, -1.0, 1.0, 0.0, "Pulse width");


	}

	void process(const ProcessArgs &args) override {
		// Compute frequency, pitch is 1V/oct

		int channels_pitch1 = inputs[PITCH1_INPUT].getChannels();
		int channels_pitch2 = inputs[PITCH2_INPUT].getChannels();
		int channels_fm     = inputs[FM_INPUT].getChannels();

		int channels = 1;
		channels = MAX(channels, channels_pitch1);
		channels = MAX(channels, channels_pitch2);
		// channels = MAX(channels, channels_fm);

		float pitch_0 = 1.f + std::round(params[OCTAVE_PARAM].getValue()) + params[TUNE_PARAM].getValue() / 12.f;


		pitch += inputs[PITCH1_INPUT].getVoltage() + inputs[PITCH2_INPUT].getVoltage();
		pitch += inputs[FM_INPUT].getVoltage() / 4.f;
		float freq = dsp::FREQ_C4 * std::pow(2.f, pitch);
		freq = clamp(freq, 0.f, 20000.f);

		// Pulse width
		float pw = params[PWM_PARAM].getValue() + inputs[PWM_INPUT].getVoltage() / 5.f;
		const float minPw = 0.05;
		pw = rescale(clamp(pw, -1.f, 1.f), -1.f, 1.f, minPw, 1.f - minPw);

		// Advance phase
		float deltaPhase = clamp(freq * args.sampleTime, 1e-6f, 0.5f);
		float oldPhase = phase;
		phase += deltaPhase;

		if (oldPhase < 0.5 && phase >= 0.5) {
			float crossing = -(phase - 0.5) / deltaPhase;
			triSquareMinBlep.insertDiscontinuity(crossing, 2.f);
			doubleSawMinBlep.insertDiscontinuity(crossing, -2.f);
		}

		if (!halfPhase && phase >= pw) {
			float crossing  = -(phase - pw) / deltaPhase;
			squareMinBlep.insertDiscontinuity(crossing, 2.f);
			halfPhase = true;
		}

		// Reset phase if at end of cycle
		if (phase >= 1.f) {
			phase -= 1.f;
			float crossing = -phase / deltaPhase;
			triSquareMinBlep.insertDiscontinuity(crossing, -2.f);
			doubleSawMinBlep.insertDiscontinuity(crossing, -2.f);
			squareMinBlep.insertDiscontinuity(crossing, -2.f);
			sawMinBlep.insertDiscontinuity(crossing, -2.f);
			halfPhase = false;
		}

		// Outputs
		float triSquare = (phase < 0.5) ? -1.f : 1.f;
		triSquare += triSquareMinBlep.process();

		// Integrate square for triangle
		tri += 4.f * triSquare * freq * args.sampleTime;
		tri *= (1.f - 40.f * args.sampleTime);

		float sine = -std::cos(2*M_PI * phase);
		float doubleSaw = (phase < 0.5) ? (-1.f + 4.f*phase) : (-1.f + 4.f*(phase - 0.5));
		doubleSaw += doubleSawMinBlep.process();
		float even = 0.55 * (doubleSaw + 1.27 * sine);
		float saw = -1.f + 2.f*phase;
		saw += sawMinBlep.process();
		float square = (phase < pw) ? -1.f : 1.f;
		square += squareMinBlep.process();

		// Set outputs
		outputs[TRI_OUTPUT].setVoltage(5.f*tri);
		outputs[SINE_OUTPUT].setVoltage(5.f*sine);
		outputs[EVEN_OUTPUT].setVoltage(5.f*even);
		outputs[SAW_OUTPUT].setVoltage(5.f*saw);
		outputs[SQUARE_OUTPUT].setVoltage(5.f*square);
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
