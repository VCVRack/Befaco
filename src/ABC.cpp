#include "Befaco.hpp"


struct ABC : Module {
	enum ParamIds {
		B1_LEVEL_PARAM,
		C1_LEVEL_PARAM,
		B2_LEVEL_PARAM,
		C2_LEVEL_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		A1_INPUT,
		B1_INPUT,
		C1_INPUT,
		A2_INPUT,
		B2_INPUT,
		C2_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT1_OUTPUT,
		OUT2_OUTPUT,
		NUM_OUTPUTS
	};

	float lights[2] = {};

	ABC() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS) {}
	void step();
};


static float clip(float x) {
	x = clampf(x, -2.0, 2.0);
	return x / powf(1.0 + powf(x, 24.0), 1/24.0);
}

void ABC::step() {
	float a1 = inputs[A1_INPUT].value;
	float b1 = inputs[B1_INPUT].normalize(5.0) * 2.0*exponentialBipolar(80.0, params[B1_LEVEL_PARAM].value);
	float c1 = inputs[C1_INPUT].normalize(10.0) * exponentialBipolar(80.0, params[C1_LEVEL_PARAM].value);
	float out1 = a1 * b1 / 5.0 + c1;

	float a2 = inputs[A2_INPUT].value;
	float b2 = inputs[B2_INPUT].normalize(5.0) * 2.0*exponentialBipolar(80.0, params[B2_LEVEL_PARAM].value);
	float c2 = inputs[C2_INPUT].normalize(20.0) * exponentialBipolar(80.0, params[C2_LEVEL_PARAM].value);
	float out2 = a2 * b2 / 5.0 + c2;

	// Set outputs
	if (outputs[OUT1_OUTPUT].active) {
		outputs[OUT1_OUTPUT].value = clip(out1 / 10.0) * 10.0;
	}
	else {
		out2 += out1;
	}
	if (outputs[OUT2_OUTPUT].active) {
		outputs[OUT2_OUTPUT].value = clip(out2 / 10.0) * 10.0;
	}
	lights[0] = out1 / 5.0;
	lights[1] = out2 / 5.0;
}


ABCWidget::ABCWidget() {
	ABC *module = new ABC();
	setModule(module);
	box.size = Vec(15*6, 380);

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/ABC.svg")));
		addChild(panel);
	}

	addChild(createScrew<Knurlie>(Vec(15, 0)));
	addChild(createScrew<Knurlie>(Vec(15, 365)));

	addParam(createParam<Davies1900hRedKnob>(Vec(45, 37), module, ABC::B1_LEVEL_PARAM, -1.0, 1.0, 0.0));
	addParam(createParam<Davies1900hWhiteKnob>(Vec(45, 107), module, ABC::C1_LEVEL_PARAM, -1.0, 1.0, 0.0));
	addParam(createParam<Davies1900hRedKnob>(Vec(45, 204), module, ABC::B2_LEVEL_PARAM, -1.0, 1.0, 0.0));
	addParam(createParam<Davies1900hWhiteKnob>(Vec(45, 274), module, ABC::C2_LEVEL_PARAM, -1.0, 1.0, 0.0));

	addInput(createInput<PJ3410Port>(Vec(3, 24), module, ABC::A1_INPUT));
	addInput(createInput<PJ3410Port>(Vec(3, 66), module, ABC::B1_INPUT));
	addInput(createInput<PJ3410Port>(Vec(3, 108), module, ABC::C1_INPUT));
	addOutput(createOutput<PJ3410Port>(Vec(3, 150), module, ABC::OUT1_OUTPUT));
	addInput(createInput<PJ3410Port>(Vec(3, 191), module, ABC::A2_INPUT));
	addInput(createInput<PJ3410Port>(Vec(3, 233), module, ABC::B2_INPUT));
	addInput(createInput<PJ3410Port>(Vec(3, 275), module, ABC::C2_INPUT));
	addOutput(createOutput<PJ3410Port>(Vec(3, 317), module, ABC::OUT2_OUTPUT));

	addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(38, 162), &module->lights[0]));
	addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(38, 330), &module->lights[1]));
}
