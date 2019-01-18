#include "Befaco.hpp"



static float clip(float x) {
	x = clamp(x, -2.f, 2.f);
	return x / std::pow(1.f + std::pow(x, 24.f), 1/24.f);
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
		OUT1_POS_LIGHT,
		OUT1_NEG_LIGHT,
		OUT2_POS_LIGHT,
		OUT2_NEG_LIGHT,
		NUM_LIGHTS
	};

	ABC() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		params[B1_LEVEL_PARAM].config(-1.0, 1.0, 0.0, "B1 Level");
		params[C1_LEVEL_PARAM].config(-1.0, 1.0, 0.0, "C1 Level");
		params[B2_LEVEL_PARAM].config(-1.0, 1.0, 0.0, "B2 Level");
		params[C2_LEVEL_PARAM].config(-1.0, 1.0, 0.0, "C2 Level");
	}

	void step() override {
		float a1 = inputs[A1_INPUT].value;
		float b1 = inputs[B1_INPUT].normalize(5.f) * 2.f*dsp::exponentialBipolar(80.f, params[B1_LEVEL_PARAM].value);
		float c1 = inputs[C1_INPUT].normalize(10.f) * dsp::exponentialBipolar(80.f, params[C1_LEVEL_PARAM].value);
		float out1 = a1 * b1 / 5.f + c1;

		float a2 = inputs[A2_INPUT].value;
		float b2 = inputs[B2_INPUT].normalize(5.f) * 2.f*dsp::exponentialBipolar(80.f, params[B2_LEVEL_PARAM].value);
		float c2 = inputs[C2_INPUT].normalize(10.f) * dsp::exponentialBipolar(80.f, params[C2_LEVEL_PARAM].value);
		float out2 = a2 * b2 / 5.f + c2;

		// Set outputs
		if (outputs[OUT1_OUTPUT].active) {
			outputs[OUT1_OUTPUT].value = clip(out1 / 10.f) * 10.f;
		}
		else {
			out2 += out1;
		}
		if (outputs[OUT2_OUTPUT].active) {
			outputs[OUT2_OUTPUT].value = clip(out2 / 10.f) * 10.f;
		}

		// Lights
		lights[OUT1_POS_LIGHT].setBrightnessSmooth(std::max(0.f, out1 / 5.f));
		lights[OUT1_NEG_LIGHT].setBrightnessSmooth(std::max(0.f, -out1 / 5.f));
		lights[OUT2_POS_LIGHT].setBrightnessSmooth(std::max(0.f, out2 / 5.f));
		lights[OUT2_NEG_LIGHT].setBrightnessSmooth(std::max(0.f, -out2 / 5.f));
	}
};


struct ABCWidget : ModuleWidget {
	ABCWidget(ABC *module) {
		setModule(module);
		setPanel(SVG::load(asset::plugin(plugin, "res/ABC.svg")));

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

		addChild(createLight<MediumLight<GreenRedLight>>(Vec(37, 162), module, ABC::OUT1_POS_LIGHT));
		addChild(createLight<MediumLight<GreenRedLight>>(Vec(37, 329), module, ABC::OUT2_POS_LIGHT));
	}
};


Model *modelABC = createModel<ABC, ABCWidget>("ABC");
