#include "Befaco.hpp"


struct SlewLimiter : Module {
	enum ParamIds {
		SHAPE_PARAM,
		RISE_PARAM,
		FALL_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		RISE_INPUT,
		FALL_INPUT,
		IN_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_OUTPUT,
		NUM_OUTPUTS
	};

	float output = 0.0;

	SlewLimiter();
	void step();
};


SlewLimiter::SlewLimiter() {
	params.resize(NUM_PARAMS);
	inputs.resize(NUM_INPUTS);
	outputs.resize(NUM_OUTPUTS);
}

void SlewLimiter::step() {
	float input = getf(inputs[IN_INPUT]);
	float shape = params[SHAPE_PARAM];

	// minimum and maximum slopes in volts per second
	const float slewMin = 0.1;
	const float slewMax = 40000.0;
	// Amount of extra slew per voltage difference
	const float shapeScale = 1/10.0;

	// Rise
	if (input > output) {
		float rise = getf(inputs[RISE_INPUT]) + params[RISE_PARAM];
		float slew = slewMax * powf(slewMin / slewMax, rise);
		output += slew * crossf(1.0, shapeScale * (input - output), shape) / gSampleRate;
		if (output > input)
			output = input;
	}
	// Fall
	else if (input < output) {
		float fall = getf(inputs[FALL_INPUT]) + params[FALL_PARAM];
		float slew = slewMax * powf(slewMin / slewMax, fall);
		output -= slew * crossf(1.0, shapeScale * (output - input), shape) / gSampleRate;
		if (output < input)
			output = input;
	}

	setf(outputs[OUT_OUTPUT], output);
}


SlewLimiterWidget::SlewLimiterWidget() {
	SlewLimiter *module = new SlewLimiter();
	setModule(module);
	box.size = Vec(15*6, 380);

	{
		Panel *panel = new DarkPanel();
		panel->box.size = box.size;
		panel->backgroundImage = Image::load("plugins/Befaco/res/SlewLimiter.png");
		addChild(panel);
	}

	addChild(createScrew<BlackScrew>(Vec(15, 0)));
	addChild(createScrew<BlackScrew>(Vec(15, 365)));

	addParam(createParam<Davies1900hWhiteKnob>(Vec(26, 39), module, SlewLimiter::SHAPE_PARAM, 0.0, 1.0, 0.0));

	addParam(createParam<BefacoSlidePot>(Vec(17, 100), module, SlewLimiter::RISE_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<BefacoSlidePot>(Vec(61, 100), module, SlewLimiter::FALL_PARAM, 0.0, 1.0, 0.0));

	addInput(createInput<PJ3410Port>(Vec(6, 270), module, SlewLimiter::RISE_INPUT));
	addInput(createInput<PJ3410Port>(Vec(52, 270), module, SlewLimiter::FALL_INPUT));

	addInput(createInput<PJ3410Port>(Vec(6, 320), module, SlewLimiter::IN_INPUT));
	addOutput(createOutput<PJ3410Port>(Vec(52, 320), module, SlewLimiter::OUT_OUTPUT));
}
