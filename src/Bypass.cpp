#include "plugin.hpp"

using namespace simd;

struct Bypass : Module {
	enum ParamId {
		MODE_PARAM,
		FX_GAIN_PARAM,
		LAUNCH_MODE_PARAM,
		LAUNCH_BUTTON_PARAM,
		SLEW_TIME_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		IN_R_INPUT,
		FROM_FX_L_INPUT,
		FROM_FX_R_INPUT,
		LAUNCH_INPUT,
		IN_L_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		TO_FX_L_OUTPUT,
		TO_FX_R_OUTPUT,
		OUT_L_OUTPUT,
		OUT_R_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LAUNCH_LED,
		LIGHTS_LEN
	};
	enum LatchMode {
		TOGGLE_MODE, 	// i.e. latch
		MOMENTARY_MODE // i.e. gate
	};
	enum ReturnMode {
		HARD_MODE,
		SOFT_MODE
	};
	ReturnMode returnMode = ReturnMode::HARD_MODE;
	ParamQuantity* launchParam, * slewTimeParam;
	dsp::SchmittTrigger launchCvTrigger;
	dsp::BooleanTrigger launchButtonTrigger;
	dsp::BooleanTrigger latchTrigger;
	dsp::SlewLimiter clickFilter;
	bool launchButtonHeld = false;
	bool applySaturation = true;
	bool active = false;

	struct GainParamQuantity : ParamQuantity {
		std::string getDisplayValueString() override {
			if (getValue() < 0.f) {
				return string::f("%g dB", 30 * getValue());
			}
			else {
				return string::f("%g dB", 12 * getValue());
			}
		}
	};

	Bypass() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		auto switchParam = configSwitch(MODE_PARAM, 0.f, 1.f, 0.f, "Return mode", {"Hard", "Soft"});
		switchParam->description = "In hard mode, Bypass wil cut off any sound coming from the loop.\nWith soft mode, the FX return is still active giving you reverb tails, decaying delay taps etc.";
		configParam<GainParamQuantity>(FX_GAIN_PARAM, -1.f, 1.f, 0.f, "FX return gain");
		configSwitch(LAUNCH_MODE_PARAM, 0.f, 1.f, 0.f, "Launch Mode", {"Latch (Toggle)", "Gate (Momentary)"});
		launchParam = configButton(LAUNCH_BUTTON_PARAM, "Launch");
		slewTimeParam = configParam(SLEW_TIME_PARAM, .005f, 0.05f, 0.01f, "Slew time", "s");

		configInput(IN_L_INPUT, "Left");
		configInput(IN_R_INPUT, "Right");
		configInput(FROM_FX_L_INPUT, "From FX L");
		configInput(FROM_FX_R_INPUT, "From FX R");
		configInput(LAUNCH_INPUT, "Launch");

		configOutput(TO_FX_L_OUTPUT, "To FX L");
		configOutput(TO_FX_R_OUTPUT, "To FX R");
		configOutput(OUT_L_OUTPUT, "Left");
		configOutput(OUT_R_OUTPUT, "Right");

