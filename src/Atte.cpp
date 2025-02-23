#include "plugin.hpp"

using simd::float_4;

struct Atte : Module {
	enum ParamId {
		GAIN_A_PARAM,
		GAIN_B_PARAM,
		GAIN_C_PARAM,
		GAIN_D_PARAM,
		MODE_A_PARAM,
		MODE_B_PARAM,
		MODE_C_PARAM,
		MODE_D_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		A_INPUT,
		B_INPUT,
		C_INPUT,
		D_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		A_OUTPUT,
		B_OUTPUT,
		C_OUTPUT,
		D_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		ENUMS(A_LIGHT, 3),
		ENUMS(B_LIGHT, 3),
		ENUMS(C_LIGHT, 3),
		ENUMS(D_LIGHT, 3),
		LIGHTS_LEN
	};
	const int NUM_CHANNELS = 4;

	dsp::ClockDivider lightDivider;
	int normalledVoltageIdx = 2; 	// 0 - +1V, 1 - +5V, 2 - +10V
	const float normalledVoltages[3] = {1.f, 5.f, 10.f};

	Atte() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(GAIN_A_PARAM, 0.f, 1.f, 1.f, "Gain A");
		configParam(GAIN_B_PARAM, 0.f, 1.f, 1.f, "Gain B");
		configParam(GAIN_C_PARAM, 0.f, 1.f, 1.f, "Gain C");
		configParam(GAIN_D_PARAM, 0.f, 1.f, 1.f, "Gain D");
		configSwitch(MODE_A_PARAM, 0.f, 1.f, 1.f, "Mode A", {"Inverse Attenutation", "Attenuation"});
		configSwitch(MODE_B_PARAM, 0.f, 1.f, 1.f, "Mode B", {"Inverse Attenutation", "Attenuation"});
		configSwitch(MODE_C_PARAM, 0.f, 1.f, 1.f, "Mode C", {"Inverse Attenutation", "Attenuation"});
		configSwitch(MODE_D_PARAM, 0.f, 1.f, 1.f, "Mode D", {"Inverse Attenutation", "Attenuation"});

		auto inputA = configInput(A_INPUT, "A");
		inputA->description = "Normalled to +10V";
		auto inputB = configInput(B_INPUT, "B");
		inputB->description = "Normalled to input A";
		auto inputC = configInput(C_INPUT, "C");
		inputC->description = "Normalled to input B";
		auto inputD = configInput(D_INPUT, "D");
		inputD->description = "Normalled to input C";

		configOutput(A_OUTPUT, "A");
		configOutput(B_OUTPUT, "B");
		configOutput(C_OUTPUT, "C");
		configOutput(D_OUTPUT, "D");

		lightDivider.setDivision(32);
	}

	void process(const ProcessArgs& args) override {

		const bool updateLights = lightDivider.process();
		const float deltaTime = args.sampleTime * lightDivider.getDivision();

		const float normalledVoltage = normalledVoltages[normalledVoltageIdx];
		float_4 previousChannelNormalledVoltage[4] = {normalledVoltage, normalledVoltage, normalledVoltage, normalledVoltage};
		int previousChannelPolyphony = 1;

		// loop over the 4 channels
		for (int channel = 0; channel < NUM_CHANNELS; channel += 1) {
			// polyphony setting is normalled from the previous channel
			const int numPolyphonyEngines = std::max(1, inputs[A_INPUT + channel].isConnected() ? inputs[A_INPUT + channel].getChannels() : previousChannelPolyphony);
			previousChannelPolyphony = numPolyphonyEngines;

			// loop over the polyphony engines
			for (int c = 0; c < numPolyphonyEngines; c += 4) {

				float_4 inA = inputs[A_INPUT + channel].getNormalPolyVoltageSimd<float_4>(previousChannelNormalledVoltage[c / 4], c);
				const float gainMode = (params[MODE_A_PARAM + channel].getValue() ? 1.f : -1.f);
				outputs[A_OUTPUT + channel].setVoltageSimd(inA * gainMode * params[GAIN_A_PARAM + channel].getValue(), c);

				previousChannelNormalledVoltage[c / 4] = inA;
			}

			outputs[A_OUTPUT + channel].setChannels(numPolyphonyEngines);

			if (updateLights) {
				if (numPolyphonyEngines > 1) {
					lights[A_LIGHT + 0 + channel * 3].setBrightness(0.f);
					lights[A_LIGHT + 1 + channel * 3].setBrightness(0.f);
					float sum = 0.f;
					for (int c = 0; c < numPolyphonyEngines; c += 4) {
						sum += std::pow(outputs[A_OUTPUT + channel].getVoltage(c), 2);
					}
					lights[A_LIGHT + 2 + channel * 3].setBrightness(std::sqrt(sum / numPolyphonyEngines) / 10.f);
				}
				else {
					// green for positive voltage, red for negative voltage
					lights[A_LIGHT + 0 + channel * 3].setSmoothBrightness(outputs[A_OUTPUT + channel].getVoltage() < 0.f ? -outputs[A_OUTPUT + channel].getVoltage() / 10.f : 0.f, deltaTime);
					lights[A_LIGHT + 1 + channel * 3].setSmoothBrightness(outputs[A_OUTPUT + channel].getVoltage() > 0.f ? +outputs[A_OUTPUT + channel].getVoltage() / 10.f : 0.f, deltaTime);
					lights[A_LIGHT + 2 + channel * 3].setBrightness(0.f);
				}
			}
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "normalledVoltageIdx", json_integer(normalledVoltageIdx));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* normalledVoltageIdxJ = json_object_get(rootJ, "normalledVoltageIdx");
		if (normalledVoltageIdxJ) {
			normalledVoltageIdx = json_integer_value(normalledVoltageIdxJ);
		}
	}
};


