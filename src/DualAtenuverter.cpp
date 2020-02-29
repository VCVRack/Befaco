#include "plugin.hpp"


struct DualAtenuverter : Module {
	enum ParamIds {
		ATEN1_PARAM,
		OFFSET1_PARAM,
		ATEN2_PARAM,
		OFFSET2_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		IN1_INPUT,
		IN2_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT1_OUTPUT,
		OUT2_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		OUT1_POS_LIGHT,
		OUT1_NEG_LIGHT,
		OUT2_POS_LIGHT,
		OUT2_NEG_LIGHT,
		NUM_LIGHTS
	};

	DualAtenuverter() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(ATEN1_PARAM, -1.0, 1.0, 0.0, "Ch 1 gain");
		configParam(OFFSET1_PARAM, -10.0, 10.0, 0.0, "Ch 1 offset", " V");
		configParam(ATEN2_PARAM, -1.0, 1.0, 0.0, "Ch 2 gain");
		configParam(OFFSET2_PARAM, -10.0, 10.0, 0.0, "Ch 2 offset", " V");
		configBypass(IN1_INPUT, OUT1_OUTPUT);
		configBypass(IN2_INPUT, OUT2_OUTPUT);
	}

	void process(const ProcessArgs &args) override {
		float out1 = inputs[IN1_INPUT].getVoltage() * params[ATEN1_PARAM].getValue() + params[OFFSET1_PARAM].getValue();
		float out2 = inputs[IN2_INPUT].getVoltage() * params[ATEN2_PARAM].getValue() + params[OFFSET2_PARAM].getValue();
		out1 = clamp(out1, -10.f, 10.f);
		out2 = clamp(out2, -10.f, 10.f);

		outputs[OUT1_OUTPUT].setVoltage(out1);
		outputs[OUT2_OUTPUT].setVoltage(out2);
		lights[OUT1_POS_LIGHT].setSmoothBrightness(out1 / 5.f, args.sampleTime);
		lights[OUT1_NEG_LIGHT].setSmoothBrightness(-out1 / 5.f, args.sampleTime);
		lights[OUT2_POS_LIGHT].setSmoothBrightness(out2 / 5.f, args.sampleTime);
		lights[OUT2_NEG_LIGHT].setSmoothBrightness(-out2 / 5.f, args.sampleTime);
	}
};


struct DualAtenuverterWidget : ModuleWidget {
	DualAtenuverterWidget(DualAtenuverter *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/DualAtenuverter.svg")));

		addChild(createWidget<Knurlie>(Vec(15, 0)));
		addChild(createWidget<Knurlie>(Vec(15, 365)));

		addParam(createParam<Davies1900hWhiteKnob>(Vec(20, 33), module, DualAtenuverter::ATEN1_PARAM));
		addParam(createParam<Davies1900hRedKnob>(Vec(20, 91), module, DualAtenuverter::OFFSET1_PARAM));
		addParam(createParam<Davies1900hWhiteKnob>(Vec(20, 201), module, DualAtenuverter::ATEN2_PARAM));
		addParam(createParam<Davies1900hRedKnob>(Vec(20, 260), module, DualAtenuverter::OFFSET2_PARAM));

		addInput(createInput<PJ301MPort>(Vec(7, 152), module, DualAtenuverter::IN1_INPUT));
		addOutput(createOutput<PJ301MPort>(Vec(43, 152), module, DualAtenuverter::OUT1_OUTPUT));

		addInput(createInput<PJ301MPort>(Vec(7, 319), module, DualAtenuverter::IN2_INPUT));
		addOutput(createOutput<PJ301MPort>(Vec(43, 319), module, DualAtenuverter::OUT2_OUTPUT));

		addChild(createLight<MediumLight<GreenRedLight>>(Vec(33, 143), module, DualAtenuverter::OUT1_POS_LIGHT));
		addChild(createLight<MediumLight<GreenRedLight>>(Vec(33, 311), module, DualAtenuverter::OUT2_POS_LIGHT));
	}
};


Model *modelDualAtenuverter = createModel<DualAtenuverter, DualAtenuverterWidget>("DualAtenuverter");
