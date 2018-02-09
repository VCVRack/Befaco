#include "Befaco.hpp"
#include "dsp/digital.hpp"


struct Rampage : Module {
	enum ParamIds {
		RANGE_A_PARAM,
		RANGE_B_PARAM,
		SHAPE_A_PARAM,
		SHAPE_B_PARAM,
		TRIGG_A_PARAM,
		TRIGG_B_PARAM,
		RISE_A_PARAM,
		RISE_B_PARAM,
		FALL_A_PARAM,
		FALL_B_PARAM,
		CYCLE_A_PARAM,
		CYCLE_B_PARAM,
		BALANCE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		IN_A_INPUT,
		IN_B_INPUT,
		TRIGG_A_INPUT,
		TRIGG_B_INPUT,
		RISE_CV_A_INPUT,
		RISE_CV_B_INPUT,
		FALL_CV_A_INPUT,
		FALL_CV_B_INPUT,
		EXP_CV_A_INPUT,
		EXP_CV_B_INPUT,
		CYCLE_A_INPUT,
		CYCLE_B_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		RISING_A_OUTPUT,
		RISING_B_OUTPUT,
		FALLING_A_OUTPUT,
		FALLING_B_OUTPUT,
		EOC_A_OUTPUT,
		EOC_B_OUTPUT,
		OUT_A_OUTPUT,
		OUT_B_OUTPUT,
		COMPARATOR_OUTPUT,
		MIN_OUTPUT,
		MAX_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		COMPARATOR_LIGHT,
		MIN_LIGHT,
		MAX_LIGHT,
		OUT_A_LIGHT,
		OUT_B_LIGHT,
		RISING_A_LIGHT,
		RISING_B_LIGHT,
		FALLING_A_LIGHT,
		FALLING_B_LIGHT,
		NUM_LIGHTS
	};

	float out[2] = {};
	bool gate[2] = {};
	SchmittTrigger trigger[2];
	PulseGenerator endOfCyclePulse[2];

	Rampage() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;
};


static float shapeDelta(float delta, float tau, float shape) {
	float lin = sgn(delta) * 10.0 / tau;
	if (shape < 0.0) {
		float log = sgn(delta) * 40.0 / tau / (fabsf(delta) + 1.0);
		return crossfade(lin, log, -shape * 0.95f);
	}
	else {
		float exp = M_E * delta / tau;
		return crossfade(lin, exp, shape * 0.90f);
	}
}

void Rampage::step() {
	for (int c = 0; c < 2; c++) {
		float in = inputs[IN_A_INPUT + c].value;
		if (trigger[c].process(params[TRIGG_A_PARAM + c].value * 10.0 + inputs[TRIGG_A_INPUT + c].value / 2.0)) {
			gate[c] = true;
		}
		if (gate[c]) {
			in = 10.0;
		}

		float shape = params[SHAPE_A_PARAM + c].value;
		float delta = in - out[c];

		// Integrator
		float minTime;
		switch ((int) params[RANGE_A_PARAM + c].value) {
			case 0: minTime = 1e-2; break;
			case 1: minTime = 1e-3; break;
			default: minTime = 1e-1; break;
		}

		bool rising = false;
		bool falling = false;

		if (delta > 0) {
			// Rise
			float riseCv = params[RISE_A_PARAM + c].value - inputs[EXP_CV_A_INPUT + c].value / 10.0 + inputs[RISE_CV_A_INPUT + c].value / 10.0;
			riseCv = clamp(riseCv, 0.0f, 1.0f);
			float rise = minTime * powf(2.0, riseCv * 10.0);
			out[c] += shapeDelta(delta, rise, shape) * engineGetSampleTime();
			rising = (in - out[c] > 1e-3);
			if (!rising) {
				gate[c] = false;
			}
		}
		else if (delta < 0) {
			// Fall
			float fallCv = params[FALL_A_PARAM + c].value - inputs[EXP_CV_A_INPUT + c].value / 10.0 + inputs[FALL_CV_A_INPUT + c].value / 10.0;
			fallCv = clamp(fallCv, 0.0f, 1.0f);
			float fall = minTime * powf(2.0, fallCv * 10.0);
			out[c] += shapeDelta(delta, fall, shape) * engineGetSampleTime();
			falling = (in - out[c] < -1e-3);
			if (!falling) {
				// End of cycle, check if we should turn the gate back on (cycle mode)
				endOfCyclePulse[c].trigger(1e-3);
				if (params[CYCLE_A_PARAM + c].value * 10.0 + inputs[CYCLE_A_INPUT + c].value >= 4.0) {
					gate[c] = true;
				}
			}
		}
		else {
			gate[c] = false;
		}

		if (!rising && !falling) {
			out[c] = in;
		}

		outputs[RISING_A_OUTPUT + c].value = (rising ? 10.0 : 0.0);
		outputs[FALLING_A_OUTPUT + c].value = (falling ? 10.0 : 0.0);
		lights[RISING_A_LIGHT + c].value = (rising ? 1.0 : 0.0);
		lights[FALLING_A_LIGHT + c].value = (falling ? 1.0 : 0.0);
		outputs[EOC_A_OUTPUT + c].value = (endOfCyclePulse[c].process(engineGetSampleTime()) ? 10.0 : 0.0);
		outputs[OUT_A_OUTPUT + c].value = out[c];
		lights[OUT_A_LIGHT + c].value = out[c] / 10.0;
	}

	// Logic
	float balance = params[BALANCE_PARAM].value;
	float a = out[0];
	float b = out[1];
	if (balance < 0.5)
		b *= 2.0 * balance;
	else if (balance > 0.5)
		a *= 2.0 * (1.0 - balance);
	outputs[COMPARATOR_OUTPUT].value = (b > a ? 10.0 : 0.0);
	outputs[MIN_OUTPUT].value = fminf(a, b);
	outputs[MAX_OUTPUT].value = fmaxf(a, b);
	// Lights
	lights[COMPARATOR_LIGHT].value = outputs[COMPARATOR_OUTPUT].value / 10.0;
	lights[MIN_LIGHT].value = outputs[MIN_OUTPUT].value / 10.0;
	lights[MAX_LIGHT].value = outputs[MAX_OUTPUT].value / 10.0;
}


