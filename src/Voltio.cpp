#include "plugin.hpp"

using simd::float_4;

struct Davies1900hLargeLightGreyKnobCustom : Davies1900hLargeLightGreyKnob {
	widget::SvgWidget* bg;

	Davies1900hLargeLightGreyKnobCustom() {
		minAngle = -0.83 * M_PI;
		maxAngle = M_PI;

		bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);
	}
};

struct Voltio : Module {
	enum ParamId {
		OCT_PARAM,
		RANGE_PARAM,
		SEMITONES_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		SUM_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		PLUSMINUS5_LIGHT,
		ZEROTOTEN_LIGHT,
		LIGHTS_LEN
	};

	Voltio() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		auto octParam = configParam(OCT_PARAM, 0.f, 10.f, 0.f, "Octave");
		octParam->snapEnabled = true;

		configSwitch(RANGE_PARAM, 0.f, 1.f, 0.f, "Range", {"-5 to +5", "0 to 10"});
		auto semitonesParam = configParam(SEMITONES_PARAM, 0.f, 11.f, 0.f, "Semitones");
		semitonesParam->snapEnabled = true;

		configInput(SUM_INPUT, "Sum");
		configOutput(OUT_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
		const int channels = std::max(1, inputs[SUM_INPUT].getChannels());

		for (int c = 0; c < channels; c += 4) {
			float_4 in = inputs[SUM_INPUT].getPolyVoltageSimd<float_4>(c);

			float offset = params[RANGE_PARAM].getValue() ? -5.f : 0.f;
			in += params[SEMITONES_PARAM].getValue() / 12.f + params[OCT_PARAM].getValue() + offset;

			outputs[OUT_OUTPUT].setVoltageSimd<float_4>(in, c);
		}

		outputs[OUT_OUTPUT].setChannels(channels);

		lights[PLUSMINUS5_LIGHT].setBrightness(params[RANGE_PARAM].getValue() ? 1.f : 0.f);
		lights[ZEROTOTEN_LIGHT].setBrightness(params[RANGE_PARAM].getValue() ? 0.f : 1.f);
	}

};


struct VoltioWidget : ModuleWidget {
	VoltioWidget(Voltio* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Voltio.svg")));

		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Davies1900hLargeLightGreyKnob>(mm2px(Vec(15.0, 20.828)), module, Voltio::OCT_PARAM));
		addParam(createParamCentered<BefacoSwitch>(mm2px(Vec(22.083, 44.061)), module, Voltio::RANGE_PARAM));
		addParam(createParamCentered<Davies1900hLargeLightGreyKnobCustom>(mm2px(Vec(15.0, 67.275)), module, Voltio::SEMITONES_PARAM));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(7.117, 111.003)), module, Voltio::SUM_INPUT));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(22.661, 111.003)), module, Voltio::OUT_OUTPUT));

		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(5.695, 41.541)), module, Voltio::PLUSMINUS5_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(5.695, 46.633)), module, Voltio::ZEROTOTEN_LIGHT));
	}
};


Model* modelVoltio = createModel<Voltio, VoltioWidget>("Voltio");