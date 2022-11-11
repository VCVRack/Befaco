#include "plugin.hpp"

using simd::float_4;

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

	float_4 phase[4] = {};
	float_4 tri[4] = {};

	/** The value of the last sync input */
	float sync = 0.0;
	/** The outputs */
	/** Whether we are past the pulse width already */
	bool halfPhase[PORT_MAX_CHANNELS] = {};
	bool removePulseDC = true;

	dsp::MinBlepGenerator<16, 32> triSquareMinBlep[PORT_MAX_CHANNELS];
	dsp::MinBlepGenerator<16, 32> doubleSawMinBlep[PORT_MAX_CHANNELS];
	dsp::MinBlepGenerator<16, 32> sawMinBlep[PORT_MAX_CHANNELS];
	dsp::MinBlepGenerator<16, 32> squareMinBlep[PORT_MAX_CHANNELS];

	EvenVCO() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);
		configParam(OCTAVE_PARAM, -5.0, 4.0, 0.0, "Octave", "'", 0.5);
		getParamQuantity(OCTAVE_PARAM)->snapEnabled = true;
		configParam(TUNE_PARAM, -7.0, 7.0, 0.0, "Tune", " semitones");
		configParam(PWM_PARAM, -1.0, 1.0, 0.0, "Pulse width");

		configInput(PITCH1_INPUT, "Pitch 1");
		configInput(PITCH2_INPUT, "Pitch 2");
		configInput(FM_INPUT, "FM");
		configInput(SYNC_INPUT, "Sync (not implemented)");
		configInput(PWM_INPUT, "Pulse Width Modulation");

		configOutput(TRI_OUTPUT, "Triangle");
		configOutput(SINE_OUTPUT, "Sine");
		configOutput(EVEN_OUTPUT, "Even");
		configOutput(SAW_OUTPUT, "Sawtooth");
		configOutput(SQUARE_OUTPUT, "Square");
	}

	void process(const ProcessArgs& args) override {

		int channels_pitch1 = inputs[PITCH1_INPUT].getChannels();
		int channels_pitch2 = inputs[PITCH2_INPUT].getChannels();

		int channels = 1;
		channels = std::max(channels, channels_pitch1);
		channels = std::max(channels, channels_pitch2);

		float pitch_0 = 1.f + std::round(params[OCTAVE_PARAM].getValue()) + params[TUNE_PARAM].getValue() / 12.f;

		// Compute frequency, pitch is 1V/oct
		float_4 pitch[4] = {};
		for (int c = 0; c < channels; c += 4)
			pitch[c / 4] = pitch_0;

		if (inputs[PITCH1_INPUT].isConnected()) {
			for (int c = 0; c < channels; c += 4)
				pitch[c / 4] += inputs[PITCH1_INPUT].getPolyVoltageSimd<float_4>(c);
		}

		if (inputs[PITCH2_INPUT].isConnected()) {
			for (int c = 0; c < channels; c += 4)
				pitch[c / 4] += inputs[PITCH2_INPUT].getPolyVoltageSimd<float_4>(c);
		}

		if (inputs[FM_INPUT].isConnected()) {
			for (int c = 0; c < channels; c += 4)
				pitch[c / 4] += inputs[FM_INPUT].getPolyVoltageSimd<float_4>(c) / 4.f;
		}

		float_4 freq[4] = {};
		for (int c = 0; c < channels; c += 4) {
			freq[c / 4] = dsp::FREQ_C4 * simd::pow(2.f, pitch[c / 4]);
			freq[c / 4] = clamp(freq[c / 4], 0.f, 20000.f);
		}

		// Pulse width
		float_4 pw[4] = {};
		for (int c = 0; c < channels; c += 4)
			pw[c / 4] = params[PWM_PARAM].getValue();

		if (inputs[PWM_INPUT].isConnected()) {
			for (int c = 0; c < channels; c += 4)
				pw[c / 4] += inputs[PWM_INPUT].getPolyVoltageSimd<float_4>(c) / 5.f;
		}

		float_4 deltaPhase[4] = {};
		float_4 oldPhase[4] = {};
		for (int c = 0; c < channels; c += 4) {
			pw[c / 4] = rescale(clamp(pw[c / 4], -1.0f, 1.0f), -1.0f, 1.0f, 0.05f, 1.0f - 0.05f);

			// Advance phase
			deltaPhase[c / 4] = clamp(freq[c / 4] * args.sampleTime, 1e-6f, 0.5f);
			oldPhase[c / 4] = phase[c / 4];
			phase[c / 4] += deltaPhase[c / 4];
		}

		// the next block can't be done with SIMD instructions, but should at least be completed with
		// blocks of 4 (otherwise popping artfifacts are generated from invalid phase/oldPhase/deltaPhase)
		const int channelsRoundedUpNearestFour = (1 + (channels - 1) / 4) * 4;
		for (int c = 0; c < channelsRoundedUpNearestFour; c++) {

			if (oldPhase[c / 4].s[c % 4] < 0.5 && phase[c / 4].s[c % 4] >= 0.5) {
				float crossing = -(phase[c / 4].s[c % 4] - 0.5) / deltaPhase[c / 4].s[c % 4];
				triSquareMinBlep[c].insertDiscontinuity(crossing, 2.f);
				doubleSawMinBlep[c].insertDiscontinuity(crossing, -2.f);
			}

			if (!halfPhase[c] && phase[c / 4].s[c % 4] >= pw[c / 4].s[c % 4]) {
				float crossing  = -(phase[c / 4].s[c % 4] - pw[c / 4].s[c % 4]) / deltaPhase[c / 4].s[c % 4];
				squareMinBlep[c].insertDiscontinuity(crossing, 2.f);
				halfPhase[c] = true;
			}

			// Reset phase if at end of cycle
			if (phase[c / 4].s[c % 4] >= 1.f) {
				phase[c / 4].s[c % 4] -= 1.f;
				float crossing = -phase[c / 4].s[c % 4] / deltaPhase[c / 4].s[c % 4];
				triSquareMinBlep[c].insertDiscontinuity(crossing, -2.f);
				doubleSawMinBlep[c].insertDiscontinuity(crossing, -2.f);
				squareMinBlep[c].insertDiscontinuity(crossing, -2.f);
				sawMinBlep[c].insertDiscontinuity(crossing, -2.f);
				halfPhase[c] = false;
			}
		}

		float_4 triSquareMinBlepOut[4] = {};
		float_4 doubleSawMinBlepOut[4] = {};
		float_4 sawMinBlepOut[4] = {};
		float_4 squareMinBlepOut[4] = {};

		float_4 triSquare[4] = {};
		float_4 sine[4] = {};
		float_4 doubleSaw[4] = {};

		float_4 even[4] = {};
		float_4 saw[4] = {};
		float_4 square[4] = {};
		float_4 triOut[4] = {};

		for (int c = 0; c < channelsRoundedUpNearestFour; c++) {
			triSquareMinBlepOut[c / 4].s[c % 4] = triSquareMinBlep[c].process();
			doubleSawMinBlepOut[c / 4].s[c % 4] = doubleSawMinBlep[c].process();
			sawMinBlepOut[c / 4].s[c % 4] = sawMinBlep[c].process();
			squareMinBlepOut[c / 4].s[c % 4] = squareMinBlep[c].process();
		}

		for (int c = 0; c < channels; c += 4) {

			triSquare[c / 4] = simd::ifelse((phase[c / 4] < 0.5f), -1.f, +1.f);
			triSquare[c / 4] += triSquareMinBlepOut[c / 4];

			// Integrate square for triangle

			tri[c / 4] += (4.f * triSquare[c / 4]) * (freq[c / 4] * args.sampleTime);
			tri[c / 4] *= (1.f - 40.f * args.sampleTime);
			triOut[c / 4] = 5.f * tri[c / 4];

			sine[c / 4] = 5.f * simd::cos(2 * M_PI * phase[c / 4]);

			// minBlep adds a small amount of DC that becomes significant at higher frequencies,
			// this subtracts DC based on empirical observvations about the scaling relationship
			const float sawCorrect = -5.7;
			const float_4 sawDCComp = deltaPhase[c / 4] * sawCorrect;

			doubleSaw[c / 4] = simd::ifelse((phase[c / 4] < 0.5), (-1.f + 4.f * phase[c / 4]), (-1.f + 4.f * (phase[c / 4] - 0.5f)));
			doubleSaw[c / 4] += doubleSawMinBlepOut[c / 4];
			doubleSaw[c / 4] += 2.f * sawDCComp;
			doubleSaw[c / 4] *= 5.f;

			even[c / 4] = 0.55 * (doubleSaw[c / 4] + 1.27 * sine[c / 4]);
			saw[c / 4] = -1.f + 2.f * phase[c / 4];
			saw[c / 4] += sawMinBlepOut[c / 4];
			saw[c / 4] += sawDCComp;
			saw[c / 4] *= 5.f;

			square[c / 4] = simd::ifelse((phase[c / 4] < pw[c / 4]),  -1.f, +1.f);
			square[c / 4] += squareMinBlepOut[c / 4];
			square[c / 4] += removePulseDC * 2.f * (pw[c / 4] - 0.5f);
			square[c / 4] *= 5.f;

			// Set outputs
			outputs[TRI_OUTPUT].setVoltageSimd(triOut[c / 4], c);
			outputs[SINE_OUTPUT].setVoltageSimd(sine[c / 4], c);
			outputs[EVEN_OUTPUT].setVoltageSimd(even[c / 4], c);
			outputs[SAW_OUTPUT].setVoltageSimd(saw[c / 4], c);
			outputs[SQUARE_OUTPUT].setVoltageSimd(square[c / 4], c);
		}

		// Outputs
		outputs[TRI_OUTPUT].setChannels(channels);
		outputs[SINE_OUTPUT].setChannels(channels);
		outputs[EVEN_OUTPUT].setChannels(channels);
		outputs[SAW_OUTPUT].setChannels(channels);
		outputs[SQUARE_OUTPUT].setChannels(channels);
	}


	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "removePulseDC", json_boolean(removePulseDC));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* pulseDCJ = json_object_get(rootJ, "removePulseDC");
		if (pulseDCJ) {
			removePulseDC = json_boolean_value(pulseDCJ);
		}
	}
};


