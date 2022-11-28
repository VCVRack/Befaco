#include "plugin.hpp"


struct MotionMTR : Module {
	enum ParamId {
		MODE1_PARAM,
		CTRL_1_PARAM,
		MODE2_PARAM,
		CTRL_2_PARAM,
		MODE3_PARAM,
		CTRL_3_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		IN1_INPUT,
		IN2_INPUT,
		IN3_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUT1_OUTPUT,
		OUT2_OUTPUT,
		OUT3_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightDisplayType {
		CV_INV,
		CV_ATT,
		AUDIO
	};

	const std::vector<std::string> modeLabels = {"CV Inversion", "CV Attentuation", "Audio"};
	static const int NUM_LIGHTS_PER_DIAL = 20;

	enum LightId {
		ENUMS(LIGHT_1, NUM_LIGHTS_PER_DIAL * 3),
		ENUMS(LIGHT_2, NUM_LIGHTS_PER_DIAL * 3),
		ENUMS(LIGHT_3, NUM_LIGHTS_PER_DIAL * 3),
		LIGHTS_LEN
	};

	struct Map {
		float dbValue;
		long ledNumber;
	};

	const Map lut[20] = {
		{ -30.5, 1},  { -30, 2},  { -30, 3},  { -26, 4},  { -25, 5}, { -24, 6}, { -23, 7 }, { -22, 8 }, { -21, 9}, { -20, 10},
		{ -19, 11}, { -18, 12}, { -16, 13}, { -14, 14},  { -12, 15},  { -10, 16}, { -8, 17}, { -6, 18}, { -4, 19}, { -2, 20}
	};

	dsp::VuMeter2 vuBar[3];

	const int updateLEDRate = 16;
	dsp::ClockDivider sliderUpdate;

	MotionMTR() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(MODE1_PARAM, 0.f, 2.f, 1.f, "Channel 1 mode", modeLabels);
		configParam(CTRL_1_PARAM, 0.f, 1.f, 0.f, "Channel 1 gain");
		configSwitch(MODE2_PARAM, 0.f, 2.f, 1.f, "Channel 2 mode", modeLabels);
		configParam(CTRL_2_PARAM, 0.f, 1.f, 0.f, "Channel 2 gain");
		configSwitch(MODE3_PARAM, 0.f, 2.f, 1.f, "Channel 3 mode", modeLabels);
		configParam(CTRL_3_PARAM, 0.f, 1.f, 0.f, "Channel 3 gain");
		auto in1 = configInput(IN1_INPUT, "Channel 1");
		in1->description = "Normalled to 10V (except in audio mode)";
		auto in2 = configInput(IN2_INPUT, "Channel 2");
		in2->description = "Normalled to 10V (except in audio mode)";
		auto in3 = configInput(IN3_INPUT, "Channel 3");
		in3->description = "Normalled to 10V (except in audio mode)";

		configOutput(OUT1_OUTPUT, "Channel 1");
		configOutput(OUT2_OUTPUT, "Channel 2");
		configOutput(OUT3_OUTPUT, "Channel 3 (Mix)");

		for (int i = 1; i < NUM_LIGHTS_PER_DIAL; ++i) {
			configLight(LIGHT_1 + i * 3, string::f("%g to %g dB", lut[i - 1].dbValue, lut[i].dbValue));
		}

		for (int i = 0; i < 3; ++i) {
			vuBar[i].mode = dsp::VuMeter2::PEAK;
			vuBar[i].lambda = 10.f * updateLEDRate;
		}

		// only poll EQ sliders every 16 samples
		sliderUpdate.setDivision(updateLEDRate);
	}

	void process(const ProcessArgs& args) override {

		const LightDisplayType mode1 = (LightDisplayType) params[MODE1_PARAM].getValue();
		const LightDisplayType mode2 = (LightDisplayType) params[MODE2_PARAM].getValue();
		const LightDisplayType mode3 = (LightDisplayType) params[MODE3_PARAM].getValue();

		const float in1 = inputs[IN1_INPUT].getNormalVoltage(10.f * (mode1 != AUDIO));
		const float in2 = inputs[IN2_INPUT].getNormalVoltage(10.f * (mode2 != AUDIO));
		const float in3 = inputs[IN3_INPUT].getNormalVoltage(10.f * (mode3 != AUDIO));

		const float out1 = in1 * params[CTRL_1_PARAM].getValue() * (mode1 == CV_INV ? -1 : +1);
		const float out2 = in2 * params[CTRL_2_PARAM].getValue() * (mode2 == CV_INV ? -1 : +1);
		float out3 = in3 * params[CTRL_3_PARAM].getValue() * (mode3 == CV_INV ? -1 : +1);

		if (!outputs[OUT1_OUTPUT].isConnected()) {
			out3 += out1;
		}
		if (!outputs[OUT2_OUTPUT].isConnected()) {
			out3 += out2;
		}

		if (sliderUpdate.process()) {
			lightsForSignal(mode1, LIGHT_1, out1, args, 0);
			lightsForSignal(mode2, LIGHT_2, out2, args, 1);
			lightsForSignal(mode3, LIGHT_3, out3, args, 2);
		}

		outputs[OUT1_OUTPUT].setVoltage(out1);
		outputs[OUT2_OUTPUT].setVoltage(out2);
		outputs[OUT3_OUTPUT].setVoltage(out3);
	}

