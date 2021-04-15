#include "plugin.hpp"
#include "Common.hpp"

static float shapeDelta(float delta, float tau, float shape) {
	float lin = sgn(delta) * 10.f / tau;
	if (shape < 0.f) {
		float log = sgn(delta) * 40.f / tau / (std::fabs(delta) + 1.f);
		return crossfade(lin, log, -shape * 0.95f);
	}
	else {
		float exp = M_E * delta / tau;
		return crossfade(lin, exp, shape * 0.90f);
	}
}


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
	dsp::SchmittTrigger trigger[2];
	dsp::PulseGenerator endOfCyclePulse[2];

	Rampage() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(RANGE_A_PARAM, 0.0, 2.0, 0.0, "Ch 1 range");
		configParam(SHAPE_A_PARAM, -1.0, 1.0, 0.0, "Ch 1 shape");
		configParam(TRIGG_A_PARAM, 0.0, 1.0, 0.0, "Ch 1 trigger");
		configParam(RISE_A_PARAM, 0.0, 1.0, 0.0, "Ch 1 rise time");
		configParam(FALL_A_PARAM, 0.0, 1.0, 0.0, "Ch 1 fall time");
		configParam(CYCLE_A_PARAM, 0.0, 1.0, 0.0, "Ch 1 cycle");
		configParam(RANGE_B_PARAM, 0.0, 2.0, 0.0, "Ch 2 range");
		configParam(SHAPE_B_PARAM, -1.0, 1.0, 0.0, "Ch 2 shape");
		configParam(TRIGG_B_PARAM, 0.0, 1.0, 0.0, "Ch 2 trigger");
		configParam(RISE_B_PARAM, 0.0, 1.0, 0.0, "Ch 2 rise time");
		configParam(FALL_B_PARAM, 0.0, 1.0, 0.0, "Ch 2 fall time");
		configParam(CYCLE_B_PARAM, 0.0, 1.0, 0.0, "Ch 2 cycle");
		configParam(BALANCE_PARAM, 0.0, 1.0, 0.5, "Balance");
	}

	void process(const ProcessArgs &args) override {
		for (int c = 0; c < 2; c++) {
			float in = inputs[IN_A_INPUT + c].getVoltage();
			if (trigger[c].process(params[TRIGG_A_PARAM + c].getValue() * 10.0 + inputs[TRIGG_A_INPUT + c].getVoltage() / 2.0)) {
				gate[c] = true;
			}
			if (gate[c]) {
				in = 10.0;
			}

			float shape = params[SHAPE_A_PARAM + c].getValue();
			float delta = in - out[c];

			// Integrator
			float minTime;
			switch ((int) params[RANGE_A_PARAM + c].getValue()) {
				case 0: minTime = 1e-2; break;
				case 1: minTime = 1e-3; break;
				default: minTime = 1e-1; break;
			}

			bool rising = false;
			bool falling = false;

			if (delta > 0) {
				// Rise
				float riseCv = params[RISE_A_PARAM + c].getValue() - inputs[EXP_CV_A_INPUT + c].getVoltage() / 10.0 + inputs[RISE_CV_A_INPUT + c].getVoltage() / 10.0;
				riseCv = clamp(riseCv, 0.0f, 1.0f);
				float rise = minTime * std::pow(2.0, riseCv * 10.0);
				out[c] += shapeDelta(delta, rise, shape) * args.sampleTime;
				rising = (in - out[c] > 1e-3);
				if (!rising) {
					gate[c] = false;
				}
			}
			else if (delta < 0) {
				// Fall
				float fallCv = params[FALL_A_PARAM + c].getValue() - inputs[EXP_CV_A_INPUT + c].getVoltage() / 10.0 + inputs[FALL_CV_A_INPUT + c].getVoltage() / 10.0;
				fallCv = clamp(fallCv, 0.0f, 1.0f);
				float fall = minTime * std::pow(2.0, fallCv * 10.0);
				out[c] += shapeDelta(delta, fall, shape) * args.sampleTime;
				falling = (in - out[c] < -1e-3);
				if (!falling) {
					// End of cycle, check if we should turn the gate back on (cycle mode)
					endOfCyclePulse[c].trigger(1e-3);
					if (params[CYCLE_A_PARAM + c].getValue() * 10.0 + inputs[CYCLE_A_INPUT + c].getVoltage() >= 4.0) {
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

			outputs[RISING_A_OUTPUT + c].setVoltage((rising ? 10.0 : 0.0));
			outputs[FALLING_A_OUTPUT + c].setVoltage((falling ? 10.0 : 0.0));
			lights[RISING_A_LIGHT + c].setSmoothBrightness(rising ? 1.0 : 0.0, args.sampleTime);
			lights[FALLING_A_LIGHT + c].setSmoothBrightness(falling ? 1.0 : 0.0, args.sampleTime);
			outputs[EOC_A_OUTPUT + c].setVoltage((endOfCyclePulse[c].process(args.sampleTime) ? 10.0 : 0.0));
			outputs[OUT_A_OUTPUT + c].setVoltage(out[c]);
			lights[OUT_A_LIGHT + c].setSmoothBrightness(out[c] / 10.0, args.sampleTime);
		}

		// Logic
		float balance = params[BALANCE_PARAM].getValue();
		float a = out[0];
		float b = out[1];
		if (balance < 0.5)
			b *= 2.0 * balance;
		else if (balance > 0.5)
			a *= 2.0 * (1.0 - balance);
		outputs[COMPARATOR_OUTPUT].setVoltage((b > a ? 10.0 : 0.0));
		outputs[MIN_OUTPUT].setVoltage(std::min(a, b));
		outputs[MAX_OUTPUT].setVoltage(std::max(a, b));
		// Lights
		lights[COMPARATOR_LIGHT].setSmoothBrightness(outputs[COMPARATOR_OUTPUT].value / 10.0, args.sampleTime);
		lights[MIN_LIGHT].setSmoothBrightness(outputs[MIN_OUTPUT].value / 10.0, args.sampleTime);
		lights[MAX_LIGHT].setSmoothBrightness(outputs[MAX_OUTPUT].value / 10.0, args.sampleTime);
	}
};




struct RampageWidget : ModuleWidget {
	RampageWidget(Rampage *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Rampage.svg")));

		addChild(createWidget<Knurlie>(Vec(15, 0)));
		addChild(createWidget<Knurlie>(Vec(box.size.x-30, 0)));
		addChild(createWidget<Knurlie>(Vec(15, 365)));
		addChild(createWidget<Knurlie>(Vec(box.size.x-30, 365)));

		addParam(createParam<BefacoSwitch>(Vec(94, 32), module, Rampage::RANGE_A_PARAM));
		addParam(createParam<BefacoTinyKnob>(Vec(27, 90), module, Rampage::SHAPE_A_PARAM));
		addParam(createParam<BefacoPush>(Vec(72, 82), module, Rampage::TRIGG_A_PARAM));
		addParam(createParam<BefacoSlidePot>(Vec(16, 135), module, Rampage::RISE_A_PARAM));
		addParam(createParam<BefacoSlidePot>(Vec(57, 135), module, Rampage::FALL_A_PARAM));
		addParam(createParam<BefacoSwitch>(Vec(101, 238), module, Rampage::CYCLE_A_PARAM));
		addParam(createParam<BefacoSwitch>(Vec(147, 32), module, Rampage::RANGE_B_PARAM));
		addParam(createParam<BefacoTinyKnob>(Vec(217, 90), module, Rampage::SHAPE_B_PARAM));
		addParam(createParam<BefacoPush>(Vec(170, 82), module, Rampage::TRIGG_B_PARAM));
		addParam(createParam<BefacoSlidePot>(Vec(197, 135), module, Rampage::RISE_B_PARAM));
		addParam(createParam<BefacoSlidePot>(Vec(238, 135), module, Rampage::FALL_B_PARAM));
		addParam(createParam<BefacoSwitch>(Vec(141, 238), module, Rampage::CYCLE_B_PARAM));
		addParam(createParam<Davies1900hWhiteKnob>(Vec(117, 76), module, Rampage::BALANCE_PARAM));

		addInput(createInput<BefacoInputPort>(Vec(14, 30), module, Rampage::IN_A_INPUT));
		addInput(createInput<BefacoInputPort>(Vec(52, 37), module, Rampage::TRIGG_A_INPUT));
		addInput(createInput<BefacoInputPort>(Vec(8, 268), module, Rampage::RISE_CV_A_INPUT));
		addInput(createInput<BefacoInputPort>(Vec(67, 268), module, Rampage::FALL_CV_A_INPUT));
		addInput(createInput<BefacoInputPort>(Vec(38, 297), module, Rampage::EXP_CV_A_INPUT));
		addInput(createInput<BefacoInputPort>(Vec(102, 290), module, Rampage::CYCLE_A_INPUT));
		addInput(createInput<BefacoInputPort>(Vec(229, 30), module, Rampage::IN_B_INPUT));
		addInput(createInput<BefacoInputPort>(Vec(192, 37), module, Rampage::TRIGG_B_INPUT));
		addInput(createInput<BefacoInputPort>(Vec(176, 268), module, Rampage::RISE_CV_B_INPUT));
		addInput(createInput<BefacoInputPort>(Vec(237, 268), module, Rampage::FALL_CV_B_INPUT));
		addInput(createInput<BefacoInputPort>(Vec(207, 297), module, Rampage::EXP_CV_B_INPUT));
		addInput(createInput<BefacoInputPort>(Vec(143, 290), module, Rampage::CYCLE_B_INPUT));

		addOutput(createOutput<BefacoOutputPort>(Vec(8, 326), module, Rampage::RISING_A_OUTPUT));
		addOutput(createOutput<BefacoOutputPort>(Vec(68, 326), module, Rampage::FALLING_A_OUTPUT));
		addOutput(createOutput<BefacoOutputPort>(Vec(104, 326), module, Rampage::EOC_A_OUTPUT));
		addOutput(createOutput<BefacoOutputPort>(Vec(102, 195), module, Rampage::OUT_A_OUTPUT));
		addOutput(createOutput<BefacoOutputPort>(Vec(177, 326), module, Rampage::RISING_B_OUTPUT));
		addOutput(createOutput<BefacoOutputPort>(Vec(237, 326), module, Rampage::FALLING_B_OUTPUT));
		addOutput(createOutput<BefacoOutputPort>(Vec(140, 326), module, Rampage::EOC_B_OUTPUT));
		addOutput(createOutput<BefacoOutputPort>(Vec(142, 195), module, Rampage::OUT_B_OUTPUT));
		addOutput(createOutput<BefacoOutputPort>(Vec(122, 133), module, Rampage::COMPARATOR_OUTPUT));
		addOutput(createOutput<BefacoOutputPort>(Vec(89, 157), module, Rampage::MIN_OUTPUT));
		addOutput(createOutput<BefacoOutputPort>(Vec(155, 157), module, Rampage::MAX_OUTPUT));

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
};


Model *modelRampage = createModel<Rampage, RampageWidget>("Rampage");
