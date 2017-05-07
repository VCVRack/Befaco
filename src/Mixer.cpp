#include "Befaco.hpp"


struct Mixer : Module {
	enum ParamIds {
		CH1_PARAM,
		CH2_PARAM,
		CH3_PARAM,
		CH4_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		IN1_INPUT,
		IN2_INPUT,
		IN3_INPUT,
		IN4_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT1_OUTPUT,
		OUT2_OUTPUT,
		NUM_OUTPUTS
	};

	float lights[1] = {};

	Mixer();
	void step();
};


Mixer::Mixer() {
	params.resize(NUM_PARAMS);
	inputs.resize(NUM_INPUTS);
	outputs.resize(NUM_OUTPUTS);
}

void Mixer::step() {
	float in1 = getf(inputs[IN1_INPUT]) * params[CH1_PARAM];
	float in2 = getf(inputs[IN2_INPUT]) * params[CH2_PARAM];
	float in3 = getf(inputs[IN3_INPUT]) * params[CH3_PARAM];
	float in4 = getf(inputs[IN4_INPUT]) * params[CH4_PARAM];

	float out = in1 + in2 + in3 + in4;
	setf(outputs[OUT1_OUTPUT], out);
	setf(outputs[OUT2_OUTPUT], -out);
	lights[0] = out / 5.0;
}


MixerWidget::MixerWidget() {
	Mixer *module = new Mixer();
	setModule(module);
	box.size = Vec(15*5, 380);

	{
		Panel *panel = new DarkPanel();
		panel->box.size = box.size;
		panel->backgroundImage = Image::load("plugins/Befaco/res/Mixer.png");
		addChild(panel);
	}

	addChild(createScrew<ScrewBlack>(Vec(15, 0)));
	addChild(createScrew<ScrewBlack>(Vec(15, 365)));

	addParam(createParam<Davies1900hWhiteKnob>(Vec(19, 32), module, Mixer::CH1_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<Davies1900hWhiteKnob>(Vec(19, 85), module, Mixer::CH2_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<Davies1900hWhiteKnob>(Vec(19, 137), module, Mixer::CH3_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<Davies1900hWhiteKnob>(Vec(19, 190), module, Mixer::CH4_PARAM, 0.0, 1.0, 0.0));

	addInput(createInput<PJ3410Port>(Vec(4, 239), module, Mixer::IN1_INPUT));
	addInput(createInput<PJ3410Port>(Vec(40, 239), module, Mixer::IN2_INPUT));

	addInput(createInput<PJ3410Port>(Vec(4, 278), module, Mixer::IN3_INPUT));
	addInput(createInput<PJ3410Port>(Vec(40, 278), module, Mixer::IN4_INPUT));

	addOutput(createOutput<PJ3410Port>(Vec(4, 321), module, Mixer::OUT1_OUTPUT));
	addOutput(createOutput<PJ3410Port>(Vec(40, 321), module, Mixer::OUT2_OUTPUT));

	addChild(createValueLight<MediumLight<GreenRedPolarityLight>>(Vec(31, 309), &module->lights[0]));
}
