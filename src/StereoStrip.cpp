#include "plugin.hpp"

using simd::float_4;

// from https://github.com/wiqid/repelzen/blob/master/src/aefilter.hpp (29307df4fd3e713d206f2155dcff0337fc067f1f)
// with permission (GPL-3.0-or-later)
enum AeFilterType {
	AeLOWPASS,
	AeHIGHPASS
};

enum AeEQType {
	AeLOWSHELVE,
	AeHIGHSHELVE,
	AePEAKINGEQ
};

template <typename T>
struct AeFilter {
	T x[2] = {};
	T y[2] = {};

	float a0, a1, a2, b0, b1, b2;

	inline T process(const T& in) noexcept {
		T out = b0 * in + b1 * x[0] + b2 * x[1] - a1 * y[0] - a2 * y[1];

		//shift buffers
		x[1] = x[0];
		x[0] = in;
		y[1] = y[0];
		y[0] = out;

		return out;
	}

	void setCutoff(float f, float q, int type) {
		const float w0 = 2 * M_PI * f / APP->engine->getSampleRate();
		const float alpha = std::sin(w0) / (2.0f * q);
		const float cs0 = std::cos(w0);

		switch (type) {
			case AeLOWPASS:
				a0 = 1 + alpha;
				b0 = (1 - cs0) / 2 / a0;
				b1 = (1 - cs0) / a0;
				b2 = (1 - cs0) / 2 / a0;
				a1 = (-2 * cs0) / a0;
				a2 = (1 - alpha) / a0;
				break;
			case AeHIGHPASS:
				a0 = 1 + alpha;
				b0 = (1 + cs0) / 2 / a0;
				b1 = -(1 + cs0) / a0;
				b2 = (1 + cs0) / 2 / a0;
				a1 = -2 * cs0 / a0;
				a2 = (1 - alpha) / a0;
		}
	}
};

template <typename T>
struct AeFilterStereo : AeFilter<T> {
	T xl[2] = {};
	T xr[2] = {};
	T yl[2] = {};
	T yr[2] = {};

	void process(T* inL, T* inR) {
		T l = AeFilter<T>::b0 * *inL + AeFilter<T>::b1 * xl[0] + AeFilter<T>::b2 * xl[1] - AeFilter<T>::a1 * yl[0] - AeFilter<T>::a2 * yl[1];
		T r = AeFilter<T>::b0 * *inR + AeFilter<T>::b1 * xr[0] + AeFilter<T>::b2 * xr[1] - AeFilter<T>::a1 * yr[0] - AeFilter<T>::a2 * yr[1];

		//shift buffers
		xl[1] = xl[0];
		xl[0] = *inL;
		xr[1] = xr[0];
		xr[0] = *inR;

		yl[1] = yl[0];
		yl[0] = l;
		yr[1] = yr[0];
		yr[0] = r;

		*inL = l;
		*inR = r;
	}
};

template <typename T>
struct AeEqualizer {
	T x[2] = {};
	T y[2] = {};

	float a0, a1, a2, b0, b1, b2;

	T process(T in) {
		T out = b0 * in + b1 * x[0] + b2 * x[1] - a1 * y[0] - a2 * y[1];
		//shift buffers
		x[1] = x[0];
		x[0] = in;
		y[1] = y[0];
		y[0] = out;
		return out;
	}

