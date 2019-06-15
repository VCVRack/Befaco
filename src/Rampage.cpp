#include "plugin.hpp"
#include "simd_mask.hpp"
#include "PulseGenerator_4.hpp"

#define MAX(a,b) a>b?a:b

/*
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
*/


inline simd::float_4 crossfade_4(simd::float_4 a, simd::float_4 b, simd::float_4 p) {
        return a + (b - a) * p;
}

static simd::float_4 shapeDelta(simd::float_4 delta, simd::float_4 tau, float shape) {
	simd::float_4 lin = simd::sgn(delta) * 10.f / tau;
	if (shape < 0.f) {
		simd::float_4 log = simd::sgn(delta) * simd::float_4(40.f) / tau / (simd::fabs(delta) + simd::float_4(1.f));
		return crossfade_4(lin, log, -shape * 0.95f);
	}
	else {
		simd::float_4 exp = M_E * delta / tau;
		return crossfade_4(lin, exp, shape * 0.90f);
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

	/*
	float out[2] = {};
	bool gate[2] = {};
	*/

	simd::float_4 out[2][4];
	simd::float_4 gate[2][4]; // use simd __m128 logic instead of bool

	dsp::TSchmittTrigger<simd::float_4> trigger_4[2][4];
	PulseGenerator_4 endOfCyclePulse[2][4];

	ChannelMask channelMask; 

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

		memset(out, 0, sizeof(out));
		memset(gate, 0, sizeof(gate));
	}

	void process(const ProcessArgs &args) override {

		int channels_in[2];
		int channels_trig[2];
		int channels[2];

		// determine number of channels: 

		for (int part=0; part<2; part++) {
			int tmp = inputs[IN_A_INPUT+part].getChannels();
			channels_in[part] = MAX(1,tmp);
			tmp = inputs[TRIGG_A_INPUT+part].getChannels();
			channels_trig[part] = MAX(1,tmp);
			channels[part] = MAX(channels_in[part], channels_trig[part]);

			outputs[OUT_A_OUTPUT+part].setChannels(channels[part]);
			outputs[RISING_A_OUTPUT+part].setChannels(channels[part]);
			outputs[FALLING_A_OUTPUT+part].setChannels(channels[part]);
			outputs[EOC_A_OUTPUT+part].setChannels(channels[part]);
		}
		int channels_max = MAX(channels[0], channels[1]);

		// loop over two parts of Rampage:

		for (int part = 0; part < 2; part++) {

			simd::float_4 in[4];
			simd::float_4 in_trig[4];
			simd::float_4 expCV[4] = {};
			simd::float_4 riseCV[4] = {};
			simd::float_4 fallCV[4] = {};
			simd::float_4 cycle[4] = {};

			float shape = params[SHAPE_A_PARAM + part].getValue();
			float minTime;
			switch ((int) params[RANGE_A_PARAM + part].getValue()) {
				case 0: minTime = 1e-2; break;
				case 1: minTime = 1e-3; break;
				default: minTime = 1e-1; break;
			}

			simd::float_4 param_rise  = simd::float_4(params[RISE_A_PARAM  + part].getValue());
			simd::float_4 param_fall  = simd::float_4(params[FALL_A_PARAM  + part].getValue());
			simd::float_4 param_trig  = simd::float_4(params[TRIGG_A_PARAM + part].getValue() * 10.0f);
			simd::float_4 param_cycle = simd::float_4(params[CYCLE_A_PARAM + part].getValue() * 10.0f);

			if(inputs[IN_A_INPUT + part].isConnected()) {
				load_input(inputs[IN_A_INPUT + part], in, channels_in[part]);
				channelMask.apply_all(in, channels_in[part]);
			} else {
				memset(in, 0, sizeof(in));
			}

			if(inputs[TRIGG_A_INPUT + part].isConnected()) {
				load_input(inputs[TRIGG_A_INPUT + part], in_trig, channels_trig[part]);
				channelMask.apply_all(in_trig, channels_trig[part]);
			} else {
				memset(in_trig, 0, sizeof(in_trig));
			}	

			for(int c=0; c<channels_trig[part]; c+=4) in_trig[c/4] = param_trig + in_trig[c/4]/2.0f;
			channelMask.apply_all(in_trig, channels_trig[part] );
			
			for(int c=0; c<channels[part]; c+=4) {
				simd::float_4 trig_mask = trigger_4[part][c/4].process(in_trig[c/4]);
				gate[part][c/4] = ifelse(trig_mask, simd::float_4::mask(), gate[part][c/4]);
				in[c/4] = ifelse(gate[part][c/4], simd::float_4(10.0f), in[c/4]);
			}

			if(inputs[EXP_CV_A_INPUT + part].isConnected()) {
				load_input(inputs[EXP_CV_A_INPUT + part], expCV, channels[part]);
				for(int c=0; c<channels[part]; c+=4) riseCV[c/4] *= -1.0f;
			}

			load_input(inputs[RISE_CV_A_INPUT + part], riseCV, channels[part]);
			for(int c=0; c<channels[part]; c+=4) {
				riseCV[c/4] += expCV[c/4];
				riseCV[c/4] *= 0.10f;
				riseCV[c/4] += param_rise;
			}

			load_input(inputs[FALL_CV_A_INPUT + part], fallCV, channels[part]);
			for(int c=0; c<channels[part]; c+=4) {
				fallCV[c/4] += expCV[c/4];
				fallCV[c/4] *= 0.10f;
				fallCV[c/4] += param_fall;
			}

			load_input(inputs[CYCLE_A_INPUT], cycle, channels[part]);
			// channelMask.apply_all(cycle, channels[part]);
			for(int c=0; c<channels[part]; c+=4) {
				cycle[c/4] += param_cycle;
			}
			channelMask.apply_all(cycle, channels[part]);


			for(int c=0; c<channels[part]; c+=4) {

				simd::float_4 delta = in[c/4] - out[part][c/4];

				simd::float_4 delta_gt_0 = delta > simd::float_4::zero();
				simd::float_4 delta_lt_0 = delta < simd::float_4::zero();
				simd::float_4 delta_eq_0 = ~(delta_lt_0|delta_gt_0);

				simd::float_4 rateCV = ifelse(delta_gt_0, riseCV[c/4], simd::float_4::zero());
				rateCV = ifelse(delta_lt_0, fallCV[c/4], rateCV);
				rateCV = clamp(rateCV, simd::float_4::zero(), simd::float_4(1.0f));

				simd::float_4 rate = minTime * simd::pow(2.0f, rateCV * 10.0f);

				out[part][c/4] += shapeDelta(delta, rate, shape) * args.sampleTime;

				simd::float_4 rising  = (in[c/4] - out[part][c/4]) > simd::float_4( 1e-3);
				simd::float_4 falling = (in[c/4] - out[part][c/4]) < simd::float_4(-1e-3);
				simd::float_4 end_of_cycle = simd::andnot(falling,delta_lt_0);

				endOfCyclePulse[part][c/4].trigger(end_of_cycle, 1e-3);

				gate[part][c/4] = ifelse( simd::andnot(rising, delta_gt_0),                 simd::float_4::zero(), gate[part][c/4]);
				gate[part][c/4] = ifelse( end_of_cycle & (cycle[c/4]>=simd::float_4(4.0f)), simd::float_4::mask(), gate[part][c/4] );
				gate[part][c/4] = ifelse( delta_eq_0,                                       simd::float_4::zero(), gate[part][c/4] );

				out[part][c/4]  = ifelse( rising|falling, out[part][c/4], in[c/4] );

				simd::float_4 out_rising  = ifelse(rising,  simd::float_4(10.0f), simd::float_4::zero() );
				simd::float_4 out_falling = ifelse(falling, simd::float_4(10.0f), simd::float_4::zero() );

				simd::float_4 pulse = endOfCyclePulse[part][c/4].process(args.sampleTime);
				simd::float_4 out_EOC = ifelse(pulse, simd::float_4(10.f), simd::float_4::zero() );

				out[part][c/4].store(outputs[OUT_A_OUTPUT+part].getVoltages(c));

				out_rising.store( outputs[RISING_A_OUTPUT+part].getVoltages(c));
				out_falling.store(outputs[FALLING_A_OUTPUT+part].getVoltages(c));
				out_EOC.store(outputs[EOC_A_OUTPUT+part].getVoltages(c));


			} // for(int c, ...)

			if(channels[part] == 1) {
				lights[RISING_A_LIGHT + part].setSmoothBrightness(outputs[RISING_A_OUTPUT+part].getVoltage()/10.f, args.sampleTime);
				lights[FALLING_A_LIGHT + part].setSmoothBrightness(outputs[FALLING_A_OUTPUT+part].getVoltage()/10.f, args.sampleTime);
				lights[OUT_A_LIGHT + part].setSmoothBrightness(out[part][0].s[0] / 10.0, args.sampleTime);
			} else {
				lights[RISING_A_LIGHT + part].setSmoothBrightness(outputs[RISING_A_OUTPUT+part].getVoltageSum()/10.f, args.sampleTime);
				lights[FALLING_A_LIGHT + part].setSmoothBrightness(outputs[FALLING_A_OUTPUT+part].getVoltageSum()/10.f, args.sampleTime);
				lights[OUT_A_LIGHT + part].setSmoothBrightness(outputs[OUT_A_OUTPUT].getVoltageSum() / 10.0, args.sampleTime);

			}

			// Integrator

			// bool rising = false;
			// bool falling = false;

			/* 
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
			*/

			// outputs[RISING_A_OUTPUT + part].setVoltage((rising ? 10.0 : 0.0));
			// outputs[FALLING_A_OUTPUT + part].setVoltage((falling ? 10.0 : 0.0));
			// lights[RISING_A_LIGHT + part].setSmoothBrightness(rising ? 1.0 : 0.0, args.sampleTime);
			// lights[FALLING_A_LIGHT + part].setSmoothBrightness(falling ? 1.0 : 0.0, args.sampleTime);
			// outputs[EOC_A_OUTPUT + part].setVoltage((endOfCyclePulse[c].process(args.sampleTime) ? 10.0 : 0.0));
			// outputs[OUT_A_OUTPUT + part].setVoltage(out[c]);
			// lights[OUT_A_LIGHT + part].setSmoothBrightness(out[c] / 10.0, args.sampleTime);


		} // for (int part, ... )



		// Logic
		float balance = params[BALANCE_PARAM].getValue();

		for(int c=0; c<channels_max; c+=4) {

			simd::float_4 a = out[0][c/4];
			simd::float_4 b = out[1][c/4];

			if (balance < 0.5)
				b *= 2.0 * balance;
			else if (balance > 0.5)
				a *= 2.0 * (1.0 - balance);

			simd::float_4 comp    = ifelse( b>a, simd::float_4(10.0f), simd::float_4::zero() );
			simd::float_4 out_min = simd::fmin(a,b);
			simd::float_4 out_max = simd::fmax(a,b);

			comp.store(outputs[COMPARATOR_OUTPUT].getVoltages(c));
			out_min.store(outputs[MIN_OUTPUT].getVoltages(c));
			out_max.store(outputs[MAX_OUTPUT].getVoltages(c));

		}
		// Lights
		lights[COMPARATOR_LIGHT].setSmoothBrightness(outputs[COMPARATOR_OUTPUT].getVoltageSum() / 10.0, args.sampleTime);
		lights[MIN_LIGHT].setSmoothBrightness(outputs[MIN_OUTPUT].getVoltageSum() / 10.0, args.sampleTime);
		lights[MAX_LIGHT].setSmoothBrightness(outputs[MAX_OUTPUT].getVoltageSum() / 10.0, args.sampleTime);

	} // end process()
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
};


Model *modelRampage = createModel<Rampage, RampageWidget>("Rampage");
