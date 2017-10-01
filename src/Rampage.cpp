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

	float lastOutA = 0.0;
	float lastOutB = 0.0;

	float comparatorLight = 0.0;
	float minLight = 0.0;
	float maxLight = 0.0;
	float outALight = 0.0;
	float outBLight = 0.0;
	float risingALight = 0.0;
	float fallingALight = 0.0;
	float risingBLight = 0.0;
	float fallingBLight = 0.0;

	Rampage() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS) {}
	void step();
};


void Rampage::step() {
	// TEMP
	float outA = inputs[IN_A_INPUT].value;
	float outB = inputs[IN_B_INPUT].value;
	outputs[OUT_A_OUTPUT].value = outA;
	outputs[OUT_B_OUTPUT].value = outB;
	outALight = outA / 10.0;
	outBLight = outB / 10.0;

	// Slope detector
	const float slopeThreshold = 1.0; // volts per second
#define SLOPE(out, lastOut, rising, falling, risingLight, fallingLight) { \
	float slope = (out - lastOut) / gSampleRate; \
	lastOut = out; \
	float rising = slope > slopeThreshold ? 10.0 : 0.0; \
	float falling = slope < -slopeThreshold ? 10.0 : 0.0; \
	outputs[rising].value = rising; \
	outputs[falling].value = falling; \
	risingLight = rising / 10.0; \
	fallingLight = falling / 10.0; \
}
	SLOPE(outA, lastOutA, RISING_A_OUTPUT, FALLING_A_OUTPUT, risingALight, fallingALight)
	SLOPE(outB, lastOutB, RISING_B_OUTPUT, FALLING_B_OUTPUT, risingBLight, fallingBLight)

	// Analog logic processor
	float balance = params[BALANCE_PARAM].value;
	const float balancePower = 0.5;
	outA *= powf(1.0 - balance, balancePower);
	outB *= powf(balance, balancePower);
	float max = fmaxf(outA, outB);
	float min = fminf(outA, outB);
	float comparator = outB > outA ? 10.0 : 0.0;
	outputs[MAX_OUTPUT].value = max;
	outputs[MIN_OUTPUT].value = min;
	outputs[COMPARATOR_OUTPUT].value = comparator;
	maxLight = max / 10.0;
	minLight = min / 10.0;
	comparatorLight = comparator / 20.0;
}


RampageWidget::RampageWidget() {
	Rampage *module = new Rampage();
	setModule(module);
	box.size = Vec(15*18, 380);

	{
		Panel *panel = new DarkPanel();
		panel->box.size = box.size;
		panel->backgroundImage = Image::load(assetPlugin(plugin, "res/Rampage.png"));
		addChild(panel);
	}

	addChild(createScrew<Knurlie>(Vec(15, 0)));
	addChild(createScrew<Knurlie>(Vec(box.size.x-30, 0)));
	addChild(createScrew<Knurlie>(Vec(15, 365)));
	addChild(createScrew<Knurlie>(Vec(box.size.x-30, 365)));

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
	addParam(createParam<BefacoTinyKnob>(Vec(27, 90), module, Rampage::SHAPE_A_PARAM, -1.0, 1.0, 0.0));
	addParam(createParam<BefacoPush>(Vec(72, 82), module, Rampage::TRIGG_A_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<BefacoSlidePot>(Vec(21-5, 140-5), module, Rampage::RISE_A_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<BefacoSlidePot>(Vec(62-5, 140-5), module, Rampage::FALL_A_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<BefacoSwitch>(Vec(101, 240-2), module, Rampage::CYCLE_A_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<BefacoSwitch>(Vec(149-2, 35-3), module, Rampage::RANGE_B_PARAM, 0.0, 2.0, 0.0));
	addParam(createParam<BefacoTinyKnob>(Vec(217, 90), module, Rampage::SHAPE_B_PARAM, -1.0, 1.0, 0.0));
	addParam(createParam<BefacoPush>(Vec(170, 82), module, Rampage::TRIGG_B_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<BefacoSlidePot>(Vec(202-5, 140-5), module, Rampage::RISE_B_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<BefacoSlidePot>(Vec(243-5, 140-5), module, Rampage::FALL_B_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<BefacoSwitch>(Vec(141, 240-2), module, Rampage::CYCLE_B_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<Davies1900hWhiteKnob>(Vec(117, 76), module, Rampage::BALANCE_PARAM, 0.0, 1.0, 0.5));

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

	addChild(createValueLight<SmallLight<RedValueLight>>(Vec(131, 167), &module->comparatorLight));
	addChild(createValueLight<SmallLight<RedValueLight>>(Vec(122, 174), &module->minLight));
	addChild(createValueLight<SmallLight<RedValueLight>>(Vec(140, 174), &module->maxLight));
	addChild(createValueLight<SmallLight<RedValueLight>>(Vec(125, 185), &module->outALight));
	addChild(createValueLight<SmallLight<RedValueLight>>(Vec(137, 185), &module->outBLight));
	addChild(createValueLight<SmallLight<RedValueLight>>(Vec(17, 312), &module->risingALight));
	addChild(createValueLight<SmallLight<RedValueLight>>(Vec(77, 312), &module->fallingALight));
	addChild(createValueLight<SmallLight<RedValueLight>>(Vec(186, 312), &module->risingBLight));
	addChild(createValueLight<SmallLight<RedValueLight>>(Vec(245, 312), &module->fallingBLight));
}