	void setParams(float f, float q, float gaindb, AeEQType type) {

		const float w0 = 2 * M_PI * f / APP->engine->getSampleRate();
		const float alpha = sin(w0) / (2.0f * q);
		const float cs0 = cos(w0);
		const float A = pow(10, gaindb / 40.0f);

		switch (type) {
			case AeLOWSHELVE:
				a0 = (A + 1.0f) + (A - 1.0f) * cs0 + 2 * sqrt(A) * alpha;
				b0 = A * ((A + 1.0f) - (A - 1.0f) * cs0 + 2 * sqrt(A) * alpha) / a0;
				b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cs0) / a0;
				b2 = A * ((A + 1.0f) - (A - 1.0f) * cs0 - 2.0f * sqrt(A) * alpha) / a0;
				a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cs0) / a0;
				a2 = ((A + 1.0f) + (A - 1.0f) * cs0 - 2.0f * sqrt(A) * alpha) / a0;
				break;
			case AeHIGHSHELVE:
				a0 = (A + 1.0f) - (A - 1.0f) * cs0 + 2 * sqrt(A) * alpha;
				b0 = A * ((A + 1.0f) + (A - 1.0f) * cs0 + 2 * sqrt(A) * alpha) / a0;
				b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cs0) / a0;
				b2 = A * ((A + 1.0f) + (A - 1.0f) * cs0 - 2.0f * sqrt(A) * alpha) / a0;
				a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cs0) / a0;
				a2 = ((A + 1.0f) - (A - 1.0f) * cs0 - 2.0f * sqrt(A) * alpha) / a0;
				break;
			case AePEAKINGEQ:
				a0 = 1.0f + alpha / A;
				b0 = (1.0f + alpha * A) / a0;
				b1 = -2.0f * cs0 / a0;
				b2 = (1.0f - alpha * A) / a0;
				a1 = -2.0f * cs0 / a0;
				a2 = (1.0f - alpha / A) / a0;
		}
	}
};

template <typename T>
struct AeEqualizerStereo : AeEqualizer<T> {
	T xl[2] = {};
	T xr[2] = {};
	T yl[2] = {};
	T yr[2] = {};

	void process(T* inL, T* inR) {
		T l = AeEqualizer<T>::b0 * *inL + AeEqualizer<T>::b1 * xl[0] + AeEqualizer<T>::b2 * xl[1] - AeEqualizer<T>::a1 * yl[0] - AeEqualizer<T>::a2 * yl[1];
		T r = AeEqualizer<T>::b0 * *inR + AeEqualizer<T>::b1 * xr[0] + AeEqualizer<T>::b2 * xr[1] - AeEqualizer<T>::a1 * yr[0] - AeEqualizer<T>::a2 * yr[1];

		// shift buffers
		xl[1] = xl[0];
		xl[0] = *inL;
		xr[1] = xr[0];
		xr[0] = *inR;

		yl[1] = yl[0];
		yl[0] = l;
		yr[1] = yr[0];
		yr[0] = r;

		*inL = l;
		*inR = r;
	}
};

struct StereoStrip : Module {
	enum ParamId {
		LOW_PARAM,
		MID_PARAM,
		HIGH_PARAM,
		PAN_PARAM,
		MUTE_PARAM,
		PAN_CV_PARAM,
		LEVEL_PARAM,
		IN_BOOST_PARAM,
		OUT_CUT_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		LEFT_INPUT,
		LEVEL_INPUT,
		RIGHT_INPUT,
		PAN_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		LEFT_OUTPUT,
		RIGHT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		ENUMS(LEFT_LIGHT, 3),
		ENUMS(RIGHT_LIGHT, 3),
		LIGHTS_LEN
	};
	enum MuteStates {
		MUTE_OFF_MOMENTARY = -1,
		MUTE_ON,
		MUTE_OFF
	};
	enum MixerSides {
		LEFT,
		RIGHT
	};
	enum PanningLaw {
		LINEAR_6dB,
		EQUAL_POWER,
		LINEAR_CLIPPED
	};

	PanningLaw panningLaw = LINEAR_6dB;

	AeEqualizer<float_4> eqLow[4][2];
	AeEqualizer<float_4> eqMid[4][2];
	AeEqualizer<float_4> eqHigh[4][2];

	bool applyHighpass = true;
	AeFilter<float_4> highpass[4][2];
	bool applyHighshelf = true;
	AeEqualizer<float_4> highshelf[4][2];
	bool applySoftClipping = true;

	float lastLowGain = -INFINITY;
	float lastMidGain = -INFINITY;
	float lastHighGain = -INFINITY;

