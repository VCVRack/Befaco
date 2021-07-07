#include "plugin.hpp"

using simd::float_4;

template <typename T>
static T clip4(T x) {
	// Pade approximant of x/(1 + x^12)^(1/12)
	const T limit = 1.16691853009184f;
	x = clamp(x * 0.1f, -limit, limit);
	return 10.0f * (x + 1.45833f * simd::pow(x, 13) + 0.559028f * simd::pow(x, 25) + 0.0427035f * simd::pow(x, 37))
	       / (1.0f + 1.54167f * simd::pow(x, 12) + 0.642361f * simd::pow(x, 24) + 0.0579909f * simd::pow(x, 36));
}


static float exponentialBipolar80Pade_5_4(float x) {
	return (0.109568 * x + 0.281588 * std::pow(x, 3) + 0.133841 * std::pow(x, 5))
	       / (1. - 0.630374 * std::pow(x, 2) + 0.166271 * std::pow(x, 4));
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
		ENUMS(OUT1_LIGHT, 3),
		ENUMS(OUT2_LIGHT, 3),
		NUM_LIGHTS
	};

	ABC() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(B1_LEVEL_PARAM, -1.0, 1.0, 0.0, "B1 Level");
		configParam(C1_LEVEL_PARAM, -1.0, 1.0, 0.0, "C1 Level");
		configParam(B2_LEVEL_PARAM, -1.0, 1.0, 0.0, "B2 Level");
		configParam(C2_LEVEL_PARAM, -1.0, 1.0, 0.0, "C2 Level");
	}

	int processSection(simd::float_4* out, InputIds inputA, InputIds inputB, InputIds inputC, ParamIds levelB, ParamIds levelC) {

		float_4 inA[4] = {0.f};
		float_4 inB[4] = {0.f};
		float_4 inC[4] = {0.f};

		int channelsA = inputs[inputA].getChannels();
		int channelsB = inputs[inputB].getChannels();
		int channelsC = inputs[inputC].getChannels();

		// this sets the number of active engines (according to polyphony standard)
		// NOTE: A*B + C has the number of active engines set by any one of the three inputs
		int activeEngines = std::max(1, channelsA);
		activeEngines = std::max(activeEngines, channelsB);
		activeEngines = std::max(activeEngines, channelsC);

		float mult_B = (2.f / 5.f) * exponentialBipolar80Pade_5_4(params[levelB].getValue());
		float mult_C = exponentialBipolar80Pade_5_4(params[levelC].getValue());

		if (inputs[inputA].isConnected()) {
			for (int c = 0; c < activeEngines; c += 4)
				inA[c / 4] = inputs[inputA].getPolyVoltageSimd<float_4>(c);
		}

		if (inputs[inputB].isConnected()) {
			for (int c = 0; c < activeEngines; c += 4)
				inB[c / 4] = inputs[inputB].getPolyVoltageSimd<float_4>(c) * mult_B;
		}
		else {
			for (int c = 0; c < activeEngines; c += 4)
				inB[c / 4] = 5.f * mult_B;
		}

		if (inputs[inputC].isConnected()) {
			for (int c = 0; c < activeEngines; c += 4)
				inC[c / 4] = inputs[inputC].getPolyVoltageSimd<float_4>(c) * mult_C;
		}
		else {
			for (int c = 0; c < activeEngines; c += 4)
				inC[c / 4] = float_4(10.f * mult_C);
		}

		for (int c = 0; c < activeEngines; c += 4)
			out[c / 4] = clip4(inA[c / 4] * inB[c / 4] + inC[c / 4]);

		return activeEngines;
	}

	void process(const ProcessArgs& args) override {

		// process upper section
		float_4 out1[4] = {0.f};
		int activeEngines1 = 1;
		if (outputs[OUT1_OUTPUT].isConnected() || outputs[OUT2_OUTPUT].isConnected()) {
			activeEngines1 = processSection(out1, A1_INPUT, B1_INPUT, C1_INPUT, B1_LEVEL_PARAM, C1_LEVEL_PARAM);
		}

		float_4 out2[4] = {0.f};
		int activeEngines2 = 1;
		// process lower section
		if (outputs[OUT2_OUTPUT].isConnected()) {
			activeEngines2 = processSection(out2, A2_INPUT, B2_INPUT, C2_INPUT, B2_LEVEL_PARAM, C2_LEVEL_PARAM);
		}

		// Set outputs
		if (outputs[OUT1_OUTPUT].isConnected()) {
			outputs[OUT1_OUTPUT].setChannels(activeEngines1);
			for (int c = 0; c < activeEngines1; c += 4)
				outputs[OUT1_OUTPUT].setVoltageSimd(out1[c / 4], c);
		}
		else if (outputs[OUT2_OUTPUT].isConnected()) {

			for (int c = 0; c < activeEngines1; c += 4)
				out2[c / 4] += out1[c / 4];

			activeEngines2 = std::max(activeEngines1, activeEngines2);

			outputs[OUT2_OUTPUT].setChannels(activeEngines2);
			for (int c = 0; c < activeEngines2; c += 4)
				outputs[OUT2_OUTPUT].setVoltageSimd(out2[c / 4], c);
		}

		// Lights

		if (activeEngines1 == 1) {
			float b = out1[0].s[0];
			lights[OUT1_LIGHT + 0].setSmoothBrightness(b / 5.f, args.sampleTime);
			lights[OUT1_LIGHT + 1].setSmoothBrightness(-b / 5.f, args.sampleTime);
			lights[OUT1_LIGHT + 2].setBrightness(0.f);
		}
		else {
			float b = 10.f;
			lights[OUT1_LIGHT + 0].setBrightness(0.0f);
			lights[OUT1_LIGHT + 1].setBrightness(0.0f);
			lights[OUT1_LIGHT + 2].setBrightness(b);
		}

		if (activeEngines2 == 1) {
			float b = out2[0].s[0];
			lights[OUT2_LIGHT + 0].setSmoothBrightness(b / 5.f, args.sampleTime);
			lights[OUT2_LIGHT + 1].setSmoothBrightness(-b / 5.f, args.sampleTime);
			lights[OUT2_LIGHT + 2].setBrightness(0.f);
		}
		else {
			float b = 10.f;
			lights[OUT2_LIGHT + 0].setBrightness(0.0f);
			lights[OUT2_LIGHT + 1].setBrightness(0.0f);
			lights[OUT2_LIGHT + 2].setBrightness(b);
		}
	}
};


