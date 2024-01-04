#include "plugin.hpp"

using simd::float_4;

static float_4 shapeDelta(float_4 delta, float_4 tau, float shape) {
	float_4 lin = simd::sgn(delta) * 10.f / tau;
	if (shape < 0.f) {
		float_4 log = simd::sgn(delta) * 40.f / tau / (simd::fabs(delta) + 1.f);
		return simd::crossfade(lin, log, -shape * 0.95f);
	}
	else {
		float_4 exp = M_E * delta / tau;
		return simd::crossfade(lin, exp, shape * 0.90f);
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
		ENUMS(COMPARATOR_LIGHT, 3),
		ENUMS(MIN_LIGHT, 3),
		ENUMS(MAX_LIGHT, 3),
		ENUMS(OUT_A_LIGHT, 3),
		ENUMS(OUT_B_LIGHT, 3),
		ENUMS(RISING_A_LIGHT, 3),
		ENUMS(RISING_B_LIGHT, 3),
		ENUMS(FALLING_A_LIGHT, 3),
		ENUMS(FALLING_B_LIGHT, 3),
		NUM_LIGHTS
	};


	float_4 out[2][4] = {};
	float_4 gate[2][4] = {}; // use simd __m128 logic instead of bool

	dsp::TSchmittTrigger<float_4> trigger_4[2][4];
	PulseGenerator_4 endOfCyclePulse[2][4];

	// ChannelMask channelMask;

	Rampage() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configSwitch(RANGE_A_PARAM, 0.0, 2.0, 0.0, "Ch 1 range", {"Medium", "Fast", "Slow"});
		configParam(SHAPE_A_PARAM, -1.0, 1.0, 0.0, "Ch 1 shape");
		configButton(TRIGG_A_PARAM, "Ch 1 trigger");
		configParam(RISE_A_PARAM, 0.0, 1.0, 0.0, "Ch 1 rise time");
		configParam(FALL_A_PARAM, 0.0, 1.0, 0.0, "Ch 1 fall time");
		configSwitch(CYCLE_A_PARAM, 0.0, 1.0, 0.0, "Ch 1 cycle", {"Off", "On"});
		configSwitch(RANGE_B_PARAM, 0.0, 2.0, 0.0, "Ch 2 range", {"Medium", "Fast", "Slow"});
		configParam(SHAPE_B_PARAM, -1.0, 1.0, 0.0, "Ch 2 shape");
		configButton(TRIGG_B_PARAM, "Ch 2 trigger");
		configParam(RISE_B_PARAM, 0.0, 1.0, 0.0, "Ch 2 rise time");
		configParam(FALL_B_PARAM, 0.0, 1.0, 0.0, "Ch 2 fall time");
		configSwitch(CYCLE_B_PARAM, 0.0, 1.0, 0.0, "Ch 2 cycle", {"Off", "On"});
		configParam(BALANCE_PARAM, 0.0, 1.0, 0.5, "Balance");

		configInput(IN_A_INPUT, "A");
		configInput(IN_B_INPUT, "B");
		configInput(TRIGG_A_INPUT, "Trigger A");
		configInput(TRIGG_B_INPUT, "Trigger B");
		configInput(RISE_CV_A_INPUT, "Rise CV A");
		configInput(RISE_CV_B_INPUT, "Rise CV B");
		configInput(FALL_CV_A_INPUT, "Fall CV A");
		configInput(FALL_CV_B_INPUT, "Fall CV B");
		configInput(EXP_CV_A_INPUT, "Exponetial CV A");
		configInput(EXP_CV_B_INPUT, "Exponetial CV B");
		configInput(CYCLE_A_INPUT, "Cycle A");
		configInput(CYCLE_B_INPUT, "Cycle B");

		configOutput(RISING_A_OUTPUT, "Rising A");
		configOutput(RISING_B_OUTPUT, "Rising B");
		configOutput(FALLING_A_OUTPUT, "Falling A");
		configOutput(FALLING_B_OUTPUT, "Falling B");
		configOutput(EOC_A_OUTPUT, "End of cycle A");
		configOutput(EOC_B_OUTPUT, "End of cycle B");
		configOutput(OUT_A_OUTPUT, "A");
		configOutput(OUT_B_OUTPUT, "B");
		configOutput(COMPARATOR_OUTPUT, "B > A");
		configOutput(MIN_OUTPUT, "Minimum of A and B");
		configOutput(MAX_OUTPUT, "Maximum of A and B");
	}

	void process(const ProcessArgs& args) override {
		int channels_in[2] = {};
		int channels_trig[2] = {};
		int channels[2] = {}; 	// the larger of in or trig (per-part)

		// determine number of channels:

		for (int part = 0; part < 2; part++) {

			channels_in[part]   = inputs[IN_A_INPUT + part].getChannels();
			channels_trig[part] = inputs[TRIGG_A_INPUT + part].getChannels();
			channels[part] = std::max(channels_in[part], channels_trig[part]);
			channels[part] = std::max(1, channels[part]);

			outputs[OUT_A_OUTPUT + part].setChannels(channels[part]);
			outputs[RISING_A_OUTPUT + part].setChannels(channels[part]);
			outputs[FALLING_A_OUTPUT + part].setChannels(channels[part]);
			outputs[EOC_A_OUTPUT + part].setChannels(channels[part]);
		}

		// total number of active polyphony engines, accounting for both halves
		// (channels[0] / channels[1] are the number of active engines per section)
		const int channels_max = std::max(channels[0], channels[1]);

		outputs[COMPARATOR_OUTPUT].setChannels(channels_max);
		outputs[MIN_OUTPUT].setChannels(channels_max);
		outputs[MAX_OUTPUT].setChannels(channels_max);

		// loop over two parts of Rampage:
		for (int part = 0; part < 2; part++) {

			float_4 in[4] = {};
			float_4 in_trig[4] = {};
			float_4 riseCV[4] = {};
			float_4 fallCV[4] = {};
			float_4 cycle[4] = {};

			// get parameters:
			float minTime;
			switch ((int) params[RANGE_A_PARAM + part].getValue()) {
				case 0:
					minTime = 1e-2;
					break;
				case 1:
					minTime = 1e-3;
					break;
				default:
					minTime = 1e-1;
					break;
			}

			float_4 param_rise  = params[RISE_A_PARAM  + part].getValue() * 10.0f;
			float_4 param_fall  = params[FALL_A_PARAM  + part].getValue() * 10.0f;
			float_4 param_trig  = params[TRIGG_A_PARAM + part].getValue() * 20.0f;
			float_4 param_cycle = params[CYCLE_A_PARAM + part].getValue() * 10.0f;

			for (int c = 0; c < channels[part]; c += 4) {
				riseCV[c / 4] = param_rise;
				fallCV[c / 4] = param_fall;
				cycle[c / 4] = param_cycle;
				in_trig[c / 4] = param_trig;
			}

			// read inputs:
			if (inputs[IN_A_INPUT + part].isConnected()) {
				for (int c = 0; c < channels[part]; c += 4)
					in[c / 4] = inputs[IN_A_INPUT + part].getPolyVoltageSimd<float_4>(c);
			}

			if (inputs[TRIGG_A_INPUT + part].isConnected()) {
				for (int c = 0; c < channels[part]; c += 4)
					in_trig[c / 4] += inputs[TRIGG_A_INPUT + part].getPolyVoltageSimd<float_4>(c);
			}

			if (inputs[EXP_CV_A_INPUT + part].isConnected()) {
				float_4 expCV[4] = {};
				for (int c = 0; c < channels[part]; c += 4)
					expCV[c / 4] = inputs[EXP_CV_A_INPUT + part].getPolyVoltageSimd<float_4>(c);

				for (int c = 0; c < channels[part]; c += 4) {
					riseCV[c / 4] -= expCV[c / 4];
					fallCV[c / 4] -= expCV[c / 4];
				}
			}

			for (int c = 0; c < channels[part]; c += 4)
				riseCV[c / 4] += inputs[RISE_CV_A_INPUT + part].getPolyVoltageSimd<float_4>(c);
			for (int c = 0; c < channels[part]; c += 4)
				fallCV[c / 4] += inputs[FALL_CV_A_INPUT + part].getPolyVoltageSimd<float_4>(c);
			for (int c = 0; c < channels[part]; c += 4)
				cycle[c / 4] += inputs[CYCLE_A_INPUT + part].getPolyVoltageSimd<float_4>(c);

			// start processing:
			for (int c = 0; c < channels[part]; c += 4) {

				// process SchmittTriggers
				float_4 trig_mask = trigger_4[part][c / 4].process(in_trig[c / 4] / 2.0, 0.1, 2.0);
				gate[part][c / 4] = ifelse(trig_mask, float_4::mask(), gate[part][c / 4]);
				in[c / 4] = ifelse(gate[part][c / 4], 10.0f, in[c / 4]);

				float_4 delta = in[c / 4] - out[part][c / 4];

				// rise / fall branching
				float_4 delta_gt_0 = delta > 0.f;
				float_4 delta_lt_0 = delta < 0.f;
				float_4 delta_eq_0 = ~(delta_lt_0 | delta_gt_0);

				float_4 rateCV = ifelse(delta_gt_0, riseCV[c / 4], 0.f);
				rateCV = ifelse(delta_lt_0, fallCV[c / 4], rateCV);
				rateCV = clamp(rateCV, 0.f, 10.0f);

				float_4 rate = minTime * simd::pow(2.0f, rateCV);

				float shape = params[SHAPE_A_PARAM + part].getValue();
				out[part][c / 4] += shapeDelta(delta, rate, shape) * args.sampleTime;

				float_4 rising  = simd::ifelse(delta_gt_0, (in[c / 4] - out[part][c / 4]) > 1e-3f, float_4::zero());
				float_4 falling = simd::ifelse(delta_lt_0, (in[c / 4] - out[part][c / 4]) < -1e-3f, float_4::zero());

				float_4 end_of_cycle = simd::andnot(falling, delta_lt_0);

				endOfCyclePulse[part][c / 4].trigger(end_of_cycle, 1e-3);

				gate[part][c / 4] = ifelse(simd::andnot(rising, delta_gt_0), 0.f, gate[part][c / 4]);
				gate[part][c / 4] = ifelse(end_of_cycle & (cycle[c / 4] >= 4.0f), float_4::mask(), gate[part][c / 4]);
				gate[part][c / 4] = ifelse(delta_eq_0, 0.f, gate[part][c / 4]);

				out[part][c / 4]  = ifelse(rising | falling, out[part][c / 4], in[c / 4]);

				float_4 out_rising  = ifelse(rising, 10.0f, 0.f);
				float_4 out_falling = ifelse(falling, 10.0f, 0.f);

				float_4 pulse = endOfCyclePulse[part][c / 4].process(args.sampleTime);
				float_4 out_EOC = ifelse(pulse, 10.f, 0.f);

				outputs[OUT_A_OUTPUT + part].setVoltageSimd(out[part][c / 4], c);
				outputs[RISING_A_OUTPUT + part].setVoltageSimd(out_rising, c);
				outputs[FALLING_A_OUTPUT + part].setVoltageSimd(out_falling, c);
				outputs[EOC_A_OUTPUT + part].setVoltageSimd(out_EOC, c);

			} // for(int c, ...)

			if (channels[part] == 1) {
				lights[RISING_A_LIGHT + 3 * part  ].setSmoothBrightness(outputs[RISING_A_OUTPUT + part].getVoltage() / 10.f, args.sampleTime);
				lights[RISING_A_LIGHT + 3 * part + 1].setBrightness(0.0f);
				lights[RISING_A_LIGHT + 3 * part + 2].setBrightness(0.0f);
				lights[FALLING_A_LIGHT + 3 * part  ].setSmoothBrightness(outputs[FALLING_A_OUTPUT + part].getVoltage() / 10.f, args.sampleTime);
				lights[FALLING_A_LIGHT + 3 * part + 1].setBrightness(0.0f);
				lights[FALLING_A_LIGHT + 3 * part + 2].setBrightness(0.0f);
				lights[OUT_A_LIGHT + 3 * part  ].setSmoothBrightness(out[part][0].s[0] / 10.0, args.sampleTime);
				lights[OUT_A_LIGHT + 3 * part + 1].setBrightness(0.0f);
				lights[OUT_A_LIGHT + 3 * part + 2].setBrightness(0.0f);
			}
			else {
				lights[RISING_A_LIGHT + 3 * part  ].setBrightness(0.0f);
				lights[RISING_A_LIGHT + 3 * part + 1].setBrightness(0.0f);
				lights[RISING_A_LIGHT + 3 * part + 2].setBrightness(10.0f);
				lights[FALLING_A_LIGHT + 3 * part  ].setBrightness(0.0f);
				lights[FALLING_A_LIGHT + 3 * part + 1].setBrightness(0.0f);
				lights[FALLING_A_LIGHT + 3 * part + 2].setBrightness(10.0f);
				lights[OUT_A_LIGHT + 3 * part  ].setBrightness(0.0f);
				lights[OUT_A_LIGHT + 3 * part + 1].setBrightness(0.0f);
				lights[OUT_A_LIGHT + 3 * part + 2].setBrightness(10.0f);
			}

		} // for (int part, ... )


		// Logic
		float balance = params[BALANCE_PARAM].getValue();

		for (int c = 0; c < channels_max; c += 4) {

			float_4 a = out[0][c / 4];
			float_4 b = out[1][c / 4];

			if (balance < 0.5)
				b *= 2.0f * balance;
			else if (balance > 0.5)
				a *= 2.0f * (1.0 - balance);

			float_4 comp = ifelse(b > a, 10.0f, 0.f);
			float_4 out_min = simd::fmin(a, b);
			float_4 out_max = simd::fmax(a, b);

			outputs[COMPARATOR_OUTPUT].setVoltageSimd(comp, c);
			outputs[MIN_OUTPUT].setVoltageSimd(out_min, c);
			outputs[MAX_OUTPUT].setVoltageSimd(out_max, c);
		}
		// Lights
		if (channels_max == 1) {
			lights[COMPARATOR_LIGHT  ].setSmoothBrightness(outputs[COMPARATOR_OUTPUT].getVoltage(), args.sampleTime);
			lights[COMPARATOR_LIGHT + 1].setBrightness(0.0f);
			lights[COMPARATOR_LIGHT + 2].setBrightness(0.0f);
			lights[MIN_LIGHT  ].setSmoothBrightness(outputs[MIN_OUTPUT].getVoltage(), args.sampleTime);
			lights[MIN_LIGHT + 1].setBrightness(0.0f);
			lights[MIN_LIGHT + 2].setBrightness(0.0f);
			lights[MAX_LIGHT  ].setSmoothBrightness(outputs[MAX_OUTPUT].getVoltage(), args.sampleTime);
			lights[MAX_LIGHT + 1].setBrightness(0.0f);
			lights[MAX_LIGHT + 2].setBrightness(0.0f);
		}
		else {
			lights[COMPARATOR_LIGHT  ].setBrightness(0.0f);
			lights[COMPARATOR_LIGHT + 1].setBrightness(0.0f);
			lights[COMPARATOR_LIGHT + 2].setBrightness(10.0f);
			lights[MIN_LIGHT  ].setBrightness(0.0f);
			lights[MIN_LIGHT + 1].setBrightness(0.0f);
			lights[MIN_LIGHT + 2].setBrightness(10.0f);
			lights[MAX_LIGHT  ].setBrightness(0.0f);
			lights[MAX_LIGHT + 1].setBrightness(0.0f);
			lights[MAX_LIGHT + 2].setBrightness(10.0f);
		}
	}
};