struct AtteWidget : ModuleWidget {
	AtteWidget(Atte* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Atte.svg")));

		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParam<CKSSNarrow>(mm2px(Vec(1.168, 10.207)), module, Atte::MODE_A_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(12.2, 13.8)), module, Atte::GAIN_A_PARAM));
		addParam(createParam<CKSSNarrow>(mm2px(Vec(1.168, 26.174)), module, Atte::MODE_B_PARAM));
		addParam(createParamCentered<BefacoTinyKnobLightGrey>(mm2px(Vec(12.2, 29.767)), module, Atte::GAIN_B_PARAM));
		addParam(createParam<CKSSNarrow>(mm2px(Vec(1.168, 42.14)), module, Atte::MODE_C_PARAM));
		addParam(createParamCentered<BefacoTinyKnobDarkGrey>(mm2px(Vec(12.2, 45.733)), module, Atte::GAIN_C_PARAM));
		addParam(createParam<CKSSNarrow>(mm2px(Vec(1.168, 58.107)), module, Atte::MODE_D_PARAM));
		addParam(createParamCentered<BefacoTinyKnobBlack>(mm2px(Vec(12.2, 61.7)), module, Atte::GAIN_D_PARAM));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(5.0, 76.6)), module, Atte::A_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(5.0, 88.9)), module, Atte::B_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(5.0, 101.2)), module, Atte::C_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(5.0, 113.5)), module, Atte::D_INPUT));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(14.978, 76.6)), module, Atte::A_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(14.978, 88.9)), module, Atte::B_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(14.978, 101.2)), module, Atte::C_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(14.978, 113.5)), module, Atte::D_OUTPUT));

		addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(mm2px(Vec(2.9, 20.85)), module, Atte::A_LIGHT));
		addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(mm2px(Vec(2.9, 36.817)), module, Atte::B_LIGHT));
		addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(mm2px(Vec(2.9, 52.783)), module, Atte::C_LIGHT));
		addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(mm2px(Vec(2.9, 68.75)), module, Atte::D_LIGHT));
	}

	void appendContextMenu(Menu* menu) override {

		Atte* module = dynamic_cast<Atte*>(this->module);
		assert(module);

		// user can pick +1V, +5V or +10V for the normalled voltage
		menu->addChild(createIndexPtrSubmenuItem("Normalled voltage", {"+1V", "+5V", "+10V"}, &module->normalledVoltageIdx));
	}

	void step() override {
		Atte* module = dynamic_cast<Atte*>(this->module);

		if (module) {
			module->getInputInfo(Atte::A_INPUT)->description = "Normalled to +" + string::f("%.0gV", module->normalledVoltages[module->normalledVoltageIdx]);
		}

		ModuleWidget::step();
	}
};


Model* modelAtte = createModel<Atte, AtteWidget>("Atte");