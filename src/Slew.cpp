#include "plugin.hpp"

using simd::float_4;

struct Slew : Module {
	enum ParamId {
		SHAPE_PARAM,
		RANGE_PARAM,
		RISE_PARAM,
		FALL_PARAM,
		CV_MODE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		IN_INPUT,
		CV_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUT_OUTPUT,
		RISING_OUTPUT,
		FALLING_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		ENUMS(FALLING_LIGHT, 3),
		ENUMS(RISING_LIGHT, 3),
		LIGHTS_LEN
	};
	enum CvMode {
		CV_MODE_FALL,
		CV_MODE_RISE_FALL,
		CV_MODE_RISE
	};

	float_4 out[4] = {};
	dsp::ClockDivider lightDivider;


	Slew() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(SHAPE_PARAM, 0.f, 1.f, 0.f, "Shape");
		configSwitch(RANGE_PARAM, 0.f, 2.f, 1.f, "Range", {"Fast", "Medium", "Slow"});
		auto rise = configParam(RISE_PARAM, 0.f, 1.f, 0.f, "Rise");
		rise->description = "Sets the RISE slew time manually, higher is longer slew time.\n"
		                    "Acts as an attenuator of CV in when CV sent to rise.";
		auto fall = configParam(FALL_PARAM, 0.f, 1.f, 0.f, "Fall");
		fall->description = "Sets the FALL slew time manually, higher is longer slew time.\n"
		                    "Acts as an attenuator of CV in when CV sent to fall.";
		configSwitch(CV_MODE_PARAM, 0.f, 2.f, 1.f, "", {"Fall", "Rise/Fall", "Rise"});
		configInput(IN_INPUT, "In");
		auto cvIn = configInput(CV_INPUT, "CV");
		cvIn->description = "CV input for slew time, 0V to 10V, attenuated by relevant sliders.";
		configOutput(OUT_OUTPUT, "Out");
		configOutput(RISING_OUTPUT, "Rising");
		configOutput(FALLING_OUTPUT, "Falling");

