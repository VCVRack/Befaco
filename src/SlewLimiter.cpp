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

	float out = 0.0;

	SlewLimiter() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);
		params[SHAPE_PARAM].config(0.0, 1.0, 0.0, "Shape");
		params[RISE_PARAM].config(0.0, 1.0, 0.0, "Rise time");
		params[FALL_PARAM].config(0.0, 1.0, 0.0, "Fall time");
	}

	void step() override {
		float in = inputs[IN_INPUT].value;
		float shape = params[SHAPE_PARAM].value;

		// minimum and maximum slopes in volts per second
		const float slewMin = 0.1;
		const float slewMax = 10000.f;
		// Amount of extra slew per voltage difference
		const float shapeScale = 1/10.f;

		// Rise
		if (in > out) {
			float rise = inputs[RISE_INPUT].value / 10.f + params[RISE_PARAM].value;
			float slew = slewMax * powf(slewMin / slewMax, rise);
			out += slew * crossfade(1.f, shapeScale * (in - out), shape) * app()->engine->getSampleTime();
			if (out > in)
				out = in;
		}
		// Fall
		else if (in < out) {
			float fall = inputs[FALL_INPUT].value / 10.f + params[FALL_PARAM].value;
			float slew = slewMax * powf(slewMin / slewMax, fall);
			out -= slew * crossfade(1.f, shapeScale * (out - in), shape) * app()->engine->getSampleTime();
			if (out < in)
				out = in;
		}

		outputs[OUT_OUTPUT].value = out;
	}
};


struct SlewLimiterWidget : ModuleWidget {
	SlewLimiterWidget(::SlewLimiter *module) {
		setModule(module);
		setPanel(SVG::load(asset::plugin(plugin, "res/SlewLimiter.svg")));

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