	void setLightRGB(int lightId, float R, float G, float B) {
		lights[lightId + 0].setBrightness(R);
		lights[lightId + 1].setBrightness(G);
		lights[lightId + 2].setBrightness(B);
	}

	void setLightRGBSmooth(int lightId, const ProcessArgs& args, float R, float G, float B) {
		// inverse time constant for LED smoothing
		const float lambda = 10.f * updateLEDRate;
		lights[lightId + 0].setBrightnessSmooth(R, args.sampleTime, lambda);
		lights[lightId + 1].setBrightnessSmooth(G, args.sampleTime, lambda);
		lights[lightId + 2].setBrightnessSmooth(B, args.sampleTime, lambda);
	}

	void lightsForSignal(LightDisplayType type, const LightId lightId, float signal, const ProcessArgs& args, const int channel) {

		if (type == AUDIO) {
			setLightRGB(lightId, 0.f, 1.0f, 0.f);

			vuBar[channel].process(args.sampleTime * updateLEDRate, signal / 10.f);

			for (int i = 1; i < NUM_LIGHTS_PER_DIAL; i++) {
				const float value = vuBar[channel].getBrightness(lut[i - 1].dbValue, lut[i].dbValue);
				if (i < 15) {
					// green
					setLightRGB(lightId + 3 * i, 0.f, value, 0.f);
				}
				else if (i < NUM_LIGHTS_PER_DIAL - 1) {
					// yellow
					setLightRGB(lightId + 3 * i, value, 0.65 * value, 0.f);
				}
				else {
					// red
					setLightRGB(lightId + 3 * i, value, 0.f, 0.f);
				}
			}
		}
		else {
			setLightRGBSmooth(lightId, args, 0.82f, 0.0f, 0.82f);

			if (signal >= 0) {
				for (int i = 1; i < NUM_LIGHTS_PER_DIAL; ++i) {
					float value = 0.5f * (signal > (10 * (i + 1.) / (NUM_LIGHTS_PER_DIAL + 1)));
					// purple
					setLightRGBSmooth(lightId + 3 * i, args, 0.82f * value, 0.0f, 0.82f * value);
				}
			}
			else {
				for (int i = 1; i < NUM_LIGHTS_PER_DIAL; ++i) {
					float value = (signal < (-10 * (NUM_LIGHTS_PER_DIAL - i + 1.) / (NUM_LIGHTS_PER_DIAL + 1.)));
					setLightRGBSmooth(lightId + 3 * i, args, value, 0.65f * value, 0.f);
				}
			}
		}
	}

};


struct MotionMTRWidget : ModuleWidget {
	MotionMTRWidget(MotionMTR* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/MotionMTR.svg")));

		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParam<CKSSThree>(mm2px(Vec(1.298, 17.851)), module, MotionMTR::MODE1_PARAM));
		addParam(createParamCentered<Davies1900hBlackKnob>(mm2px(Vec(18.217, 22.18)), module, MotionMTR::CTRL_1_PARAM));
		addParam(createParam<CKSSThree>(mm2px(Vec(23.762, 46.679)), module, MotionMTR::MODE2_PARAM));
		addParam(createParamCentered<Davies1900hBlackKnob>(mm2px(Vec(11.777, 50.761)), module, MotionMTR::CTRL_2_PARAM));
		addParam(createParam<CKSSThree>(mm2px(Vec(1.34, 74.461)), module, MotionMTR::MODE3_PARAM));
		addParam(createParamCentered<Davies1900hBlackKnob>(mm2px(Vec(18.31, 78.89)), module, MotionMTR::CTRL_3_PARAM));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(5.008, 100.315)), module, MotionMTR::IN1_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(14.993, 100.315)), module, MotionMTR::IN2_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(24.978, 100.315)), module, MotionMTR::IN3_INPUT));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(5.0, 113.207)), module, MotionMTR::OUT1_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(14.978, 113.185)), module, MotionMTR::OUT2_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(25.014, 113.207)), module, MotionMTR::OUT3_OUTPUT));


		struct LightRingDetails {
			MotionMTR::LightId startingId;
			float x, y; 	// centre
		} ;

		std::vector<LightRingDetails> ringDetails = {
			{MotionMTR::LIGHT_1, 18.217, 22.18},
			{MotionMTR::LIGHT_2, 11.777, 50.761},
			{MotionMTR::LIGHT_3, 18.217, 78.85}
		};

		const float R = 9.65; 	// mm
		for (auto detailForRing : ringDetails) {
			for (int i = 0; i < MotionMTR::NUM_LIGHTS_PER_DIAL; ++i) {
				float theta = 2 * M_PI * i / MotionMTR::NUM_LIGHTS_PER_DIAL;
				float x = detailForRing.x + sin(theta) * R;
				float y = detailForRing.y - cos(theta) * R;
				addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(mm2px(Vec(x, y)), module, detailForRing.startingId + 3 * i));
			}
		}
	}
};


Model* modelMotionMTR = createModel<MotionMTR, MotionMTRWidget>("MotionMTR");