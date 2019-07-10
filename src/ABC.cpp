#include "plugin.hpp"


inline float clip(float x) {
	// Pade approximant of x/(1 + x^12)^(1/12)
	const float limit = 1.16691853009184f;
	x = clamp(x, -limit, limit);
	return (x + 1.45833f * std::pow(x, 13) + 0.559028f * std::pow(x, 25) + 0.0427035f * std::pow(x, 37))
		/ (1 + 1.54167f * std::pow(x, 12) + 0.642361f * std::pow(x, 24) + 0.0579909f * std::pow(x, 36));
}

inline float exponentialBipolar80Pade_5_4(float x) {
	return (0.109568f * x + 0.281588f * std::pow(x, 3) + 0.133841f * std::pow(x, 5))
		/ (1 - 0.630374f * std::pow(x, 2) + 0.166271f * std::pow(x, 4));
}


struct ABC : Module {
	enum ParamIds {
		B1_LEVEL_PARAM,
		C1_LEVEL_PARAM,
		B2_LEVEL_PARAM,
		C2_LEVEL_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		A1_INPUT,
		B1_INPUT,
		C1_INPUT,
		A2_INPUT,
		B2_INPUT,
		C2_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT1_OUTPUT,
		OUT2_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(OUT1_LIGHT, 2),
		ENUMS(OUT2_LIGHT, 2),
		NUM_LIGHTS
	};

	ABC() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(B1_LEVEL_PARAM, -1.0, 1.0, 0.0, "B1 Level");
		configParam(C1_LEVEL_PARAM, -1.0, 1.0, 0.0, "C1 Level");
		configParam(B2_LEVEL_PARAM, -1.0, 1.0, 0.0, "B2 Level");
		configParam(C2_LEVEL_PARAM, -1.0, 1.0, 0.0, "C2 Level");
	}

	void process(const ProcessArgs &args) override {
		float a1 = inputs[A1_INPUT].getVoltage();
		float b1 = inputs[B1_INPUT].getNormalVoltage(5.f) * 2.f*exponentialBipolar80Pade_5_4(params[B1_LEVEL_PARAM].getValue());
		float c1 = inputs[C1_INPUT].getNormalVoltage(10.f) * exponentialBipolar80Pade_5_4(params[C1_LEVEL_PARAM].getValue());
		float out1 = a1 * b1 / 5.f + c1;

		float a2 = inputs[A2_INPUT].getVoltage();
		float b2 = inputs[B2_INPUT].getNormalVoltage(5.f) * 2.f*exponentialBipolar80Pade_5_4(params[B2_LEVEL_PARAM].getValue());
		float c2 = inputs[C2_INPUT].getNormalVoltage(10.f) * exponentialBipolar80Pade_5_4(params[C2_LEVEL_PARAM].getValue());
		float out2 = a2 * b2 / 5.f + c2;

		// Set outputs
		if (outputs[OUT1_OUTPUT].isConnected()) {
			outputs[OUT1_OUTPUT].setVoltage(clip(out1 / 10.f) * 10.f);
		}
		else {
			out2 += out1;
		}
		if (outputs[OUT2_OUTPUT].isConnected()) {
			outputs[OUT2_OUTPUT].setVoltage(clip(out2 / 10.f) * 10.f);
		}

		// Lights
		lights[OUT1_LIGHT + 0].setSmoothBrightness(out1 / 5.f, args.sampleTime);
		lights[OUT1_LIGHT + 1].setSmoothBrightness(-out1 / 5.f, args.sampleTime);
		lights[OUT2_LIGHT + 0].setSmoothBrightness(out2 / 5.f, args.sampleTime);
		lights[OUT2_LIGHT + 1].setSmoothBrightness(-out2 / 5.f, args.sampleTime);
	}
};


struct ABCWidget : ModuleWidget {
	ABCWidget(ABC *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ABC.svg")));

		addChild(createWidget<Knurlie>(Vec(15, 0)));
		addChild(createWidget<Knurlie>(Vec(15, 365)));

		addParam(createParam<Davies1900hRedKnob>(Vec(45, 37), module, ABC::B1_LEVEL_PARAM));
		addParam(createParam<Davies1900hWhiteKnob>(Vec(45, 107), module, ABC::C1_LEVEL_PARAM));
		addParam(createParam<Davies1900hRedKnob>(Vec(45, 204), module, ABC::B2_LEVEL_PARAM));
		addParam(createParam<Davies1900hWhiteKnob>(Vec(45, 274), module, ABC::C2_LEVEL_PARAM));

		addInput(createInput<PJ301MPort>(Vec(7, 28), module, ABC::A1_INPUT));
		addInput(createInput<PJ301MPort>(Vec(7, 70), module, ABC::B1_INPUT));
		addInput(createInput<PJ301MPort>(Vec(7, 112), module, ABC::C1_INPUT));
		addOutput(createOutput<PJ301MPort>(Vec(7, 154), module, ABC::OUT1_OUTPUT));
		addInput(createInput<PJ301MPort>(Vec(7, 195), module, ABC::A2_INPUT));
		addInput(createInput<PJ301MPort>(Vec(7, 237), module, ABC::B2_INPUT));
		addInput(createInput<PJ301MPort>(Vec(7, 279), module, ABC::C2_INPUT));
		addOutput(createOutput<PJ301MPort>(Vec(7, 321), module, ABC::OUT2_OUTPUT));

		addChild(createLight<MediumLight<GreenRedLight>>(Vec(37, 162), module, ABC::OUT1_LIGHT));
		addChild(createLight<MediumLight<GreenRedLight>>(Vec(37, 329), module, ABC::OUT2_LIGHT));
	}
};


Model *modelABC = createModel<ABC, ABCWidget>("ABC");
