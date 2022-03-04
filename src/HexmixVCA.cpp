#include "plugin.hpp"

using simd::float_4;

static float gainFunction(float x, float shape) {
	float lin = x;
	if (shape > 0.f) {
		float log = 11.f * x / (10.f * x + 1.f);
		return crossfade(lin, log, shape);
	}
	else {
		float exp = std::pow(x, 4);
		return crossfade(lin, exp, -shape);
	}
}

struct HexmixVCA : Module {
	enum ParamIds {
		ENUMS(SHAPE_PARAM, 6),
		ENUMS(VOL_PARAM, 6),
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(IN_INPUT, 6),
		ENUMS(CV_INPUT, 6),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(OUT_OUTPUT, 6),
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	const static int numRows = 6;
	dsp::ClockDivider cvDivider;
	float outputLevels[numRows] = {};
	float shapes[numRows] = {};
	bool finalRowIsMix = true;

	HexmixVCA() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		for (int i = 0; i < numRows; ++i) {
			configParam(SHAPE_PARAM + i, -1.f, 1.f, 0.f, string::f("Channel %d VCA response", i + 1));
			configParam(VOL_PARAM + i, 0.f, 1.f, 1.f, string::f("Channel %d output level", i + 1));

			configInput(IN_INPUT + i, string::f("Channel %d", i + 1));
			configInput(CV_INPUT + i, string::f("Gain %d", i + 1));
			configOutput(OUT_OUTPUT + i, string::f("Channel %d", i + 1));

			getInputInfo(CV_INPUT + i)->description = "Normalled to 10V";

			configBypass(IN_INPUT + i, OUT_OUTPUT + i);
		}
		cvDivider.setDivision(16);

		for (int row = 0; row < numRows; ++row) {
			outputLevels[row] = 1.f;
		}
	}

	void process(const ProcessArgs& args) override {
		float_4 mix[4] = {};
		int maxChannels = 1;

		// only calculate gains/shapes every 16 samples
		if (cvDivider.process()) {
			for (int row = 0; row < numRows; ++row) {
				shapes[row] = params[SHAPE_PARAM + row].getValue();
				outputLevels[row] = params[VOL_PARAM + row].getValue();
			}
		}

		for (int row = 0; row < numRows; ++row) {
			bool finalRow = (row == numRows - 1);
			int channels = 1;
			float_4 in[4] = {};
			bool inputIsConnected = inputs[IN_INPUT + row].isConnected();
			if (inputIsConnected) {
				channels = inputs[row].getChannels();

				// if we're in "mixer" mode, an input only counts towards the main output polyphony count if it's
				// not taken out of the mix (i.e. patched in). the final row should count towards polyphony calc.
				if (finalRowIsMix && (finalRow || !outputs[OUT_OUTPUT + row].isConnected())) {
					maxChannels = std::max(maxChannels, channels);
				}

				float cvGain = clamp(inputs[CV_INPUT + row].getNormalVoltage(10.f) / 10.f, 0.f, 1.f);
				float gain = gainFunction(cvGain, shapes[row]) * outputLevels[row];

				for (int c = 0; c < channels; c += 4) {
					in[c / 4] = inputs[row].getVoltageSimd<float_4>(c) * gain;
				}
			}

			if (!finalRow) {
				if (outputs[OUT_OUTPUT + row].isConnected()) {
					// if output is connected, we don't add to mix
					outputs[OUT_OUTPUT + row].setChannels(channels);
					for (int c = 0; c < channels; c += 4) {
						outputs[OUT_OUTPUT + row].setVoltageSimd(in[c / 4], c);
					}
				}
				else if (finalRowIsMix) {
					// else add to mix (if setting enabled)
					for (int c = 0; c < channels; c += 4) {
						mix[c / 4] += in[c / 4];
					}
				}
			}
			// final row
			else {
				if (outputs[OUT_OUTPUT + row].isConnected()) {
					if (finalRowIsMix) {
						outputs[OUT_OUTPUT + row].setChannels(maxChannels);

						// last channel must always go into mix
						for (int c = 0; c < channels; c += 4) {
							mix[c / 4] += in[c / 4];
						}

						for (int c = 0; c < maxChannels; c += 4) {
							outputs[OUT_OUTPUT + row].setVoltageSimd(mix[c / 4], c);
						}
					}
					else {
						// same as other rows
						outputs[OUT_OUTPUT + row].setChannels(channels);
						for (int c = 0; c < channels; c += 4) {
							outputs[OUT_OUTPUT + row].setVoltageSimd(in[c / 4], c);
						}
					}
				}
			}
		}
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* modeJ = json_object_get(rootJ, "finalRowIsMix");
		if (modeJ) {
			finalRowIsMix = json_boolean_value(modeJ);
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "finalRowIsMix", json_boolean(finalRowIsMix));
		return rootJ;
	}
};


