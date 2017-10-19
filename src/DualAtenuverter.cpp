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

	DualAtenuverter() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS) {}
	void step() override;
};


void DualAtenuverter::step() {
	float out1 = inputs[IN1_INPUT].value * params[ATEN1_PARAM].value + params[OFFSET1_PARAM].value;
	float out2 = inputs[IN2_INPUT].value * params[ATEN2_PARAM].value + params[OFFSET2_PARAM].value;
	out1 = clampf(out1, -10.0, 10.0);
	out2 = clampf(out2, -10.0, 10.0);

	outputs[OUT1_OUTPUT].value = out1;
	outputs[OUT2_OUTPUT].value = out2;
	lights[0] = out1 / 5.0;
	lights[1] = out2 / 5.0;
}


DualAtenuverterWidget::DualAtenuverterWidget() {
	DualAtenuverter *module = new DualAtenuverter();
	setModule(module);
	box.size = Vec(15*5, 380);

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/DualAtenuverter.svg")));
		addChild(panel);
	}

	addChild(createScrew<Knurlie>(Vec(15, 0)));
	addChild(createScrew<Knurlie>(Vec(15, 365)));

	addParam(createParam<Davies1900hWhiteKnob>(Vec(20, 33), module, DualAtenuverter::ATEN1_PARAM, -1.0, 1.0, 0.0));
	addParam(createParam<Davies1900hRedKnob>(Vec(20, 91), module, DualAtenuverter::OFFSET1_PARAM, -10.0, 10.0, 0.0));
	addParam(createParam<Davies1900hWhiteKnob>(Vec(20, 201), module, DualAtenuverter::ATEN2_PARAM, -1.0, 1.0, 0.0));
	addParam(createParam<Davies1900hRedKnob>(Vec(20, 260), module, DualAtenuverter::OFFSET2_PARAM, -10.0, 10.0, 0.0));

	addInput(createInput<PJ3410Port>(Vec(4, 149), module, DualAtenuverter::IN1_INPUT));
	addOutput(createOutput<PJ3410Port>(Vec(39, 149), module, DualAtenuverter::OUT1_OUTPUT));

	addInput(createInput<PJ3410Port>(Vec(4, 316), module, DualAtenuverter::IN2_INPUT));
	addOutput(createOutput<PJ3410Port>(Vec(39, 316), module, DualAtenuverter::OUT2_OUTPUT));

	addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(33, 143), &module->lights[0]));
	addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(33, 311), &module->lights[1]));
}