	// for processing mutes
	dsp::SlewLimiter clickFilter;

	dsp::ClockDivider sliderUpdate;

	StereoStrip() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(HIGH_PARAM, -15.0f, 15.0f, 0.0f, "High shelf (2000 Hz) gain", " dB");
		configParam(MID_PARAM, -12.5f, 12.5f, 0.0f, "Mid band (1200 Hz) gain", " dB");
		configParam(LOW_PARAM, -20.0f, 20.0f, 0.0f, "Low shelf (125 Hz) gain", " dB");
		configParam(PAN_PARAM, -1.f, 1.f, 0.0f, "Pan");
		configSwitch(MUTE_PARAM, MUTE_OFF_MOMENTARY, MUTE_OFF, MUTE_OFF, "Mute", {"Off (momentary)", "On", "Off"});
		configParam(PAN_CV_PARAM, 0.f, 1.f, 0.f, "Pan CV");
		configParam(LEVEL_PARAM, -60.0f, 0.0f, -60.0f, "Gain", "dB");
		configSwitch(IN_BOOST_PARAM, 0, 1, 0, "In boost", {"0dB", "+6dB"});
		configSwitch(OUT_CUT_PARAM, 0, 1, 0, "Out cut", {"0dB", "-6dB"});

		configInput(LEFT_INPUT, "Left");
		configInput(LEVEL_INPUT, "Level (10 V normalled)");
		configInput(RIGHT_INPUT, "Right (left normalled)");
		configInput(PAN_INPUT, "Pan CV (-5 V to +5 V)");

		configOutput(LEFT_OUTPUT, "Left");
		configOutput(RIGHT_OUTPUT, "Right");

		configLight(LEFT_LIGHT, "Left");
		configLight(RIGHT_LIGHT, "Right");

		configBypass(LEFT_INPUT, LEFT_OUTPUT);
		configBypass(RIGHT_INPUT, RIGHT_OUTPUT);

		onSampleRateChange();

		clickFilter.rise = 50.f; // Hz
		clickFilter.fall = 50.f; // Hz

