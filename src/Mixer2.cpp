#include "plugin.hpp"

using simd::float_4;

struct Mixer2 : Module {
	enum ParamId {
		GAIN1_PARAM,
		GAIN2_PARAM,
		GAIN3_PARAM,
		GAIN4_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		CH1_INPUT,
		CH2_INPUT,
		CH3_INPUT,
		CH4_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		MIX_12_OUPUT,
		MIX_34_OUPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		ENUMS(MIX12_LIGHT, 3),
		ENUMS(MIX34_LIGHT, 3),
		LIGHTS_LEN
	};

	dsp::ClockDivider lightDivider;
	bool applyClipping = false;

	Mixer2() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(GAIN1_PARAM, 0.f, 1.f, 1.f, "Gain 1");
		configParam(GAIN2_PARAM, 0.f, 1.f, 1.f, "Gain 2");
		configParam(GAIN3_PARAM, 0.f, 1.f, 1.f, "Gain 3");
		configParam(GAIN4_PARAM, 0.f, 1.f, 1.f, "Gain 4");

		configInput(CH1_INPUT, "Channel 1");
		configInput(CH2_INPUT, "Channel 2");
		configInput(CH3_INPUT, "Channel 3");
		configInput(CH4_INPUT, "Channel 4");

		configOutput(MIX_12_OUPUT, "Mix 1+2");
		configOutput(MIX_34_OUPUT, "Mix 3+4 (Master)");

		lightDivider.setDivision(32);
	}

	void process(const ProcessArgs& args) override {
		const int numPolyphonyEngines = std::max({1, inputs[CH1_INPUT].getChannels(), inputs[CH2_INPUT].getChannels(), inputs[CH3_INPUT].getChannels(), inputs[CH4_INPUT].getChannels()});
		const bool useMasterMix = !outputs[MIX_12_OUPUT].isConnected();

		// used for LEDs
		float_4 sum12 = 0.f, sum34 = 0.f;
		for (int c = 0; c < numPolyphonyEngines; c += 4) {
			float_4 out12 = 0.f;
			float_4 out34 = 0.f;

			if (inputs[CH1_INPUT].isConnected()) {
				out12 += inputs[CH1_INPUT].getVoltageSimd<float_4>(c) * params[GAIN1_PARAM].getValue();
			}

			if (inputs[CH2_INPUT].isConnected()) {
				out12 += inputs[CH2_INPUT].getVoltageSimd<float_4>(c) * params[GAIN2_PARAM].getValue();
			}

			if (inputs[CH3_INPUT].isConnected()) {
				out34 += inputs[CH3_INPUT].getVoltageSimd<float_4>(c) * params[GAIN3_PARAM].getValue();
			}

			if (inputs[CH4_INPUT].isConnected()) {
				out34 += inputs[CH4_INPUT].getVoltageSimd<float_4>(c) * params[GAIN4_PARAM].getValue();
			}

			const float_4 mix12 = useMasterMix ? float_4::zero() : out12;
			const float_4 mix34 = useMasterMix ? out12 + out34 : out34;

			if (applyClipping) {
				outputs[MIX_12_OUPUT].setVoltageSimd(clip(mix12), c);
				outputs[MIX_34_OUPUT].setVoltageSimd(clip(mix34), c);
			}
			else {
				outputs[MIX_12_OUPUT].setVoltageSimd(mix12, c);
				outputs[MIX_34_OUPUT].setVoltageSimd(mix34, c);
			}

			sum12 += simd::pow(out12, 2);
			sum34 += simd::pow(out34, 2);
		}

		outputs[MIX_12_OUPUT].setChannels(numPolyphonyEngines);
		outputs[MIX_34_OUPUT].setChannels(numPolyphonyEngines);

		if (lightDivider.process()) {
			const float deltaTime = args.sampleTime * lightDivider.getDivision();
			if (numPolyphonyEngines == 1) {
				lights[MIX12_LIGHT + 0].setBrightnessSmooth(std::abs(sum12[0]) / 5.f, deltaTime);
				lights[MIX12_LIGHT + 1].setBrightness(0.f);
				lights[MIX12_LIGHT + 2].setBrightness(0.f);
				lights[MIX34_LIGHT + 0].setBrightnessSmooth(std::abs(sum34[0]) / 5.f, deltaTime);
				lights[MIX34_LIGHT + 1].setBrightness(0.f);
				lights[MIX34_LIGHT + 2].setBrightness(0.f);
			}
			else {
				// TODO: better polyphonic lights?
				lights[MIX12_LIGHT + 0].setBrightness(0.f);
				lights[MIX12_LIGHT + 1].setBrightness(0.f);
				float light12 = std::sqrt((sum12[0] + sum12[1] + sum12[2] + sum12[3]) / numPolyphonyEngines) / 5.f;
				lights[MIX12_LIGHT + 2].setBrightnessSmooth(light12, deltaTime);

				lights[MIX34_LIGHT + 0].setBrightness(0.f);
				lights[MIX34_LIGHT + 1].setBrightness(0.f);
				float light34 = std::sqrt((sum34[0] + sum34[1] + sum34[2] + sum34[3]) / numPolyphonyEngines) / 5.f;
				lights[MIX34_LIGHT + 2].setBrightnessSmooth(light34, deltaTime);
			}
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "applyClipping", json_boolean(applyClipping));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* applyClippingJ = json_object_get(rootJ, "applyClipping");
		if (applyClippingJ) {
			applyClipping = json_boolean_value(applyClippingJ);
		}
	}
};


struct Mixer2Widget : ModuleWidget {
	Mixer2Widget(Mixer2* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Mixer2.svg")));

		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(10.0, 13.49)), module, Mixer2::GAIN1_PARAM));
		addParam(createParamCentered<Davies1900hLightGreyKnob>(mm2px(Vec(10.0, 33.6)), module, Mixer2::GAIN2_PARAM));
		addParam(createParamCentered<Davies1900hDarkGreyKnob>(mm2px(Vec(10.0, 53.5)), module, Mixer2::GAIN3_PARAM));
		addParam(createParamCentered<Davies1900hBlackKnob>(mm2px(Vec(10.0, 73.3)), module, Mixer2::GAIN4_PARAM));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(5.065, 88.898)), module, Mixer2::CH1_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(15.0, 88.9)), module, Mixer2::CH2_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(5.0, 101.2)), module, Mixer2::CH3_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(15.065, 101.198)), module, Mixer2::CH4_INPUT));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(5.0, 113.5)), module, Mixer2::MIX_12_OUPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(15.0, 113.5)), module, Mixer2::MIX_34_OUPUT));

		addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(mm2px(Vec(2.5, 23.621)), module, Mixer2::MIX12_LIGHT));
		addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(mm2px(Vec(2.5, 63.4)), module, Mixer2::MIX34_LIGHT));
	}

	void appendContextMenu(Menu* menu) override {
		Mixer2* module = dynamic_cast<Mixer2*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator());
		menu->addChild(createSubmenuItem("Hardware compatibility", "",
		[ = ](Menu * menu) {
			menu->addChild(createBoolPtrMenuItem("Clip outputs at ±10V", "", &module->applyClipping));
		}));
	}
};


Model* modelMixer2 = createModel<Mixer2, Mixer2Widget>("Mixer2");