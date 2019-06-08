#include "plugin.hpp"


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

	float out[PORT_MAX_CHANNELS];

	SlewLimiter() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);
		configParam(SHAPE_PARAM, 0.0, 1.0, 0.0, "Shape");
		configParam(RISE_PARAM, 0.0, 1.0, 0.0, "Rise time");
		configParam(FALL_PARAM, 0.0, 1.0, 0.0, "Fall time");

		memset(out, 0, sizeof(out));
	}

	void process(const ProcessArgs &args) override {

		int channels = inputs[IN_INPUT].getChannels();

		float shape = params[SHAPE_PARAM].getValue();

		// minimum and maximum slopes in volts per second
		const float slewMin = 0.1;
		const float slewMax = 10000.f;
		// Amount of extra slew per voltage difference
		const float shapeScale = 1/10.f;

		const float param_rise = params[RISE_PARAM].getValue();
		const float param_fall = params[FALL_PARAM].getValue();


		outputs[OUT_OUTPUT].setChannels(channels);

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