		// only poll EQ sliders every 16 samples
		sliderUpdate.setDivision(16);
	}

	void onSampleRateChange() override {
		bool forceUpdate = true;
		updateEQsIfChanged(forceUpdate);

		for (int side = 0; side < 2; ++side) {
			for (int c = 0; c < 16; c += 4) {
				highpass[side][c / 4].setCutoff(25.0f, 0.8f, AeFilterType::AeHIGHPASS);
				highshelf[side][c / 4].setParams(12000.0f, 0.8f, -5.0f, AeEQType::AeHIGHSHELVE);
			}
		}
	}

	void updateEQsIfChanged(bool forceUpdate = false) {
		float highGain = params[HIGH_PARAM].getValue();
		float midGain = params[MID_PARAM].getValue();
		float lowGain = params[LOW_PARAM].getValue();

		// only calculate coefficients when neccessary
		if (highGain != lastHighGain || forceUpdate) {
			for (int c = 0; c < 16; c += 4) {
				for (int side = 0; side < 2; ++side) {
					eqHigh[c / 4][side].setParams(2000.0f, 0.4f, highGain, AeEQType::AeHIGHSHELVE);
				}
			}
			lastHighGain = highGain;
		}

		if (midGain != lastMidGain || forceUpdate) {
			for (int c = 0; c < 16; c += 4) {
				for (int side = 0; side < 2; ++side) {
					eqMid[c / 4][side].setParams(1200.0f, 0.52f, midGain, AeEQType::AePEAKINGEQ);
				}
			}
			lastMidGain = midGain;
		}

		if (lowGain != lastLowGain || forceUpdate) {
			for (int c = 0; c < 16; c += 4) {
				for (int side = 0; side < 2; ++side) {
					eqLow[c / 4][side].setParams(125.0f, 0.45f, lowGain, AeEQType::AeLOWSHELVE);
				}
			}
			lastLowGain = lowGain;
		}
	}

	void process(const ProcessArgs& args) override {

		float_4 out[4][2] = {}, in[4][2] = {};

		const int numPolyphonyEngines = std::max(inputs[LEFT_INPUT].getChannels(), inputs[RIGHT_INPUT].getChannels());

		// slew mute to avoid clicks
		const float muteGain = clickFilter.process(args.sampleTime, params[MUTE_PARAM].getValue() != MUTE_ON);

		if (inputs[LEFT_INPUT].isConnected() || inputs[RIGHT_INPUT].isConnected()) {

			const float switchGains = (params[IN_BOOST_PARAM].getValue() ? 2.0f : 1.0f) * (params[OUT_CUT_PARAM].getValue() ? 0.5f : 1.0f);
			const float preVCAGain = switchGains * muteGain * std::pow(10, params[LEVEL_PARAM].getValue() / 20.0f);

			if (sliderUpdate.process()) {
				updateEQsIfChanged();
			}

			for (int c = 0; c < numPolyphonyEngines; c += 4) {

				const float_4 postVCAGain = preVCAGain * clamp(inputs[LEVEL_INPUT].getNormalPolyVoltageSimd<float_4>(10.f, c) / 10.f, 0.f, 1.f);

				const float_4 panCV = clamp(params[PAN_CV_PARAM].getValue() * inputs[PAN_INPUT].getPolyVoltageSimd<float_4>(c) / 5.f, -1.f, +1.f);
				const float_4 pan = clamp(params[PAN_PARAM].getValue() + panCV, -1.f, +1.f);

				// https://www.desmos.com/calculator/b0lisclikw
				float_4 gainForSide[2] = {};
				switch (panningLaw) {
					case LINEAR_6dB: {
						gainForSide[0] = postVCAGain * (1.f - pan);
						gainForSide[1] = postVCAGain * (1.f + pan);
						break;
					}
					case EQUAL_POWER: {
						gainForSide[0] = postVCAGain * simd::sqrt(1.f - pan);
						gainForSide[1] = postVCAGain * simd::sqrt(1.f + pan);
						break;
					}
					case LINEAR_CLIPPED: {
						gainForSide[0] = simd::ifelse(pan < 0, postVCAGain, postVCAGain * (1.f - pan));
						gainForSide[1] = simd::ifelse(pan > 0, postVCAGain, postVCAGain * (1.f + pan));
						break;
					}
				}

				in[c / 4][LEFT] = inputs[LEFT_INPUT].getPolyVoltageSimd<float_4>(c);
				in[c / 4][RIGHT] = inputs[RIGHT_INPUT].getNormalPolyVoltageSimd<float_4>(in[c / 4][LEFT], c);

				for (int side = 0; side < 2; ++side) {

					float_4 outForSide = in[c / 4][side];

					outForSide = eqLow[c / 4][side].process(outForSide);
					outForSide = eqMid[c / 4][side].process(outForSide);
					outForSide = eqHigh[c / 4][side].process(outForSide);
					outForSide = applyHighpass ? highpass[c / 4][side].process(outForSide) : outForSide;
					outForSide = applyHighshelf ? highshelf[c / 4][side].process(outForSide) : outForSide;
					outForSide = outForSide * gainForSide[side];

					// soft clipping: the Saturator used elsewhere expects values in range [-1, +1] roughly, so rescale before
					// and after (assuming input signals are 10Vpp, clipping will kick in above 12Vpp with the present values)
					if (applySoftClipping) {
						outForSide = Saturator<float_4>::process(outForSide / 6.f) * 6.f;
					}

					out[c / 4][side] = outForSide;
				}
			}
		}

		if (numPolyphonyEngines <= 1) {
			lights[LEFT_LIGHT + 0].setBrightness(0.f);
			lights[RIGHT_LIGHT + 0].setBrightness(0.f);
			lights[LEFT_LIGHT + 1].setBrightnessSmooth(std::abs(out[0][LEFT][0]), args.sampleTime);
			lights[RIGHT_LIGHT + 1].setBrightnessSmooth(std::abs(out[0][RIGHT][0]), args.sampleTime);
			lights[LEFT_LIGHT + 2].setBrightness(0.f);
			lights[RIGHT_LIGHT + 2].setBrightness(0.f);
		}
		else {
			lights[LEFT_LIGHT + 0].setBrightness(0.f);
			lights[RIGHT_LIGHT + 0].setBrightness(0.f);
			lights[LEFT_LIGHT + 1].setBrightness(0.f);
			lights[RIGHT_LIGHT + 1].setBrightness(0.f);
			lights[LEFT_LIGHT + 2].setBrightness(1.f);
			lights[RIGHT_LIGHT + 2].setBrightness(1.f);
		}

		for (int c = 0; c < numPolyphonyEngines; c += 4) {
			outputs[LEFT_OUTPUT].setVoltageSimd(out[c / 4][LEFT], c);
			outputs[RIGHT_OUTPUT].setVoltageSimd(out[c / 4][RIGHT], c);
		}

		outputs[LEFT_OUTPUT].setChannels(numPolyphonyEngines);
		outputs[RIGHT_OUTPUT].setChannels(numPolyphonyEngines);

	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "applyHighpass", json_boolean(applyHighpass));
		json_object_set_new(rootJ, "applyHighshelf", json_boolean(applyHighshelf));
		json_object_set_new(rootJ, "panningLaw", json_integer(panningLaw));
		json_object_set_new(rootJ, "applySoftClipping", json_boolean(applySoftClipping));

		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* applyHighshelfJ = json_object_get(rootJ, "applyHighshelf");
		if (applyHighshelfJ) {
			applyHighshelf = json_boolean_value(applyHighshelfJ);
		}

		json_t* applyHighpassJ = json_object_get(rootJ, "applyHighpass");
		if (applyHighpassJ) {
			applyHighpass = json_boolean_value(applyHighpassJ);
		}

		json_t* panningLawJ = json_object_get(rootJ, "panningLaw");
		if (panningLawJ) {
			panningLaw = (PanningLaw) json_integer_value(panningLawJ);
		}

		json_t* softClippingJ = json_object_get(rootJ, "applySoftClipping");
		if (softClippingJ) {
			applySoftClipping = json_boolean_value(softClippingJ);
		}
	}
};

