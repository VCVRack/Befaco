#include "Befaco.hpp"


struct Rampage : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		NUM_OUTPUTS
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

	addChild(createScrew<BlackScrew>(Vec(15, 0)));
	addChild(createScrew<BlackScrew>(Vec(box.size.x-30, 0)));
	addChild(createScrew<BlackScrew>(Vec(15, 365)));
	addChild(createScrew<BlackScrew>(Vec(box.size.x-30, 365)));

	// addParam(createParam<ExperimentalKnob>(Vec(0, 43), module, Rampage::RADIOACTIVITY_PARAM, 0.0, 1.0, 0.0));
	// addInput(createInput(Vec(10, 248), module, Rampage::RADIOACTIVITY_INPUT));
	// addOutput(createOutput(Vec(10, 306), module, Rampage::PULSE_OUTPUT));
}