		configBypass(IN_L_INPUT, OUT_L_OUTPUT);
		configBypass(IN_R_INPUT, OUT_R_OUTPUT);
	}

	void process(const ProcessArgs& args) override {

		// slew time in secs (so take inverse for lambda)
		clickFilter.rise = clickFilter.fall = 1.0 / params[SLEW_TIME_PARAM].getValue();

		const int maxInputChannels = std::max({1, inputs[IN_L_INPUT].getChannels(), inputs[IN_R_INPUT].getChannels()});
		const int maxFxReturnChannels = std::max({1, inputs[FROM_FX_L_INPUT].getChannels(), inputs[FROM_FX_R_INPUT].getChannels()});

		const LatchMode latchMode = (LatchMode) params[LAUNCH_MODE_PARAM].getValue();
		const ReturnMode returnMode = (ReturnMode) params[MODE_PARAM].getValue();


		const bool launchCvTriggered = launchCvTrigger.process(inputs[LAUNCH_INPUT].getVoltage());
		const bool launchButtonPressed = launchButtonTrigger.process(launchButtonHeld);

		// logical or (high if either high)
		const float launchValue = std::max(launchCvTrigger.isHigh(), launchButtonTrigger.isHigh());
		if (latchMode == LatchMode::TOGGLE_MODE) {
			const bool risingEdge = launchCvTriggered || launchButtonPressed;

			if (risingEdge) {
				active = !active;
			}
		}

		// FX send section
		const float sendActive = clickFilter.process(args.sampleTime, (latchMode == LatchMode::TOGGLE_MODE) ? active : launchValue);
		for (int c = 0; c < maxInputChannels; c += 4) {
			const float_4 inL = inputs[IN_L_INPUT].getPolyVoltageSimd<float_4>(c);
			const float_4 inR = inputs[IN_R_INPUT].getNormalPolyVoltageSimd<float_4>(inL, c);

			// we start be assuming that FXs can be polyphonic, but recognise that often they are not
			outputs[TO_FX_L_OUTPUT].setVoltageSimd<float_4>(inL * sendActive, c);
			outputs[TO_FX_R_OUTPUT].setVoltageSimd<float_4>(inR * sendActive, c);
		}
		// fx send polyphony is set by input polyphony
		outputs[TO_FX_L_OUTPUT].setChannels(maxInputChannels);
		outputs[TO_FX_R_OUTPUT].setChannels(maxInputChannels);


		// FX return section
		const float gainTaper = params[FX_GAIN_PARAM].getValue() < 0.f ? 30 * params[FX_GAIN_PARAM].getValue() : params[FX_GAIN_PARAM].getValue() * 12;
		const float fxReturnGain = std::pow(10, gainTaper / 20.0f);
		float_4 dryLeft, dryRight, outL, outR;
		for (int c = 0; c < maxFxReturnChannels; c += 4) {

			const bool fxMonophonic = (maxInputChannels == 1);
			if (fxMonophonic) {
				// if the return fx is monophonic, mix down dry inputs to monophonic also
				dryLeft = inputs[IN_L_INPUT].getVoltageSum();
				dryRight = inputs[IN_R_INPUT].isConnected() ? inputs[IN_R_INPUT].getVoltageSum() : inputs[IN_L_INPUT].getVoltageSum();
			}
			else {
				// if the return fx is polyphonic, then we don't need to do anything special
				dryLeft = inputs[IN_L_INPUT].getPolyVoltageSimd<float_4>(c);
				dryRight = inputs[IN_R_INPUT].getNormalPolyVoltageSimd<float_4>(dryLeft, c);
			}

			const float_4 fxLeftReturn = fxReturnGain * inputs[FROM_FX_L_INPUT].getPolyVoltageSimd<float_4>(c);
			const float_4 fxRightReturn = fxReturnGain * inputs[FROM_FX_R_INPUT].getPolyVoltageSimd<float_4>(c);

			if (returnMode == ReturnMode::HARD_MODE) {
				outL = dryLeft * (1 - sendActive) + sendActive * fxLeftReturn;
				outR = dryRight * (1 - sendActive) + sendActive * fxRightReturn;
			}
			else {
				outL = dryLeft * (1 - sendActive) + fxLeftReturn;
				outR = dryRight * (1 - sendActive) + fxRightReturn;
			}

			if (applySaturation) {
				outL = Saturator<float_4>::process(outL / 10.f) * 10.f;
				outR = Saturator<float_4>::process(outR / 10.f) * 10.f;
			}

			outputs[OUT_L_OUTPUT].setVoltageSimd<float_4>(outL, c);
			outputs[OUT_R_OUTPUT].setVoltageSimd<float_4>(outR, c);
		}

		// output polyphony is set by fx return polyphony
		outputs[OUT_L_OUTPUT].setChannels(maxFxReturnChannels);
		outputs[OUT_R_OUTPUT].setChannels(maxFxReturnChannels);

		lights[LAUNCH_LED].setSmoothBrightness(sendActive, args.sampleTime);
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* applySaturationJ = json_object_get(rootJ, "applySaturation");
		if (applySaturationJ) {
			applySaturation = json_boolean_value(applySaturationJ);
		}

		json_t* activeJ = json_object_get(rootJ, "active");
		if (activeJ) {
			active = json_boolean_value(activeJ);
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();

		json_object_set_new(rootJ, "applySaturation", json_boolean(applySaturation));
		json_object_set_new(rootJ, "active", json_boolean(active));

		return rootJ;
	}
};

