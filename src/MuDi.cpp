#include "plugin.hpp"

using namespace simd;

struct MuDi : Module {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		CLOCK_INPUT,
		RESET_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		F_1_OUTPUT,
		F_2_OUTPUT,
		F_4_OUTPUT,
		F_8_OUTPUT,
		F_16_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		ENUMS(F_1_LIGHT, 3),
		ENUMS(F_2_LIGHT, 3),
		ENUMS(F_4_LIGHT, 3),
		ENUMS(F_8_LIGHT, 3),
		ENUMS(F_16_LIGHT, 3),
		LIGHTS_LEN
	};

	dsp::TSchmittTrigger<float_4> clockTrigger_1[4];
	dsp::TSchmittTrigger<float_4> clockTrigger_2[4];
	dsp::TSchmittTrigger<float_4> clockTrigger_4[4];
	dsp::TSchmittTrigger<float_4> clockTrigger_8[4];
	float_4 clockState_1[4] = {};
	float_4 clockState_2[4] = {};
	float_4 clockState_4[4] = {};
	float_4 clockState_8[4] = {};
	float_4 clockState_16[4] = {};

	dsp::TSchmittTrigger<float_4> resetTrigger[4];
	dsp::ClockDivider lightDivider;
	bool removeClockDC = false;

	MuDi() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(CLOCK_INPUT, "Clock");
		configInput(RESET_INPUT, "Reset");

		configOutput(F_1_OUTPUT, "F");
		configOutput(F_2_OUTPUT, "1/2 F");
		configOutput(F_4_OUTPUT, "1/4 F");
		configOutput(F_8_OUTPUT, "1/8 F");
		configOutput(F_16_OUTPUT, "1/16 F");

		lightDivider.setDivision(32);
	}

	void process(const ProcessArgs& args) override {

		const int numPolyphonyEngines = inputs[CLOCK_INPUT].getChannels();

		for (int c = 0; c < numPolyphonyEngines; c += 4) {
			// reset
			float_4 reset = resetTrigger[c / 4].process(inputs[RESET_INPUT].getPolyVoltageSimd<float_4>(c));
			clockState_2[c / 4] = ifelse(reset, 0.f, clockState_2[c / 4]);
			clockState_4[c / 4] = ifelse(reset, 0.f, clockState_4[c / 4]);
			clockState_8[c / 4] = ifelse(reset, 0.f, clockState_8[c / 4]);
			clockState_16[c / 4] = ifelse(reset, 0.f, clockState_16[c / 4]);

			// base derived clock
			float_4 triggered = clockTrigger_1[c / 4].process(inputs[CLOCK_INPUT].getVoltageSimd<float_4>(c));
			clockState_1[c / 4] = clockTrigger_1[c / 4].isHigh();

			// 1/2 derived clock changes state on every rising edge of the base clock
			clockState_2[c / 4] = ifelse(triggered, ~clockState_2[c / 4], clockState_2[c / 4]);
			float_4 clockTriggered_2 = clockTrigger_2[c / 4].process(ifelse(clockState_2[c / 4], 10.f, 0.f));

			// 1/4 derived clock changes state on every rising edge of the 1/2 derived clock
			clockState_4[c / 4] = ifelse(clockTriggered_2, ~clockState_4[c / 4], clockState_4[c / 4]);
			float_4 clockTriggered_4 = clockTrigger_4[c / 4].process(ifelse(clockState_4[c / 4], 10.f, 0.f));

			// 1/8 derived clock changes state on every rising edge of the 1/4 derived clock
			clockState_8[c / 4] = ifelse(clockTriggered_4, ~clockState_8[c / 4], clockState_8[c / 4]);
			float_4 clockTriggered_8 = clockTrigger_8[c / 4].process(ifelse(clockState_8[c / 4], 10.f, 0.f));

			// 1/16 derived clock changes state on every rising edge of the 1/8 derived clock
			clockState_16[c / 4] = ifelse(clockTriggered_8, ~clockState_16[c / 4], clockState_16[c / 4]);

			// Set outputs
			outputs[F_1_OUTPUT].setVoltageSimd(ifelse(clockState_1[c / 4], 10.f, 0.f) - 5.f * removeClockDC, c);
			outputs[F_2_OUTPUT].setVoltageSimd(ifelse(clockState_2[c / 4], 10.f, 0.f) - 5.f * removeClockDC, c);
			outputs[F_4_OUTPUT].setVoltageSimd(ifelse(clockState_4[c / 4], 10.f, 0.f) - 5.f * removeClockDC, c);
			outputs[F_8_OUTPUT].setVoltageSimd(ifelse(clockState_8[c / 4], 10.f, 0.f) - 5.f * removeClockDC, c);
			outputs[F_16_OUTPUT].setVoltageSimd(ifelse(clockState_16[c / 4], 10.f, 0.f) - 5.f * removeClockDC, c);
		}

		outputs[F_1_OUTPUT].setChannels(numPolyphonyEngines);
		outputs[F_2_OUTPUT].setChannels(numPolyphonyEngines);
		outputs[F_4_OUTPUT].setChannels(numPolyphonyEngines);
		outputs[F_8_OUTPUT].setChannels(numPolyphonyEngines);
		outputs[F_16_OUTPUT].setChannels(numPolyphonyEngines);

		bool anyState[5] = {};
		for (int c = 0; c < numPolyphonyEngines; c++) {
			anyState[0] |= ifelse(clockState_1[c / 4], 1.f, 0.f)[c % 4] > 0.f;
			anyState[1] |= ifelse(clockState_2[c / 4], 1.f, 0.f)[c % 4] > 0.f;
			anyState[2] |= ifelse(clockState_4[c / 4], 1.f, 0.f)[c % 4] > 0.f;
			anyState[3] |= ifelse(clockState_8[c / 4], 1.f, 0.f)[c % 4] > 0.f;
			anyState[4] |= ifelse(clockState_16[c / 4], 1.f, 0.f)[c % 4] > 0.f;
		}

		// Set lights
		if (lightDivider.process()) {
			float lightTime = args.sampleTime * lightDivider.getDivision();

			for (int i = 0; i < 5; i++) {
				lights[F_1_LIGHT + 3 * i + 0].setBrightnessSmooth(anyState[i] && numPolyphonyEngines == 1, lightTime);
				lights[F_1_LIGHT + 3 * i + 1].setBrightness(0.f);
				lights[F_1_LIGHT + 3 * i + 2].setBrightnessSmooth(anyState[i] && numPolyphonyEngines > 1, lightTime);
			}
		}
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* removeClockDCJ = json_object_get(rootJ, "removeClockDC");
		if (removeClockDCJ)
			removeClockDC = json_boolean_value(removeClockDCJ);
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "removeClockDC", json_boolean(removeClockDC));
		return rootJ;
	}
};


