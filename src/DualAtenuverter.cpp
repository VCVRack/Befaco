#include "Befaco.hpp"


/** Yes, in English this would be spelled "Attenuverter", but since attenuator is "atenuador" in Spanish, I suppose they (accidentally or not) went with one "t". */
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

	float lights[2] = {};

	DualAtenuverter();
	void step();
};


DualAtenuverter::DualAtenuverter() {
	params.resize(NUM_PARAMS);
	inputs.resize(NUM_INPUTS);
	outputs.resize(NUM_OUTPUTS);
}

void DualAtenuverter::step() {
	float out1 = getf(inputs[IN1_INPUT]) * params[ATEN1_PARAM] + params[OFFSET1_PARAM];
	float out2 = getf(inputs[IN2_INPUT]) * params[ATEN2_PARAM] + params[OFFSET2_PARAM];
	out1 = clampf(out1, -10.0, 10.0);
	out2 = clampf(out2, -10.0, 10.0);

	setf(outputs[OUT1_OUTPUT], out1);
	setf(outputs[OUT2_OUTPUT], out2);
	lights[0] = out1 / 5.0;
	lights[1] = out2 / 5.0;
}


DualAtenuverterWidget::DualAtenuverterWidget() {
	DualAtenuverter *module = new DualAtenuverter();
	setModule(module);
	box.size = Vec(15*5, 380);

	{
		Panel *panel = new DarkPanel();
		panel->box.size = box.size;
		panel->backgroundImage = Image::load("plugins/Befaco/res/DualAtenuverter.png");
		addChild(panel);
	}

	addChild(createScrew<BlackScrew>(Vec(15, 0)));
	addChild(createScrew<BlackScrew>(Vec(15, 365)));

	addParam(createParam<Davies1900hWhiteKnob>(Vec(19, 33), module, DualAtenuverter::ATEN1_PARAM, -1.0, 1.0, 0.0));
	addParam(createParam<Davies1900hRedKnob>(Vec(19, 91), module, DualAtenuverter::OFFSET1_PARAM, -10.0, 10.0, 0.0));
	addParam(createParam<Davies1900hWhiteKnob>(Vec(19, 201), module, DualAtenuverter::ATEN2_PARAM, -1.0, 1.0, 0.0));
	addParam(createParam<Davies1900hRedKnob>(Vec(19, 260), module, DualAtenuverter::OFFSET2_PARAM, -10.0, 10.0, 0.0));

	addInput(createInput<PJ3410Port>(Vec(4, 149), module, DualAtenuverter::IN1_INPUT));
	addOutput(createOutput<PJ3410Port>(Vec(39, 149), module, DualAtenuverter::OUT1_OUTPUT));

	addInput(createInput<PJ3410Port>(Vec(4, 316), module, DualAtenuverter::IN2_INPUT));
	addOutput(createOutput<PJ3410Port>(Vec(39, 316), module, DualAtenuverter::OUT2_OUTPUT));

	addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(33, 143), &module->lights[0]));
	addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(33, 311), &module->lights[1]));
}
