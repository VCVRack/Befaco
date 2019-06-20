#include "plugin.hpp"
#include "simd_input.hpp"
#include "PulseGenerator_4.hpp"


static simd::float_4 shapeDelta(simd::float_4 delta, simd::float_4 tau, float shape) {
	simd::float_4 lin = simd::sgn(delta) * 10.f / tau;
	if (shape < 0.f) {
		simd::float_4 log = simd::sgn(delta) * simd::float_4(40.f) / tau / (simd::fabs(delta) + simd::float_4(1.f));
		return simd::crossfade(lin, log, -shape * 0.95f);
	}
	else {
		simd::float_4 exp = M_E * delta / tau;
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


	simd::float_4 out[2][4];
	simd::float_4 gate[2][4]; // use simd __m128 logic instead of bool

	dsp::TSchmittTrigger<simd::float_4> trigger_4[2][4];
	PulseGenerator_4 endOfCyclePulse[2][4];

	// ChannelMask channelMask; 

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
			
			channels_in[part]   = inputs[IN_A_INPUT+part].getChannels();
			channels_trig[part] = inputs[TRIGG_A_INPUT+part].getChannels();
			channels[part] = std::max(channels_in[part], channels_trig[part]);
			channels[part] = std::max(1, channels[part]);

			outputs[OUT_A_OUTPUT+part].setChannels(channels[part]);
			outputs[RISING_A_OUTPUT+part].setChannels(channels[part]);
			outputs[FALLING_A_OUTPUT+part].setChannels(channels[part]);
			outputs[EOC_A_OUTPUT+part].setChannels(channels[part]);
		}
		int channels_max = std::max(channels[0], channels[1]);

		outputs[COMPARATOR_OUTPUT].setChannels(channels_max);
		outputs[MIN_OUTPUT].setChannels(channels_max);
		outputs[MAX_OUTPUT].setChannels(channels_max);

		// loop over two parts of Rampage:

		for (int part = 0; part < 2; part++) {

			simd::float_4 in[4];
			simd::float_4 in_trig[4];
			simd::float_4 expCV[4];
			simd::float_4 riseCV[4];
			simd::float_4 fallCV[4];
			simd::float_4 cycle[4];

			// get parameters:

			float shape = params[SHAPE_A_PARAM + part].getValue();
			float minTime;
			switch ((int) params[RANGE_A_PARAM + part].getValue()) {
				case 0: minTime = 1e-2; break;
				case 1: minTime = 1e-3; break;
				default: minTime = 1e-1; break;
			}

			simd::float_4 param_rise  = simd::float_4(params[RISE_A_PARAM  + part].getValue() * 10.0f);
			simd::float_4 param_fall  = simd::float_4(params[FALL_A_PARAM  + part].getValue() * 10.0f);
			simd::float_4 param_trig  = simd::float_4(params[TRIGG_A_PARAM + part].getValue() * 20.0f);
			simd::float_4 param_cycle = simd::float_4(params[CYCLE_A_PARAM + part].getValue() * 10.0f);

			for(int c=0; c<channels[part]; c+=4) {
				riseCV[c/4] = param_rise;
				fallCV[c/4] = param_fall;
				cycle[c/4] = param_cycle;
				in_trig[c/4] = param_trig;

			}


			// read inputs:

			if(inputs[IN_A_INPUT + part].isConnected()) {
				load_input(inputs[IN_A_INPUT + part], in, channels_in[part]);
				// channelMask.apply_all(in, channels_in[part]);
			} else {
				memset(in, 0, sizeof(in));
			}

			if(inputs[TRIGG_A_INPUT + part].isConnected()) {
				add_input(inputs[TRIGG_A_INPUT + part], in_trig, channels_trig[part]);
				// channelMask.apply_all(in_trig, channels_trig[part]);
			} 	

			if(inputs[EXP_CV_A_INPUT + part].isConnected()) {
				load_input(inputs[EXP_CV_A_INPUT + part], expCV, channels[part]);
				for(int c=0; c<channels[part]; c+=4) {
					riseCV[c/4] -= expCV[c/4];
					fallCV[c/4] -= expCV[c/4];
				}
			}

			add_input(inputs[RISE_CV_A_INPUT + part], riseCV, channels[part]);
			add_input(inputs[FALL_CV_A_INPUT + part], fallCV, channels[part]);	
			add_input(inputs[CYCLE_A_INPUT+part], cycle, channels[part]);
			// channelMask.apply(cycle, channels[part]); // check whether this is necessary

			// start processing:
			
			for(int c=0; c<channels[part]; c+=4) {

				// process SchmittTriggers

				simd::float_4 trig_mask = trigger_4[part][c/4].process(in_trig[c/4]/2.0);
				gate[part][c/4] = ifelse(trig_mask, simd::float_4::mask(), gate[part][c/4]);
				in[c/4] = ifelse(gate[part][c/4], simd::float_4(10.0f), in[c/4]);

				simd::float_4 delta = in[c/4] - out[part][c/4];

				// rise / fall branching

				simd::float_4 delta_gt_0 = delta > simd::float_4::zero();
				simd::float_4 delta_lt_0 = delta < simd::float_4::zero();
				simd::float_4 delta_eq_0 = ~(delta_lt_0|delta_gt_0);

				simd::float_4 rateCV = ifelse(delta_gt_0, riseCV[c/4], simd::float_4::zero());
				rateCV = ifelse(delta_lt_0, fallCV[c/4], rateCV);
				rateCV = clamp(rateCV, simd::float_4::zero(), simd::float_4(10.0f));

				simd::float_4 rate = minTime * simd::pow(2.0f, rateCV);

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
				lights[RISING_A_LIGHT + 3*part  ].setSmoothBrightness(outputs[RISING_A_OUTPUT+part].getVoltage()/10.f, args.sampleTime);
				lights[RISING_A_LIGHT + 3*part+1].setBrightness(0.0f);
				lights[RISING_A_LIGHT + 3*part+2].setBrightness(0.0f);
				lights[FALLING_A_LIGHT + 3*part  ].setSmoothBrightness(outputs[FALLING_A_OUTPUT+part].getVoltage()/10.f, args.sampleTime);
				lights[FALLING_A_LIGHT + 3*part+1].setBrightness(0.0f);
				lights[FALLING_A_LIGHT + 3*part+2].setBrightness(0.0f);
				lights[OUT_A_LIGHT + 3*part  ].setSmoothBrightness(out[part][0].s[0] / 10.0, args.sampleTime);
				lights[OUT_A_LIGHT + 3*part+1].setBrightness(0.0f);
				lights[OUT_A_LIGHT + 3*part+2].setBrightness(0.0f);
			} else {
				lights[RISING_A_LIGHT + 3*part  ].setBrightness(0.0f);
				lights[RISING_A_LIGHT + 3*part+1].setBrightness(0.0f);
				lights[RISING_A_LIGHT + 3*part+2].setBrightness(10.0f);
				lights[FALLING_A_LIGHT + 3*part  ].setBrightness(0.0f);
				lights[FALLING_A_LIGHT + 3*part+1].setBrightness(0.0f);
				lights[FALLING_A_LIGHT + 3*part+2].setBrightness(10.0f);
				lights[OUT_A_LIGHT + 3*part  ].setBrightness(0.0f);
				lights[OUT_A_LIGHT + 3*part+1].setBrightness(0.0f);
				lights[OUT_A_LIGHT + 3*part+2].setBrightness(10.0f);
			}

		} // for (int part, ... )


		// Logic
		float balance = params[BALANCE_PARAM].getValue();

		for(int c=0; c<channels_max; c+=4) {

			simd::float_4 a = out[0][c/4];
			simd::float_4 b = out[1][c/4];

			if (balance < 0.5)
				b *= 2.0f * balance;
			else if (balance > 0.5)
				a *= 2.0f * (1.0 - balance);

			simd::float_4 comp    = ifelse( b>a, simd::float_4(10.0f), simd::float_4::zero() );
			simd::float_4 out_min = simd::fmin(a,b);
			simd::float_4 out_max = simd::fmax(a,b);

			comp.store(outputs[COMPARATOR_OUTPUT].getVoltages(c));
			out_min.store(outputs[MIN_OUTPUT].getVoltages(c));
			out_max.store(outputs[MAX_OUTPUT].getVoltages(c));

		}
		// Lights
		if(channels_max==1) {
			lights[COMPARATOR_LIGHT  ].setSmoothBrightness(outputs[COMPARATOR_OUTPUT].getVoltage(), args.sampleTime);
			lights[COMPARATOR_LIGHT+1].setBrightness(0.0f);
			lights[COMPARATOR_LIGHT+2].setBrightness(0.0f);
			lights[MIN_LIGHT  ].setSmoothBrightness(outputs[MIN_OUTPUT].getVoltage(), args.sampleTime);
			lights[MIN_LIGHT+1].setBrightness(0.0f);
			lights[MIN_LIGHT+2].setBrightness(0.0f);
			lights[MAX_LIGHT  ].setSmoothBrightness(outputs[MAX_OUTPUT].getVoltage(), args.sampleTime);
			lights[MAX_LIGHT+1].setBrightness(0.0f);
			lights[MAX_LIGHT+2].setBrightness(0.0f);
		} else {
			lights[COMPARATOR_LIGHT  ].setBrightness(0.0f);
			lights[COMPARATOR_LIGHT+1].setBrightness(0.0f);
			lights[COMPARATOR_LIGHT+2].setBrightness(10.0f);
			lights[MIN_LIGHT  ].setBrightness(0.0f);
			lights[MIN_LIGHT+1].setBrightness(0.0f);
			lights[MIN_LIGHT+2].setBrightness(10.0f);
			lights[MAX_LIGHT  ].setBrightness(0.0f);
			lights[MAX_LIGHT+1].setBrightness(0.0f);
			lights[MAX_LIGHT+2].setBrightness(10.0f);
		}

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


Model *modelRampage = createModel<Rampage, RampageWidget>("Rampage");
