#include "Befaco.hpp"


struct DualAtenuverter : Module {
	enum ParamIds {
		ATEN1_PARAM,
		OFFSET1_PARAM,
		ATEN2_PARAM,
		OFFSET2_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		IN1_INPUT,
		IN2_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT1_OUTPUT,
		OUT2_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		OUT1_POS_LIGHT,
		OUT1_NEG_LIGHT,
		OUT2_POS_LIGHT,
		OUT2_NEG_LIGHT,
		NUM_LIGHTS
	};

	DualAtenuverter() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		params[ATEN1_PARAM].config(-1.0, 1.0, 0.0);
		params[OFFSET1_PARAM].config(-10.0, 10.0, 0.0);
		params[ATEN2_PARAM].config(-1.0, 1.0, 0.0);
		params[OFFSET2_PARAM].config(-10.0, 10.0, 0.0);
	}

	void step() override {
		float out1 = inputs[IN1_INPUT].value * params[ATEN1_PARAM].value + params[OFFSET1_PARAM].value;
		float out2 = inputs[IN2_INPUT].value * params[ATEN2_PARAM].value + params[OFFSET2_PARAM].value;
		out1 = clamp(out1, -10.f, 10.f);
		out2 = clamp(out2, -10.f, 10.f);

		outputs[OUT1_OUTPUT].value = out1;
		outputs[OUT2_OUTPUT].value = out2;
		lights[OUT1_POS_LIGHT].setBrightnessSmooth(std::max(0.f, out1 / 5.f));
		lights[OUT1_NEG_LIGHT].setBrightnessSmooth(std::max(0.f, -out1 / 5.f));
		lights[OUT2_POS_LIGHT].setBrightnessSmooth(std::max(0.f, out2 / 5.f));
		lights[OUT2_NEG_LIGHT].setBrightnessSmooth(std::max(0.f, -out2 / 5.f));
	}
};


struct DualAtenuverterWidget : ModuleWidget {
	DualAtenuverterWidget(DualAtenuverter *module) : ModuleWidget(module) {
		setPanel(SVG::load(asset::plugin(plugin, "res/DualAtenuverter.svg")));

		addChild(createWidget<Knurlie>(Vec(15, 0)));
		addChild(createWidget<Knurlie>(Vec(15, 365)));

		addParam(createParam<Davies1900hWhiteKnob>(Vec(20, 33), module, DualAtenuverter::ATEN1_PARAM));
		addParam(createParam<Davies1900hRedKnob>(Vec(20, 91), module, DualAtenuverter::OFFSET1_PARAM));
		addParam(createParam<Davies1900hWhiteKnob>(Vec(20, 201), module, DualAtenuverter::ATEN2_PARAM));
		addParam(createParam<Davies1900hRedKnob>(Vec(20, 260), module, DualAtenuverter::OFFSET2_PARAM));

		addInput(createInput<PJ301MPort>(Vec(7, 152), module, DualAtenuverter::IN1_INPUT));
		addOutput(createOutput<PJ301MPort>(Vec(43, 152), module, DualAtenuverter::OUT1_OUTPUT));

		addInput(createInput<PJ301MPort>(Vec(7, 319), module, DualAtenuverter::IN2_INPUT));
		addOutput(createOutput<PJ301MPort>(Vec(43, 319), module, DualAtenuverter::OUT2_OUTPUT));

		addChild(createLight<MediumLight<GreenRedLight>>(Vec(33, 143), module, DualAtenuverter::OUT1_POS_LIGHT));
		addChild(createLight<MediumLight<GreenRedLight>>(Vec(33, 311), module, DualAtenuverter::OUT2_POS_LIGHT));
	}
};


Model *modelDualAtenuverter = createModel<DualAtenuverter, DualAtenuverterWidget>("DualAtenuverter");
