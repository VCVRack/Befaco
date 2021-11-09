#include "plugin.hpp"

using simd::float_4;

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

	float_4 out[4] = {};

	SlewLimiter() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);
		configParam(SHAPE_PARAM, 0.0, 1.0, 0.0, "Shape");
		configParam(RISE_PARAM, 0.0, 1.0, 0.0, "Rise time");
		configParam(FALL_PARAM, 0.0, 1.0, 0.0, "Fall time");
		configBypass(IN_INPUT, OUT_OUTPUT);

		configInput(RISE_INPUT, "Rise CV");
		configInput(FALL_INPUT, "Fall CV");
	}

	void process(const ProcessArgs& args) override {

		float_4 in[4] = {};
		float_4 riseCV[4] = {};
		float_4 fallCV[4] = {};

		// this is the number of active polyphony engines, defined by the input
		int numPolyphonyEngines = inputs[IN_INPUT].getChannels();

		// minimum and std::maximum slopes in volts per second
		const float slewMin = 0.1;
		const float slewMax = 10000.f;
		// Amount of extra slew per voltage difference
		const float shapeScale = 1 / 10.f;

		const float_4 param_rise = params[RISE_PARAM].getValue() * 10.f;
		const float_4 param_fall = params[FALL_PARAM].getValue() * 10.f;

		outputs[OUT_OUTPUT].setChannels(numPolyphonyEngines);

		for (int c = 0; c < numPolyphonyEngines; c += 4) {
			in[c / 4] = inputs[IN_INPUT].getVoltageSimd<float_4>(c);

			if (inputs[RISE_INPUT].isConnected()) {
				riseCV[c / 4] = inputs[RISE_INPUT].getPolyVoltageSimd<float_4>(c);
			}
			if (inputs[FALL_INPUT].isConnected()) {
				fallCV[c / 4] = inputs[FALL_INPUT].getPolyVoltageSimd<float_4>(c);
			}

			riseCV[c / 4] += param_rise;
			fallCV[c / 4] += param_fall;

			float_4 delta = in[c / 4] - out[c / 4];
			float_4 delta_gt_0 = delta > 0.f;
			float_4 delta_lt_0 = delta < 0.f;

			float_4 rateCV = {};
			rateCV = ifelse(delta_gt_0, riseCV[c / 4], 0.f);
			rateCV = ifelse(delta_lt_0, fallCV[c / 4], rateCV) * 0.1f;

			float_4 pm_one = simd::sgn(delta);
			float_4 slew = slewMax * simd::pow(slewMin / slewMax, rateCV);

			const float shape = params[SHAPE_PARAM].getValue();
			out[c / 4] += slew * simd::crossfade(pm_one, shapeScale * delta, shape) * args.sampleTime;
			out[c / 4] = ifelse(delta_gt_0 & (out[c / 4] > in[c / 4]), in[c / 4], out[c / 4]);
			out[c / 4] = ifelse(delta_lt_0 & (out[c / 4] < in[c / 4]), in[c / 4], out[c / 4]);

			outputs[OUT_OUTPUT].setVoltageSimd(out[c / 4], c);
		}
	}
};


struct SlewLimiterWidget : ModuleWidget {
	SlewLimiterWidget(SlewLimiter* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/SlewLimiter.svg")));

		addChild(createWidget<Knurlie>(Vec(15, 0)));
		addChild(createWidget<Knurlie>(Vec(15, 365)));

		addParam(createParam<Davies1900hWhiteKnob>(Vec(27, 39), module, ::SlewLimiter::SHAPE_PARAM));

		addParam(createParam<BefacoSlidePot>(Vec(15, 102), module, ::SlewLimiter::RISE_PARAM));
		addParam(createParam<BefacoSlidePot>(Vec(60, 102), module, ::SlewLimiter::FALL_PARAM));

		addInput(createInput<BefacoInputPort>(Vec(10, 273), module, ::SlewLimiter::RISE_INPUT));
		addInput(createInput<BefacoInputPort>(Vec(55, 273), module, ::SlewLimiter::FALL_INPUT));

		addInput(createInput<BefacoInputPort>(Vec(10, 323), module, ::SlewLimiter::IN_INPUT));
		addOutput(createOutput<BefacoOutputPort>(Vec(55, 323), module, ::SlewLimiter::OUT_OUTPUT));
	}
};


Model* modelSlewLimiter = createModel<::SlewLimiter, SlewLimiterWidget>("SlewLimiter");