		lightDivider.setDivision(32);
	}

	// slew times:
	// range slow: 4ms to 4s
	// range mid: 40ms to (30) 40s
	// range fast: 400ms to 400s
	void process(const ProcessArgs& args) override {

		float_4 in[4] = {};
		float_4 riseCV[4] = {};
		float_4 fallCV[4] = {};
		float_4 delta[4] = {};

		// this is the number of active polyphony engines, defined by the input
		const int numPolyphonyEngines = std::max(1, inputs[IN_INPUT].getChannels());

		// minimum and maximum slopes in volts per second
		const int range = (int) params[RANGE_PARAM].getValue();
		const float slewMin = 10 / (4 * pow(10.f, range));
		const float slewMax = 10 / (0.004 * pow(10.f, range));
		// Amount of extra slew per voltage difference
		const float shapeScale = 1 / 10.f;

		const float_4 param_rise = params[RISE_PARAM].getValue() * 10.f;
		const float_4 param_fall = params[FALL_PARAM].getValue() * 10.f;
		const CvMode cvMode = (CvMode)(params[CV_MODE_PARAM].getValue());

		outputs[OUT_OUTPUT].setChannels(numPolyphonyEngines);

		for (int c = 0; c < numPolyphonyEngines; c += 4) {
			in[c / 4] = inputs[IN_INPUT].getVoltageSimd<float_4>(c);

			if (inputs[CV_INPUT].isConnected() && (cvMode == CV_MODE_RISE_FALL || cvMode == CV_MODE_RISE)) {
				riseCV[c / 4] = simd::clamp(inputs[CV_INPUT].getPolyVoltageSimd<float_4>(c), 0.f, 10.f) * params[RISE_PARAM].getValue();
			}
			else {
				riseCV[c / 4] = param_rise;

			}
			if (inputs[CV_INPUT].isConnected() && (cvMode == CV_MODE_RISE_FALL || cvMode == CV_MODE_FALL)) {
				fallCV[c / 4] = simd::clamp(inputs[CV_INPUT].getPolyVoltageSimd<float_4>(c), 0.f, 10.f) * params[FALL_PARAM].getValue();
			}
			else {
				fallCV[c / 4] = param_fall;
			}

			delta[c / 4] = in[c / 4] - out[c / 4];
			float_4 delta_gt_0 = delta[c / 4] > 0.f;
			float_4 delta_lt_0 = delta[c / 4] < 0.f;

			float_4 rateCV = {};
			rateCV = ifelse(delta_gt_0, riseCV[c / 4], 0.f);
			rateCV = ifelse(delta_lt_0, fallCV[c / 4], rateCV) * 0.1f;

			float_4 pm_one = simd::sgn(delta[c / 4]);
			float_4 slew = slewMax * simd::pow(slewMin / slewMax, rateCV);

			const float shape = params[SHAPE_PARAM].getValue();
			out[c / 4] += slew * simd::crossfade(pm_one, shapeScale * delta[c / 4], shape) * args.sampleTime;
			out[c / 4] = ifelse(delta_gt_0 & (out[c / 4] > in[c / 4]), in[c / 4], out[c / 4]);
			out[c / 4] = ifelse(delta_lt_0 & (out[c / 4] < in[c / 4]), in[c / 4], out[c / 4]);

			outputs[OUT_OUTPUT].setVoltageSimd(out[c / 4], c);
		}

		if (lightDivider.process()) {
			const float deltaTime = args.sampleTime * lightDivider.getDivision();

			if (numPolyphonyEngines == 1) {
				lights[RISING_LIGHT + 0].setSmoothBrightness(delta[0][0] > 0 ? 1.f : 0.f, deltaTime);
				lights[RISING_LIGHT + 1].setBrightness(0.f);
				lights[RISING_LIGHT + 2].setBrightness(0.f);

				lights[FALLING_LIGHT + 0].setSmoothBrightness(delta[0][0] < 0.f ? 1.f : 0.f, deltaTime);
				lights[FALLING_LIGHT + 1].setBrightness(0.f);
				lights[FALLING_LIGHT + 2].setBrightness(0.f);
			}
			else {
				bool anyRising = false, anyFalling = false;
				for (int c = 0; c < numPolyphonyEngines; c++) {
					anyRising |= out[c / 4][c % 4] < in[c / 4][c % 4];
					anyFalling |= out[c / 4][c % 4] > in[c / 4][c % 4];
				}
				lights[RISING_LIGHT + 0].setBrightness(0.f);
				lights[RISING_LIGHT + 1].setBrightness(0.f);
				lights[RISING_LIGHT + 2].setSmoothBrightness(anyRising ? 1.f : 0.f, deltaTime);

				lights[FALLING_LIGHT + 0].setBrightness(0.f);
				lights[FALLING_LIGHT + 1].setBrightness(0.f);
				lights[FALLING_LIGHT + 2].setSmoothBrightness(anyFalling ? 1.f : 0.f, deltaTime);
			}
		}
	}
};


struct SlewWidget : ModuleWidget {
	SlewWidget(Slew* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Slew.svg")));

		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<BefacoTinyKnobDarkGrey>(mm2px(Vec(9.835, 30.246)), module, Slew::SHAPE_PARAM));
		addParam(createParam<CKSSThreeHorizontal>(mm2px(Vec(5.407, 38.103)), module, Slew::RANGE_PARAM));
		addParam(createParam<BefacoSlidePot>(mm2px(Vec(2.381, 48.289)), module, Slew::RISE_PARAM));
		addParam(createParam<BefacoSlidePot>(mm2px(Vec(12.7, 48.289)), module, Slew::FALL_PARAM));
		addParam(createParam<CKSSNarrow3>(mm2px(Vec(13.351, 108.638)), module, Slew::CV_MODE_PARAM));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(4.978, 15.465)), module, Slew::IN_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(4.978, 112.232)), module, Slew::CV_INPUT));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(14.843, 15.487)), module, Slew::OUT_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(4.978, 99.399)), module, Slew::RISING_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(15.07, 99.399)), module, Slew::FALLING_OUTPUT));

		addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(mm2px(Vec(15.12, 90.397)), module, Slew::FALLING_LIGHT));
		addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(mm2px(Vec(4.978, 90.999)), module, Slew::RISING_LIGHT));
	}
};

Model* modelSlew = createModel<Slew, SlewWidget>("Slew");