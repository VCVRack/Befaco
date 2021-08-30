#include "plugin.hpp"

struct Mex : Module {
	static const int numSteps = 8;
	enum ParamIds {
		ENUMS(STEP_PARAM, numSteps),
		NUM_PARAMS
	};
	enum InputIds {
		GATE_IN_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(LED, numSteps),
		NUM_LIGHTS
	};
	enum StepState {
		GATE_IN_MODE,
		MUTE_MODE,
		MUXLICER_MODE
	};

	MexMessage leftMessages[2] = {};

	Mex() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		for (int i = 0; i < 8; ++i) {
			configParam(STEP_PARAM + i, 0.f, 2.f, 0.f, string::f("Step %d", i));
		}

		leftExpander.producerMessage = &(leftMessages[0]);
		leftExpander.consumerMessage = &(leftMessages[1]);
	}

	void process(const ProcessArgs& args) override {

		for (int i = 0; i < 8; i++) {
			lights[i].setBrightness(0.f);
		}

		if (leftExpander.module && leftExpander.module->model == modelMuxlicer) {

			// Get consumer message
			MexMessage* message = (MexMessage*) leftExpander.consumerMessage;

			float gate = 0.f;

			if (message->isPlaying) {
				const int currentStep = clamp(message->addressIndex, 0, 7);
				StepState state = (StepState) params[STEP_PARAM + currentStep].getValue();
				if (state == MUXLICER_MODE) {
					gate = message->allGates;
				}
				else if (state == GATE_IN_MODE) {
					gate = inputs[GATE_IN_INPUT].getNormalVoltage(message->outputClock);
				}

				lights[currentStep].setBrightness(gate);
			}

			outputs[OUT_OUTPUT].setVoltage(gate);
		}
	}
};


struct MexWidget : ModuleWidget {
	MexWidget(Mex* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Mex.svg")));

		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<BefacoSwitchHorizontal>(mm2px(Vec(8.088, 13.063)),  module, Mex::STEP_PARAM + 0));
		addParam(createParamCentered<BefacoSwitchHorizontal>(mm2px(Vec(8.088, 25.706)),  module, Mex::STEP_PARAM + 1));
		addParam(createParamCentered<BefacoSwitchHorizontal>(mm2px(Vec(8.088, 38.348)),  module, Mex::STEP_PARAM + 2));
		addParam(createParamCentered<BefacoSwitchHorizontal>(mm2px(Vec(8.088, 50.990)),  module, Mex::STEP_PARAM + 3));
		addParam(createParamCentered<BefacoSwitchHorizontal>(mm2px(Vec(8.088, 63.632)),  module, Mex::STEP_PARAM + 4));
		addParam(createParamCentered<BefacoSwitchHorizontal>(mm2px(Vec(8.088, 76.274)),  module, Mex::STEP_PARAM + 5));
		addParam(createParamCentered<BefacoSwitchHorizontal>(mm2px(Vec(8.088, 88.916)),  module, Mex::STEP_PARAM + 6));
		addParam(createParamCentered<BefacoSwitchHorizontal>(mm2px(Vec(8.088, 101.559)), module, Mex::STEP_PARAM + 7));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(4.978, 113.445)), module, Mex::GATE_IN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.014, 113.4)), module, Mex::OUT_OUTPUT));

		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(17.7, 13.063)),  module, Mex::LED + 0));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(17.7, 25.706)),  module, Mex::LED + 1));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(17.7, 38.348)),  module, Mex::LED + 2));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(17.7, 50.990)),  module, Mex::LED + 3));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(17.7, 63.632)),  module, Mex::LED + 4));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(17.7, 76.274)),  module, Mex::LED + 5));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(17.7, 88.916)),  module, Mex::LED + 6));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(17.7, 101.558)), module, Mex::LED + 7));
	}
};


Model* modelMex = createModel<Mex, MexWidget>("Mex");