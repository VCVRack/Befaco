#include "plugin.hpp"
#include "simd_mask.hpp"


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

	simd::float_4 out[4];
	ChannelMask channelMask;


	SlewLimiter() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);
		configParam(SHAPE_PARAM, 0.0, 1.0, 0.0, "Shape");
		configParam(RISE_PARAM, 0.0, 1.0, 0.0, "Rise time");
		configParam(FALL_PARAM, 0.0, 1.0, 0.0, "Fall time");

		memset(out, 0, sizeof(out));
	}

	void process(const ProcessArgs &args) override {

		simd::float_4 in[4];
		simd::float_4 riseCV[4];
		simd::float_4 fallCV[4];

		int channels = inputs[IN_INPUT].getChannels();


		// minimum and maximum slopes in volts per second
		const float slewMin = 0.1;
		const float slewMax = 10000.f;
		// Amount of extra slew per voltage difference
		const float shapeScale = 1/10.f;

		float shape = params[SHAPE_PARAM].getValue();
		const simd::float_4 param_rise = simd::float_4(params[RISE_PARAM].getValue() * 10.f);
		const simd::float_4 param_fall = simd::float_4(params[FALL_PARAM].getValue() * 10.f);


		outputs[OUT_OUTPUT].setChannels(channels);

		load_input(inputs[IN_INPUT], in, channels);       channelMask.apply(in, channels);
		load_input(inputs[RISE_INPUT], riseCV, channels); channelMask.apply(riseCV, channels);
		load_input(inputs[FALL_INPUT], fallCV, channels); channelMask.apply(fallCV, channels);

		for(int c=0; c<channels; c+=4) {
			riseCV[c/4] += param_rise;
			fallCV[c/4] += param_fall;

			simd::float_4 delta = in[c/4] - out[c/4];

			simd::float_4 delta_gt_0 = delta > simd::float_4::zero();
			simd::float_4 delta_lt_0 = delta < simd::float_4::zero();

			simd::float_4 rateCV = ifelse(delta_gt_0, riseCV[c/4], simd::float_4::zero());
			rateCV = ifelse(delta_lt_0, fallCV[c/4], rateCV) * 0.1f;

			simd::float_4 slew = slewMax * simd::pow(slewMin / slewMax, rateCV);

			out[c/4] += slew * crossfade_4(simd::float_4(1.0f), shapeScale*delta, shape) * args.sampleTime;
			out[c/4] = ifelse( delta_gt_0 & (out[c/4]>in[c/4]), in[c/4], out[c/4]);
			out[c/4] = ifelse( delta_lt_0 & (out[c/4]<in[c/4]), in[c/4], out[c/4]);

			out[c/4].store(outputs[OUT_OUTPUT].getVoltages(c));
		}

		/*
		for(int c=0; c<channels; c++) {

			float in = inputs[IN_INPUT].getVoltage(c);

			// Rise
			if (in > out[c]) {
				float rise = inputs[RISE_INPUT].getPolyVoltage(c) / 10.f + param_rise;
				float slew = slewMax * std::pow(slewMin / slewMax, rise);
				out[c] += slew * crossfade(1.f, shapeScale * (in - out[c]), shape) * args.sampleTime;
				if (out[c] > in)
					out[c] = in;
			}
			// Fall
			else if (in < out[c]) {
				float fall = inputs[FALL_INPUT].getPolyVoltage(c) / 10.f + param_fall;
				float slew = slewMax * std::pow(slewMin / slewMax, fall);
				out[c] -= slew * crossfade(1.f, shapeScale * (out[c] - in), shape) * args.sampleTime;
				if (out[c] < in)
					out[c] = in;
			}

		}
		outputs[OUT_OUTPUT].writeVoltages(out);
		*/
	}
};


struct SlewLimiterWidget : ModuleWidget {
	SlewLimiterWidget(::SlewLimiter *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SlewLimiter.svg")));

		addChild(createWidget<Knurlie>(Vec(15, 0)));
		addChild(createWidget<Knurlie>(Vec(15, 365)));

		addParam(createParam<Davies1900hWhiteKnob>(Vec(27, 39), module, ::SlewLimiter::SHAPE_PARAM));

		addParam(createParam<BefacoSlidePot>(Vec(15, 102), module, ::SlewLimiter::RISE_PARAM));
		addParam(createParam<BefacoSlidePot>(Vec(60, 102), module, ::SlewLimiter::FALL_PARAM));

		addInput(createInput<PJ301MPort>(Vec(10, 273), module, ::SlewLimiter::RISE_INPUT));
		addInput(createInput<PJ301MPort>(Vec(55, 273), module, ::SlewLimiter::FALL_INPUT));

		addInput(createInput<PJ301MPort>(Vec(10, 323), module, ::SlewLimiter::IN_INPUT));
		addOutput(createOutput<PJ301MPort>(Vec(55, 323), module, ::SlewLimiter::OUT_OUTPUT));
	}
};


Model *modelSlewLimiter = createModel<::SlewLimiter, SlewLimiterWidget>("SlewLimiter");
