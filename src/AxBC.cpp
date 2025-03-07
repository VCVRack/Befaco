#include "plugin.hpp"

using simd::float_4;

struct AxBC : Module {
	enum ParamId {
		GAIN_B1_PARAM,
		B1_PARAM,
		GAIN_C1_PARAM,
		C1_PARAM,
		GAIN_B2_PARAM,
		B2_PARAM,
		GAIN_C2_PARAM,
		C2_PARAM,
		MODE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		A1_INPUT,
		B1_INPUT,
		C1_INPUT,
		A2_INPUT,
		B2_INPUT,
		C2_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUT_1_OUTPUT,
		OUT_2_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		ENUMS(OUT_1_MINUS_LIGHT, 3),
		ENUMS(OUT_1_PLUS_LIGHT, 3),
		ENUMS(OUT_2_MINUS_LIGHT, 3),
		ENUMS(OUT_2_PLUS_LIGHT, 3),
		LIGHTS_LEN
	};
	const float gains[3] = {-1.f, +1.f, +2.f};
	bool applyClipping = true;
	dsp::ClockDivider lightDivider;

	AxBC() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(B1_PARAM, 0.f, 1.f, 1.f, "B1");
		configParam(C1_PARAM, 0.f, 1.f, 0.f, "C1");
		configParam(B2_PARAM, 0.f, 1.f, 1.f, "B2");
		configParam(C2_PARAM, 0.f, 1.f, 0.f, "C2");
		configSwitch(GAIN_B1_PARAM, 0.f, 2.f, 1.f, "Gain Mode", {"x -1", "x 1", "x 2"});
		configSwitch(GAIN_C1_PARAM, 0.f, 2.f, 1.f, "Gain Mode", {"x -1", "x 1", "x 2"});
		configSwitch(GAIN_B2_PARAM, 0.f, 2.f, 1.f, "Gain Mode", {"x -1", "x 1", "x 2"});
		configSwitch(GAIN_C2_PARAM, 0.f, 2.f, 1.f, "Gain Mode", {"x -1", "x 1", "x 2"});
		auto mode = configSwitch(MODE_PARAM, 0.f, 1.f, 0.f, "Mix mode", {"Mix", "Mult"});
		mode->description = "Mix: channel 1 is mixed into channel 2, if channel 1 output is unpatched.\n"
		                    "Mult: a copy of A1 is normalled to A2 input, if A2 is unpatched.";

		configInput(A1_INPUT, "A1");
		configInput(B1_INPUT, "B1");
		configInput(C1_INPUT, "C1");
		configInput(A2_INPUT, "A2");
		configInput(B2_INPUT, "B2");
		configInput(C2_INPUT, "C2");

		configOutput(OUT_1_OUTPUT, "Out 1");
		configOutput(OUT_2_OUTPUT, "Out 2");

