#include "Befaco.hpp"


struct Rampage : Module {
	enum ParamIds {
		RANGE_A_PARAM,
		SHAPE_A_PARAM,
		TRIGG_A_PARAM,
		RISE_A_PARAM,
		FALL_A_PARAM,
		CYCLE_A_PARAM,
		RANGE_B_PARAM,
		SHAPE_B_PARAM,
		TRIGG_B_PARAM,
		RISE_B_PARAM,
		FALL_B_PARAM,
		CYCLE_B_PARAM,
		BALANCE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		IN_A_INPUT,
		TRIGG_A_INPUT,
		RISE_CV_A_INPUT,
		FALL_CV_A_INPUT,
		EXP_CV_A_INPUT,
		CYCLE_A_INPUT,
		IN_B_INPUT,
		TRIGG_B_INPUT,
		RISE_CV_B_INPUT,
		FALL_CV_B_INPUT,
		EXP_CV_B_INPUT,
		CYCLE_B_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		RISING_A_OUTPUT,
		FALLING_A_OUTPUT,
		EOC_A_OUTPUT,
		OUT_A_OUTPUT,
		RISING_B_OUTPUT,
		FALLING_B_OUTPUT,
		EOC_B_OUTPUT,
		OUT_B_OUTPUT,
		COMPARATOR_OUTPUT,
		MIN_OUTPUT,
		MAX_OUTPUT,
		NUM_OUTPUTS,
	};

	Rampage();
	void step();
};


Rampage::Rampage() {
	params.resize(NUM_PARAMS);
	inputs.resize(NUM_INPUTS);
	outputs.resize(NUM_OUTPUTS);
}

void Rampage::step() {
}


RampageWidget::RampageWidget() {
	Rampage *module = new Rampage();
	setModule(module);
	box.size = Vec(15*18, 380);

	{
		Panel *panel = new DarkPanel();
		panel->box.size = box.size;
		panel->backgroundImage = Image::load("plugins/Befaco/res/Rampage.png");
		addChild(panel);
	}

	addChild(createScrew<ScrewBlack>(Vec(15, 0)));
	addChild(createScrew<ScrewBlack>(Vec(box.size.x-30, 0)));
	addChild(createScrew<ScrewBlack>(Vec(15, 365)));
	addChild(createScrew<ScrewBlack>(Vec(box.size.x-30, 365)));

	addInput(createInput<PJ3410Port>(Vec(14-3, 30-3), module, Rampage::IN_A_INPUT));
	addInput(createInput<PJ3410Port>(Vec(52-3, 37-3), module, Rampage::TRIGG_A_INPUT));
	addInput(createInput<PJ3410Port>(Vec(8-3, 268-3), module, Rampage::RISE_CV_A_INPUT));
	addInput(createInput<PJ3410Port>(Vec(67-3, 268-3), module, Rampage::FALL_CV_A_INPUT));
	addInput(createInput<PJ3410Port>(Vec(38-3, 297-3), module, Rampage::EXP_CV_A_INPUT));
	addInput(createInput<PJ3410Port>(Vec(102-3, 290-3), module, Rampage::CYCLE_A_INPUT));
	addInput(createInput<PJ3410Port>(Vec(230-3, 30-3), module, Rampage::IN_B_INPUT));
	addInput(createInput<PJ3410Port>(Vec(192-3, 37-3), module, Rampage::TRIGG_B_INPUT));
	addInput(createInput<PJ3410Port>(Vec(176-3, 268-3), module, Rampage::RISE_CV_B_INPUT));
	addInput(createInput<PJ3410Port>(Vec(237-3, 268-3), module, Rampage::FALL_CV_B_INPUT));
	addInput(createInput<PJ3410Port>(Vec(207-3, 297-3), module, Rampage::EXP_CV_B_INPUT));
	addInput(createInput<PJ3410Port>(Vec(142-3, 290-3), module, Rampage::CYCLE_B_INPUT));

	addParam(createParam<BefacoSwitch>(Vec(96-2, 35-3), module, Rampage::RANGE_A_PARAM, 0.0, 2.0, 0.0));
	addParam(createParam<BefacoTinyKnob>(Vec(27, 90), module, Rampage::SHAPE_A_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<BefacoPush>(Vec(72, 82), module, Rampage::TRIGG_A_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<BefacoSlidePot>(Vec(21-5, 140-5), module, Rampage::RISE_A_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<BefacoSlidePot>(Vec(62-5, 140-5), module, Rampage::FALL_A_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<BefacoSwitch>(Vec(101, 240-2), module, Rampage::CYCLE_A_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<BefacoSwitch>(Vec(149-2, 35-3), module, Rampage::RANGE_B_PARAM, 0.0, 2.0, 0.0));
	addParam(createParam<BefacoTinyKnob>(Vec(217, 90), module, Rampage::SHAPE_B_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<BefacoPush>(Vec(170, 82), module, Rampage::TRIGG_B_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<BefacoSlidePot>(Vec(202-5, 140-5), module, Rampage::RISE_B_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<BefacoSlidePot>(Vec(243-5, 140-5), module, Rampage::FALL_B_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<BefacoSwitch>(Vec(141, 240-2), module, Rampage::CYCLE_B_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<Davies1900hWhiteKnob>(Vec(117, 76), module, Rampage::BALANCE_PARAM, 0.0, 1.0, 0.0));

	addOutput(createOutput<PJ3410Port>(Vec(8-3, 326-3), module, Rampage::RISING_A_OUTPUT));
	addOutput(createOutput<PJ3410Port>(Vec(67-3, 326-3), module, Rampage::FALLING_A_OUTPUT));
	addOutput(createOutput<PJ3410Port>(Vec(104-3, 326-3), module, Rampage::EOC_A_OUTPUT));
	addOutput(createOutput<PJ3410Port>(Vec(102-3, 195-3), module, Rampage::OUT_A_OUTPUT));
	addOutput(createOutput<PJ3410Port>(Vec(176-3, 326-3), module, Rampage::RISING_B_OUTPUT));
	addOutput(createOutput<PJ3410Port>(Vec(237-3, 326-3), module, Rampage::FALLING_B_OUTPUT));
	addOutput(createOutput<PJ3410Port>(Vec(140-3, 326-3), module, Rampage::EOC_B_OUTPUT));
	addOutput(createOutput<PJ3410Port>(Vec(142-3, 195-3), module, Rampage::OUT_B_OUTPUT));
	addOutput(createOutput<PJ3410Port>(Vec(122-3, 133-3), module, Rampage::COMPARATOR_OUTPUT));
	addOutput(createOutput<PJ3410Port>(Vec(89-3, 156-3), module, Rampage::MIN_OUTPUT));
	addOutput(createOutput<PJ3410Port>(Vec(155-3, 156-3), module, Rampage::MAX_OUTPUT));
}