struct HexmixVCAWidget : ModuleWidget {
	HexmixVCAWidget(HexmixVCA* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/HexmixVCA.svg")));

		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<Knurlie>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<BefacoTinyKnobWhite>(mm2px(Vec(20.412, 15.51)), module, HexmixVCA::SHAPE_PARAM + 0));
		addParam(createParamCentered<BefacoTinyKnobWhite>(mm2px(Vec(20.412, 34.115)), module, HexmixVCA::SHAPE_PARAM + 1));
		addParam(createParamCentered<BefacoTinyKnobWhite>(mm2px(Vec(20.412, 52.72)), module, HexmixVCA::SHAPE_PARAM + 2));
		addParam(createParamCentered<BefacoTinyKnobWhite>(mm2px(Vec(20.412, 71.325)), module, HexmixVCA::SHAPE_PARAM + 3));
		addParam(createParamCentered<BefacoTinyKnobWhite>(mm2px(Vec(20.412, 89.93)), module, HexmixVCA::SHAPE_PARAM + 4));
		addParam(createParamCentered<BefacoTinyKnobWhite>(mm2px(Vec(20.412, 108.536)), module, HexmixVCA::SHAPE_PARAM + 5));

		addParam(createParamCentered<BefacoTinyKnobRed>(mm2px(Vec(35.458, 15.51)), module, HexmixVCA::VOL_PARAM + 0));
		addParam(createParamCentered<BefacoTinyKnobRed>(mm2px(Vec(35.458, 34.115)), module, HexmixVCA::VOL_PARAM + 1));
		addParam(createParamCentered<BefacoTinyKnobRed>(mm2px(Vec(35.458, 52.72)), module, HexmixVCA::VOL_PARAM + 2));
		addParam(createParamCentered<BefacoTinyKnobRed>(mm2px(Vec(35.458, 71.325)), module, HexmixVCA::VOL_PARAM + 3));
		addParam(createParamCentered<BefacoTinyKnobRed>(mm2px(Vec(35.458, 89.93)), module, HexmixVCA::VOL_PARAM + 4));
		addParam(createParamCentered<BefacoTinyKnobRed>(mm2px(Vec(35.458, 108.536)), module, HexmixVCA::VOL_PARAM + 5));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.581, 15.51)), module, HexmixVCA::IN_INPUT + 0));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.581, 34.115)), module, HexmixVCA::IN_INPUT + 1));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.581, 52.72)), module, HexmixVCA::IN_INPUT + 2));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.581, 71.325)), module, HexmixVCA::IN_INPUT + 3));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.581, 89.93)), module, HexmixVCA::IN_INPUT + 4));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.581, 108.536)), module, HexmixVCA::IN_INPUT + 5));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(52.083, 15.51)), module, HexmixVCA::CV_INPUT + 0));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(52.083, 34.115)), module, HexmixVCA::CV_INPUT + 1));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(52.083, 52.72)), module, HexmixVCA::CV_INPUT + 2));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(52.083, 71.325)), module, HexmixVCA::CV_INPUT + 3));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(52.083, 89.93)), module, HexmixVCA::CV_INPUT + 4));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(52.083, 108.536)), module, HexmixVCA::CV_INPUT + 5));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(64.222, 15.51)), module, HexmixVCA::OUT_OUTPUT + 0));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(64.222, 34.115)), module, HexmixVCA::OUT_OUTPUT + 1));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(64.222, 52.72)), module, HexmixVCA::OUT_OUTPUT + 2));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(64.222, 71.325)), module, HexmixVCA::OUT_OUTPUT + 3));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(64.222, 89.93)), module, HexmixVCA::OUT_OUTPUT + 4));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(64.222, 108.536)), module, HexmixVCA::OUT_OUTPUT + 5));
	}

	void appendContextMenu(Menu* menu) override {
		HexmixVCA* module = dynamic_cast<HexmixVCA*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator());
		menu->addChild(createBoolPtrMenuItem("Final row is mix", "", &module->finalRowIsMix));
	}
};


Model* modelHexmixVCA = createModel<HexmixVCA, HexmixVCAWidget>("HexmixVCA");