/** From VCV Free */
struct VCVBezelBig : app::SvgSwitch {
	VCVBezelBig() {
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/components/VCVBezelBig.svg")));
	}
};

template <typename TBase>
struct VCVBezelLightBig : TBase {
	VCVBezelLightBig() {
		this->borderColor = color::WHITE_TRANSPARENT;
		this->bgColor = color::WHITE_TRANSPARENT;
		this->box.size = mm2px(math::Vec(11, 11));
	}
};

struct RecordButton : LightButton<VCVBezelBig, VCVBezelLightBig<RedLight>> {
	// Instead of using onAction() which is called on mouse up, handle on mouse down
	void onDragStart(const event::DragStart& e) override {
		Bypass* module = dynamic_cast<Bypass*>(this->module);
		if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
			if (module) {
				module->launchButtonHeld = true;
			}
		}

		LightButton::onDragStart(e);
	}

	void onDragEnd(const event::DragEnd& e) override {
		Bypass* module = dynamic_cast<Bypass*>(this->module);
		if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
			if (module) {
				module->launchButtonHeld = false;
			}
		}
	}
};

struct BypassWidget : ModuleWidget {

	SvgSwitch* launchParam;

	BypassWidget(Bypass* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Bypass.svg")));

		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParam<CKSSHoriz2>(mm2px(Vec(6.7, 63.263)), module, Bypass::MODE_PARAM));
		addParam(createParamCentered<BefacoTinyKnobWhite>(mm2px(Vec(10.0, 78.903)), module, Bypass::FX_GAIN_PARAM));
		addParam(createParam<CKSSNarrow>(mm2px(Vec(13.8, 91.6)), module, Bypass::LAUNCH_MODE_PARAM));

		launchParam = createLightParamCentered<RecordButton>(mm2px(Vec(10.0, 111.287)), module, Bypass::LAUNCH_BUTTON_PARAM, Bypass::LAUNCH_LED);
		addParam(launchParam);

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(15.016, 15.03)), module, Bypass::IN_R_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(4.947, 40.893)), module, Bypass::FROM_FX_L_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(15.001, 40.893)), module, Bypass::FROM_FX_R_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.648, 95.028)), module, Bypass::LAUNCH_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(4.947, 15.03)), module, Bypass::IN_L_INPUT));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(4.957, 27.961)), module, Bypass::TO_FX_L_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(14.957, 27.961)), module, Bypass::TO_FX_R_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(4.947, 53.846)), module, Bypass::OUT_L_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(14.957, 53.824)), module, Bypass::OUT_R_OUTPUT));
	}

	// for context menu
	struct SlewTimeSider : ui::Slider {
		explicit SlewTimeSider(ParamQuantity* q_) {
			quantity = q_;
			this->box.size.x = 200.0f;
		}
	};

	void appendContextMenu(Menu* menu) override {
		Bypass* module = dynamic_cast<Bypass*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator());
		menu->addChild(createBoolPtrMenuItem("Soft clip at ±10V", "", &module->applySaturation));
		menu->addChild(new SlewTimeSider(module->slewTimeParam));

	}
};


Model* modelBypass = createModel<Bypass, BypassWidget>("Bypass");