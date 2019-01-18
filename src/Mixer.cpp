#include "Befaco.hpp"


struct Mixer : Module {
	enum ParamIds {
		CH1_PARAM,
		CH2_PARAM,
		CH3_PARAM,
		CH4_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		IN1_INPUT,
		IN2_INPUT,
		IN3_INPUT,
		IN4_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT1_OUTPUT,
		OUT2_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		OUT_POS_LIGHT,
		OUT_NEG_LIGHT,
		NUM_LIGHTS
	};

	Mixer() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		params[CH1_PARAM].config(0.0, 1.0, 0.0, "Ch 1 level", "%", 0, 100);
		params[CH2_PARAM].config(0.0, 1.0, 0.0, "Ch 2 level", "%", 0, 100);
		params[CH3_PARAM].config(0.0, 1.0, 0.0, "Ch 3 level", "%", 0, 100);
		params[CH4_PARAM].config(0.0, 1.0, 0.0, "Ch 4 level", "%", 0, 100);
	}

	void step() override {
		float in1 = inputs[IN1_INPUT].value * params[CH1_PARAM].value;
		float in2 = inputs[IN2_INPUT].value * params[CH2_PARAM].value;
		float in3 = inputs[IN3_INPUT].value * params[CH3_PARAM].value;
		float in4 = inputs[IN4_INPUT].value * params[CH4_PARAM].value;

		float out = in1 + in2 + in3 + in4;
		outputs[OUT1_OUTPUT].value = out;
		outputs[OUT2_OUTPUT].value = -out;
		lights[OUT_POS_LIGHT].setBrightnessSmooth(out / 5.f);
		lights[OUT_NEG_LIGHT].setBrightnessSmooth(-out / 5.f);
	}
};




struct MixerWidget : ModuleWidget {
	MixerWidget(Mixer *module) {
		setModule(module);
		setPanel(SVG::load(asset::plugin(plugin, "res/Mixer.svg")));

		addChild(createWidget<Knurlie>(Vec(15, 0)));
		addChild(createWidget<Knurlie>(Vec(15, 365)));

		addParam(createParam<Davies1900hWhiteKnob>(Vec(19, 32), module, Mixer::CH1_PARAM));
		addParam(createParam<Davies1900hWhiteKnob>(Vec(19, 85), module, Mixer::CH2_PARAM));
		addParam(createParam<Davies1900hWhiteKnob>(Vec(19, 137), module, Mixer::CH3_PARAM));
		addParam(createParam<Davies1900hWhiteKnob>(Vec(19, 190), module, Mixer::CH4_PARAM));

		addInput(createInput<PJ301MPort>(Vec(7, 242), module, Mixer::IN1_INPUT));
		addInput(createInput<PJ301MPort>(Vec(43, 242), module, Mixer::IN2_INPUT));

		addInput(createInput<PJ301MPort>(Vec(7, 281), module, Mixer::IN3_INPUT));
		addInput(createInput<PJ301MPort>(Vec(43, 281), module, Mixer::IN4_INPUT));

		addOutput(createOutput<PJ301MPort>(Vec(7, 324), module, Mixer::OUT1_OUTPUT));
		addOutput(createOutput<PJ301MPort>(Vec(43, 324), module, Mixer::OUT2_OUTPUT));

		addChild(createLight<MediumLight<GreenRedLight>>(Vec(32.7, 310), module, Mixer::OUT_POS_LIGHT));
	}
};


Model *modelMixer = createModel<Mixer, MixerWidget>("Mixer");