struct EvenVCOWidget : ModuleWidget {
	EvenVCOWidget(EvenVCO* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/EvenVCO.svg")));

		addChild(createWidget<Knurlie>(Vec(15, 0)));
		addChild(createWidget<Knurlie>(Vec(15, 365)));
		addChild(createWidget<Knurlie>(Vec(15 * 6, 0)));
		addChild(createWidget<Knurlie>(Vec(15 * 6, 365)));

		addParam(createParam<BefacoBigKnob>(Vec(22, 32), module, EvenVCO::OCTAVE_PARAM));
		addParam(createParam<BefacoTinyKnob>(Vec(73, 131), module, EvenVCO::TUNE_PARAM));
		addParam(createParam<Davies1900hRedKnob>(Vec(16, 230), module, EvenVCO::PWM_PARAM));

		addInput(createInput<BefacoInputPort>(Vec(8, 120), module, EvenVCO::PITCH1_INPUT));
		addInput(createInput<BefacoInputPort>(Vec(19, 157), module, EvenVCO::PITCH2_INPUT));
		addInput(createInput<BefacoInputPort>(Vec(48, 183), module, EvenVCO::FM_INPUT));
		addInput(createInput<BefacoInputPort>(Vec(86, 189), module, EvenVCO::SYNC_INPUT));

		addInput(createInput<BefacoInputPort>(Vec(72, 236), module, EvenVCO::PWM_INPUT));

		addOutput(createOutput<BefacoOutputPort>(Vec(10, 283), module, EvenVCO::TRI_OUTPUT));
		addOutput(createOutput<BefacoOutputPort>(Vec(87, 283), module, EvenVCO::SINE_OUTPUT));
		addOutput(createOutput<BefacoOutputPort>(Vec(48, 306), module, EvenVCO::EVEN_OUTPUT));
		addOutput(createOutput<BefacoOutputPort>(Vec(10, 327), module, EvenVCO::SAW_OUTPUT));
		addOutput(createOutput<BefacoOutputPort>(Vec(87, 327), module, EvenVCO::SQUARE_OUTPUT));
	}

	void appendContextMenu(Menu* menu) override {
		EvenVCO* module = dynamic_cast<EvenVCO*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator());
		menu->addChild(createSubmenuItem("Hardware compatibility", "",
			[ = ](Menu * menu) {
			menu->addChild(createBoolPtrMenuItem("Remove DC from pulse", "", &module->removePulseDC));
			}
		));
	}
};


Model* modelEvenVCO = createModel<EvenVCO, EvenVCOWidget>("EvenVCO");