struct MuDiWidget : ModuleWidget {
	MuDiWidget(MuDi* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/MuDi.svg")));

		addChild(createWidget<Knurlie>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(box.size.x - RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(5.0, 15.138)), module, MuDi::CLOCK_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(5.0, 30.245)), module, MuDi::RESET_INPUT));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(5.0, 56.695)), module, MuDi::F_1_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(5.0, 70.45)), module, MuDi::F_2_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(5.0, 84.204)), module, MuDi::F_4_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(5.0, 97.959)), module, MuDi::F_8_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(5.0, 111.713)), module, MuDi::F_16_OUTPUT));

		addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(mm2px(Vec(1.95, 62.74)), module, MuDi::F_1_LIGHT));
		addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(mm2px(Vec(1.95, 76.325)), module, MuDi::F_2_LIGHT));
		addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(mm2px(Vec(1.95, 90.1)), module, MuDi::F_4_LIGHT));
		addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(mm2px(Vec(1.95, 103.874)), module, MuDi::F_8_LIGHT));
		addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(mm2px(Vec(1.95, 117.648)), module, MuDi::F_16_LIGHT));
	}

	void appendContextMenu(Menu* menu) override {
		MuDi* module = dynamic_cast<MuDi*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator());
		menu->addChild(createSubmenuItem("Hardware compatibility", "",
		[ = ](Menu * menu) {
			menu->addChild(createBoolPtrMenuItem("Remove DC from clock outs", "", &module->removeClockDC));
		}));
	}
};


Model* modelMuDi = createModel<MuDi, MuDiWidget>("MuDi");