		lightDivider.setDivision(64);
	}

	void process(const ProcessArgs& args) override {
		const int numPolyphonyEngines = std::max({1,
		                                inputs[A1_INPUT].getChannels(), inputs[B1_INPUT].getChannels(), inputs[C1_INPUT].getChannels(),
		                                inputs[A2_INPUT].getChannels(), inputs[B2_INPUT].getChannels(), inputs[C2_INPUT].getChannels()});

		for (int c = 0; c < numPolyphonyEngines; c += 4) {
			const float_4 inA1 = inputs[A1_INPUT].getPolyVoltageSimd<float_4>(c);
			const float_4 inB1 = inputs[B1_INPUT].getNormalPolyVoltageSimd<float_4>(5.f, c) / 5.f;
			const float_4 inC1 = inputs[C1_INPUT].getNormalPolyVoltageSimd<float_4>(5.f, c);

			const float gainB1 = params[B1_PARAM].getValue() * gains[(int) params[GAIN_B1_PARAM].getValue()];
			const float gainC1 = params[C1_PARAM].getValue() * gains[(int) params[GAIN_C1_PARAM].getValue()];

			// ch1: a * b + c
			const float_4 out1 = inA1 * gainB1 * inB1 + gainC1 * inC1;

			const float_4 inA2 = inputs[A2_INPUT].getNormalPolyVoltageSimd<float_4>(inA1 * params[MODE_PARAM].getValue(), c);
			const float_4 inB2 = inputs[B2_INPUT].getNormalPolyVoltageSimd<float_4>(5.f, c) / 5.f;
			const float_4 inC2 = inputs[C2_INPUT].getNormalPolyVoltageSimd<float_4>(5.f, c);

			const float gainB2 = params[B2_PARAM].getValue() * gains[(int) params[GAIN_B2_PARAM].getValue()];
			const float gainC2 = params[C2_PARAM].getValue() * gains[(int) params[GAIN_C2_PARAM].getValue()];

			// ch2: a * b + c
			const float_4 out2 = inA2 * gainB2 * inB2 + gainC2 * inC2;
			// if we're in mix mode and out1 is not connected, mix ch1 into ch2
			const bool isCh1MixedIntoCh2 = (params[MODE_PARAM].getValue() == 0.f) && !outputs[OUT_1_OUTPUT].isConnected();

			if (applyClipping) {
				outputs[OUT_1_OUTPUT].setVoltageSimd(clip(out1), c);
				outputs[OUT_2_OUTPUT].setVoltageSimd(clip(out1 * isCh1MixedIntoCh2 + out2), c);
			}
			else {
				outputs[OUT_1_OUTPUT].setVoltageSimd(out1, c);
				outputs[OUT_2_OUTPUT].setVoltageSimd(out1 * isCh1MixedIntoCh2 + out2, c);
			}
		}

		outputs[OUT_1_OUTPUT].setChannels(numPolyphonyEngines);
		outputs[OUT_2_OUTPUT].setChannels(numPolyphonyEngines);

		if (lightDivider.process()) {
			const float lightTime = args.sampleTime * lightDivider.getDivision();
			processLEDs(lightTime, numPolyphonyEngines);
		}
	}

	void processLEDs(const float lightTime, const int channels) {
		
		// monophonic uses red and green LEDs
		if (channels == 1) {

			const float redValue1 = -std::min(0.f, outputs[OUT_1_OUTPUT].getVoltage() / 5.f);
			const float greenValue1 = +std::max(0.f, outputs[OUT_1_OUTPUT].getVoltage() / 5.f);
			lights[OUT_1_MINUS_LIGHT + 0].setSmoothBrightness(redValue1, lightTime);
			lights[OUT_1_MINUS_LIGHT + 1].setBrightness(0.f);
			lights[OUT_1_MINUS_LIGHT + 2].setBrightness(0.f);

			lights[OUT_1_PLUS_LIGHT + 0].setBrightness(0.f);
			lights[OUT_1_PLUS_LIGHT + 1].setSmoothBrightness(greenValue1, lightTime);
			lights[OUT_1_PLUS_LIGHT + 2].setBrightness(0.f);

			const float redValue2 = -std::min(0.f, outputs[OUT_2_OUTPUT].getVoltage() / 5.f);
			const float greenValue2 = +std::max(0.f, outputs[OUT_2_OUTPUT].getVoltage() / 5.f);
			lights[OUT_2_MINUS_LIGHT + 0].setSmoothBrightness(redValue2, lightTime);
			lights[OUT_2_MINUS_LIGHT + 1].setBrightness(0.f);
			lights[OUT_2_MINUS_LIGHT + 2].setBrightness(0.f);

			lights[OUT_2_PLUS_LIGHT + 0].setBrightness(0.f);
			lights[OUT_2_PLUS_LIGHT + 1].setSmoothBrightness(greenValue2, lightTime);
			lights[OUT_2_PLUS_LIGHT + 2].setBrightness(0.f);
		}
		// polyphonic uses blue LEDs, but seperated by signal polarity
		else {
			float sumNeg1 = 0.f, sumPos1 = 0.f;
			float sumNeg2 = 0.f, sumPos2 = 0.f;
			for (int c = 0; c < channels; c++) {
				sumNeg1 += -std::min(outputs[OUT_1_OUTPUT].getVoltage(c), 0.f);
				sumPos1 += +std::max(outputs[OUT_1_OUTPUT].getVoltage(c), 0.f);

				sumNeg2 += -std::min(outputs[OUT_2_OUTPUT].getVoltage(c), 0.f);
				sumPos2 += +std::max(outputs[OUT_2_OUTPUT].getVoltage(c), 0.f);
			}
			lights[OUT_1_MINUS_LIGHT + 0].setBrightness(0.f);
			lights[OUT_1_MINUS_LIGHT + 1].setBrightness(0.f);
			lights[OUT_1_MINUS_LIGHT + 2].setBrightness(sumNeg1 / channels / 5.f);

			lights[OUT_1_PLUS_LIGHT + 0].setBrightness(0.f);
			lights[OUT_1_PLUS_LIGHT + 1].setBrightness(0.f);
			lights[OUT_1_PLUS_LIGHT + 2].setBrightness(sumPos1 / channels / 5.f);

			lights[OUT_2_MINUS_LIGHT + 0].setBrightness(0.f);
			lights[OUT_2_MINUS_LIGHT + 1].setBrightness(0.f);
			lights[OUT_2_MINUS_LIGHT + 2].setBrightness(sumNeg2 / channels / 5.f);

			lights[OUT_2_PLUS_LIGHT + 0].setBrightness(0.f);
			lights[OUT_2_PLUS_LIGHT + 1].setBrightness(0.f);
			lights[OUT_2_PLUS_LIGHT + 2].setBrightness(sumPos2 / channels / 5.f);
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "applyClipping", json_boolean(applyClipping));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* clipJ = json_object_get(rootJ, "applyClipping");
		if (clipJ) {
			applyClipping = json_boolean_value(clipJ);
		}
	}
};