struct RampageWidget : ModuleWidget {
	RampageWidget(Rampage* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/Rampage.svg")));

		addChild(createWidget<Knurlie>(Vec(15, 0)));
		addChild(createWidget<Knurlie>(Vec(box.size.x - 30, 0)));
		addChild(createWidget<Knurlie>(Vec(15, 365)));
		addChild(createWidget<Knurlie>(Vec(box.size.x - 30, 365)));

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

		addChild(createLight<SmallLight<RedGreenBlueLight>>(Vec(132, 167), module, Rampage::COMPARATOR_LIGHT));
		addChild(createLight<SmallLight<RedGreenBlueLight>>(Vec(123, 174), module, Rampage::MIN_LIGHT));
		addChild(createLight<SmallLight<RedGreenBlueLight>>(Vec(141, 174), module, Rampage::MAX_LIGHT));
		addChild(createLight<SmallLight<RedGreenBlueLight>>(Vec(126, 185), module, Rampage::OUT_A_LIGHT));
		addChild(createLight<SmallLight<RedGreenBlueLight>>(Vec(138, 185), module, Rampage::OUT_B_LIGHT));
		addChild(createLight<SmallLight<RedGreenBlueLight>>(Vec(18, 312), module, Rampage::RISING_A_LIGHT));
		addChild(createLight<SmallLight<RedGreenBlueLight>>(Vec(78, 312), module, Rampage::FALLING_A_LIGHT));
		addChild(createLight<SmallLight<RedGreenBlueLight>>(Vec(187, 312), module, Rampage::RISING_B_LIGHT));
		addChild(createLight<SmallLight<RedGreenBlueLight>>(Vec(247, 312), module, Rampage::FALLING_B_LIGHT));
	}
};


Model* modelRampage = createModel<Rampage, RampageWidget>("Rampage");