struct ABCWidget : ModuleWidget {
	ABCWidget(ABC* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ABC.svg")));

		addChild(createWidget<Knurlie>(Vec(15, 0)));
		addChild(createWidget<Knurlie>(Vec(15, 365)));

		addParam(createParam<Davies1900hRedKnob>(Vec(45, 37), module, ABC::B1_LEVEL_PARAM));
		addParam(createParam<Davies1900hWhiteKnob>(Vec(45, 107), module, ABC::C1_LEVEL_PARAM));
		addParam(createParam<Davies1900hRedKnob>(Vec(45, 204), module, ABC::B2_LEVEL_PARAM));
		addParam(createParam<Davies1900hWhiteKnob>(Vec(45, 274), module, ABC::C2_LEVEL_PARAM));

		addInput(createInput<BefacoInputPort>(Vec(7, 28), module, ABC::A1_INPUT));
		addInput(createInput<BefacoInputPort>(Vec(7, 70), module, ABC::B1_INPUT));
		addInput(createInput<BefacoInputPort>(Vec(7, 112), module, ABC::C1_INPUT));
		addOutput(createOutput<BefacoOutputPort>(Vec(7, 154), module, ABC::OUT1_OUTPUT));
		addInput(createInput<BefacoInputPort>(Vec(7, 195), module, ABC::A2_INPUT));
		addInput(createInput<BefacoInputPort>(Vec(7, 237), module, ABC::B2_INPUT));
		addInput(createInput<BefacoInputPort>(Vec(7, 279), module, ABC::C2_INPUT));
		addOutput(createOutput<BefacoOutputPort>(Vec(7, 321), module, ABC::OUT2_OUTPUT));

		addChild(createLight<MediumLight<RedGreenBlueLight>>(Vec(37, 162), module, ABC::OUT1_LIGHT));
		addChild(createLight<MediumLight<RedGreenBlueLight>>(Vec(37, 329), module, ABC::OUT2_LIGHT));
	}
};


Model* modelABC = createModel<ABC, ABCWidget>("ABC");