struct AxBCWidget : ModuleWidget {
	AxBCWidget(AxBC* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/AxBC.svg")));

		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParam<CKSSNarrow3>(mm2px(Vec(5.327, 12.726)), module, AxBC::GAIN_B1_PARAM));
		addParam(createParamCentered<Davies1900hDarkGreyKnob>(mm2px(Vec(19.875, 16.316)), module, AxBC::B1_PARAM));
		addParam(createParam<CKSSNarrow3>(mm2px(Vec(20.93, 29.723)), module, AxBC::GAIN_C1_PARAM));
		addParam(createParamCentered<BefacoTinyKnobLightGrey>(mm2px(Vec(9.898, 33.333)), module, AxBC::C1_PARAM));
		addParam(createParam<CKSSNarrow3>(mm2px(Vec(5.327, 46.724)), module, AxBC::GAIN_B2_PARAM));
		addParam(createParamCentered<Davies1900hDarkGreyKnob>(mm2px(Vec(19.875, 50.315)), module, AxBC::B2_PARAM));
		addParam(createParam<CKSSNarrow3>(mm2px(Vec(20.93, 63.73)), module, AxBC::GAIN_C2_PARAM));
		addParam(createParamCentered<BefacoTinyKnobLightGrey>(mm2px(Vec(9.898, 67.318)), module, AxBC::C2_PARAM));
		addParam(createParam<CKSSNarrow>(mm2px(Vec(3.471, 111.231)), module, AxBC::MODE_PARAM));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(4.885, 84.785)), module, AxBC::A1_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(14.885, 84.785)), module, AxBC::B1_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(24.885, 84.785)), module, AxBC::C1_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(4.885, 98.175)), module, AxBC::A2_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(14.885, 98.175)), module, AxBC::B2_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(24.862, 98.175)), module, AxBC::C2_INPUT));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(14.907, 114.02)), module, AxBC::OUT_1_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(24.862, 114.02)), module, AxBC::OUT_2_OUTPUT));

		addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(mm2px(Vec(12.04, 107.465)), module, AxBC::OUT_1_MINUS_LIGHT));
		addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(mm2px(Vec(17.758, 107.465)), module, AxBC::OUT_1_PLUS_LIGHT));
		addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(mm2px(Vec(21.996, 107.465)), module, AxBC::OUT_2_MINUS_LIGHT));
		addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(mm2px(Vec(27.681, 107.465)), module, AxBC::OUT_2_PLUS_LIGHT));
	}

	void appendContextMenu(Menu* menu) override {
		AxBC* module = dynamic_cast<AxBC*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator());
		menu->addChild(createSubmenuItem("Hardware compatibility", "",
		[ = ](Menu * menu) {
			menu->addChild(createBoolPtrMenuItem("Clip outputs at ±10V", "", &module->applyClipping));
		}));
	}
};


Model* modelAxBC = createModel<AxBC, AxBCWidget>("AxBC");