// an implementation of a performable, 3-stage switch, where the bottom state is Momentary
struct ThreeStateBefacoSwitchMomentary : SvgSwitch {
	ThreeStateBefacoSwitchMomentary() {
		momentary = true;
		addFrame(Svg::load(asset::system("res/ComponentLibrary/BefacoSwitch_0.svg")));
		addFrame(Svg::load(asset::system("res/ComponentLibrary/BefacoSwitch_1.svg")));
		addFrame(Svg::load(asset::system("res/ComponentLibrary/BefacoSwitch_2.svg")));
	}

	void onDragStart(const event::DragStart& e) override {
		if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
			latched = false;
			pos = Vec(0, 0);
		}
		ParamWidget::onDragStart(e);
	}

	void onDragMove(const event::DragMove& e) override {
		if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
			pos += e.mouseDelta;

			// Once the user has dragged the mouse a "threshold" distance, latch
			// to disallow further changes of state until the mouse is released.
			// We don't just setValue(1) (default/rest state) because this creates a
			// jarring UI experience
			if (pos.y < -10 && !latched) {
				getParamQuantity()->setValue(StereoStrip::MUTE_OFF);
				latched = true;
			}
			if (pos.y > 10 && !latched) {
				getParamQuantity()->setValue(StereoStrip::MUTE_OFF_MOMENTARY);
				latched = true;
			}
		}
		ParamWidget::onDragMove(e);
	}

	void onDragEnd(const event::DragEnd& e) override {
		if (e.button == GLFW_MOUSE_BUTTON_LEFT) {

			// not dragged == clicked
			if (std::sqrt(pos.square()) < 5) {
				// if muted, unmute
				if (getParamQuantity()->getValue() == StereoStrip::MUTE_ON) {
					getParamQuantity()->setValue(StereoStrip::MUTE_OFF);
				}
				// if ummuted, mute
				else if (getParamQuantity()->getValue() == StereoStrip::MUTE_OFF) {
					getParamQuantity()->setValue(StereoStrip::MUTE_ON);
				}
			}

			// on release, the switch resets to default/neutral/middle position, if was previously down
			if (getParamQuantity()->getValue() == StereoStrip::MUTE_OFF_MOMENTARY) {
				getParamQuantity()->setValue(StereoStrip::MUTE_ON);
			}
			latched = false;
		}
		ParamWidget::onDragEnd(e);
	}

	Vec pos;

	bool latched = false;
};