RampageWidget::RampageWidget() {
	Rampage *module = new Rampage();
	setModule(module);
	box.size = Vec(15*18, 380);

	{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/Rampage.svg")));
		addChild(panel);
	}

	addChild(createScrew<Knurlie>(Vec(15, 0)));
	addChild(createScrew<Knurlie>(Vec(box.size.x-30, 0)));
	addChild(createScrew<Knurlie>(Vec(15, 365)));
	addChild(createScrew<Knurlie>(Vec(box.size.x-30, 365)));

	addInput(createInput<PJ301MPort>(Vec(14, 30), module, Rampage::IN_A_INPUT));
	addInput(createInput<PJ301MPort>(Vec(52, 37), module, Rampage::TRIGG_A_INPUT));
	addInput(createInput<PJ301MPort>(Vec(8, 268), module, Rampage::RISE_CV_A_INPUT));
	addInput(createInput<PJ301MPort>(Vec(67, 268), module, Rampage::FALL_CV_A_INPUT));
	addInput(createInput<PJ301MPort>(Vec(38, 297), module, Rampage::EXP_CV_A_INPUT));
	addInput(createInput<PJ301MPort>(Vec(102, 290), module, Rampage::CYCLE_A_INPUT));
	addInput(createInput<PJ301MPort>(Vec(229, 30), module, Rampage::IN_B_INPUT));
	addInput(createInput<PJ301MPort>(Vec(192, 37), module, Rampage::TRIGG_B_INPUT));
	addInput(createInput<PJ301MPort>(Vec(176, 268), module, Rampage::RISE_CV_B_INPUT));
	addInput(createInput<PJ301MPort>(Vec(237, 268), module, Rampage::FALL_CV_B_INPUT));
	addInput(createInput<PJ301MPort>(Vec(207, 297), module, Rampage::EXP_CV_B_INPUT));
	addInput(createInput<PJ301MPort>(Vec(143, 290), module, Rampage::CYCLE_B_INPUT));

	addParam(createParam<BefacoSwitch>(Vec(94, 32), module, Rampage::RANGE_A_PARAM, 0.0, 2.0, 0.0));
	addParam(createParam<BefacoTinyKnob>(Vec(27, 90), module, Rampage::SHAPE_A_PARAM, -1.0, 1.0, 0.0));
	addParam(createParam<BefacoPush>(Vec(72, 82), module, Rampage::TRIGG_A_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<BefacoSlidePot>(Vec(16, 135), module, Rampage::RISE_A_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<BefacoSlidePot>(Vec(57, 135), module, Rampage::FALL_A_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<BefacoSwitch>(Vec(101, 238), module, Rampage::CYCLE_A_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<BefacoSwitch>(Vec(147, 32), module, Rampage::RANGE_B_PARAM, 0.0, 2.0, 0.0));
	addParam(createParam<BefacoTinyKnob>(Vec(217, 90), module, Rampage::SHAPE_B_PARAM, -1.0, 1.0, 0.0));
	addParam(createParam<BefacoPush>(Vec(170, 82), module, Rampage::TRIGG_B_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<BefacoSlidePot>(Vec(197, 135), module, Rampage::RISE_B_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<BefacoSlidePot>(Vec(238, 135), module, Rampage::FALL_B_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<BefacoSwitch>(Vec(141, 238), module, Rampage::CYCLE_B_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<Davies1900hWhiteKnob>(Vec(117, 76), module, Rampage::BALANCE_PARAM, 0.0, 1.0, 0.5));

	addOutput(createOutput<PJ301MPort>(Vec(8, 326), module, Rampage::RISING_A_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(68, 326), module, Rampage::FALLING_A_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(104, 326), module, Rampage::EOC_A_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(102, 195), module, Rampage::OUT_A_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(177, 326), module, Rampage::RISING_B_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(237, 326), module, Rampage::FALLING_B_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(140, 326), module, Rampage::EOC_B_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(142, 195), module, Rampage::OUT_B_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(122, 133), module, Rampage::COMPARATOR_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(89, 157), module, Rampage::MIN_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(155, 157), module, Rampage::MAX_OUTPUT));

	addChild(createLight<SmallLight<RedLight>>(Vec(132, 167), module, Rampage::COMPARATOR_LIGHT));
	addChild(createLight<SmallLight<RedLight>>(Vec(123, 174), module, Rampage::MIN_LIGHT));
	addChild(createLight<SmallLight<RedLight>>(Vec(141, 174), module, Rampage::MAX_LIGHT));
	addChild(createLight<SmallLight<RedLight>>(Vec(126, 185), module, Rampage::OUT_A_LIGHT));
	addChild(createLight<SmallLight<RedLight>>(Vec(138, 185), module, Rampage::OUT_B_LIGHT));
	addChild(createLight<SmallLight<RedLight>>(Vec(18, 312), module, Rampage::RISING_A_LIGHT));
	addChild(createLight<SmallLight<RedLight>>(Vec(78, 312), module, Rampage::FALLING_A_LIGHT));
	addChild(createLight<SmallLight<RedLight>>(Vec(187, 312), module, Rampage::RISING_B_LIGHT));
	addChild(createLight<SmallLight<RedLight>>(Vec(247, 312), module, Rampage::FALLING_B_LIGHT));
}