struct StereoStripWidget : ModuleWidget {
	StereoStripWidget(StereoStrip* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/StereoStrip.svg")));

		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParam<BefacoSlidePotSmall>(mm2px(Vec(2.763, 35.805)), module, StereoStrip::LOW_PARAM));
		addParam(createParam<BefacoSlidePotSmall>(mm2px(Vec(12.817, 35.805)), module, StereoStrip::MID_PARAM));
		addParam(createParam<BefacoSlidePotSmall>(mm2px(Vec(22.861, 35.805)), module, StereoStrip::HIGH_PARAM));
		addParam(createParamCentered<Davies1900hDarkGreyKnob>(mm2px(Vec(15.042, 74.11)), module, StereoStrip::PAN_PARAM));
		addParam(createParamCentered<ThreeStateBefacoSwitchMomentary>(mm2px(Vec(7.416, 91.244)), module, StereoStrip::MUTE_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(22.842, 91.244)), module, StereoStrip::PAN_CV_PARAM));
		addParam(createParamCentered<Davies1900hLargeGreyKnob>(mm2px(Vec(15.054, 111.333)), module, StereoStrip::LEVEL_PARAM));
		addParam(createParam<CKSSNarrow>(mm2px(Vec(2.372, 72.298)), module, StereoStrip::IN_BOOST_PARAM));
		addParam(createParam<CKSSNarrow>(mm2px(Vec(24.253, 72.298)), module, StereoStrip::OUT_CUT_PARAM));


		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(5.038, 14.852)), module, StereoStrip::LEFT_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(15.023, 14.852)), module, StereoStrip::LEVEL_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(5.038, 26.304)), module, StereoStrip::RIGHT_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(15.023, 26.304)), module, StereoStrip::PAN_INPUT));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(25.069, 14.882)), module, StereoStrip::LEFT_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(25.069, 26.317)), module, StereoStrip::RIGHT_OUTPUT));

		addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(mm2px(Vec(4.05, 69.906)), module, StereoStrip::LEFT_LIGHT));
		addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(mm2px(Vec(26.05, 69.906)), module, StereoStrip::RIGHT_LIGHT));
	}

	void appendContextMenu(Menu* menu) override {
		StereoStrip* module = dynamic_cast<StereoStrip*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator());
		menu->addChild(createBoolPtrMenuItem("Apply Highpass (25Hz)", "", &module->applyHighpass));
		menu->addChild(createBoolPtrMenuItem("Apply Highshelf (12kHz)", "", &module->applyHighshelf));
		menu->addChild(createBoolPtrMenuItem("Apply soft-clipping", "", &module->applySoftClipping));
		menu->addChild(new MenuSeparator());
		menu->addChild(createIndexPtrSubmenuItem("Panning law", {"Linear (+6dB)", "Equal power (+3dB)", "Linear clipped"}, &module->panningLaw));
	}
};


Model* modelChannelStrip = createModel<StereoStrip, StereoStripWidget>("StereoStrip");