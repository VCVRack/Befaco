#include "plugin.hpp"

using simd::float_4;

template <typename T>
static T clip(T x) {
	// Pade approximant of x/(1 + x^12)^(1/12)
	const T limit = 1.16691853009184f;
	x = clamp(x * 0.1f, -limit, limit);
	return 10.0f * (x + 1.45833f * simd::pow(x, 13) + 0.559028f * simd::pow(x, 25) + 0.0427035f * simd::pow(x, 37))
	       / (1.0f + 1.54167f * simd::pow(x, 12) + 0.642361f * simd::pow(x, 24) + 0.0579909f * simd::pow(x, 36));
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

		configInput(A1_INPUT, "A1");
		configInput(B1_INPUT, "B1");
		configInput(C1_INPUT, "C1");
		configInput(A2_INPUT, "A2");
		configInput(B2_INPUT, "B2");
		configInput(C2_INPUT, "C2");
		
		getInputInfo(B1_INPUT)->description = "Normalled to 5V";
		getInputInfo(C1_INPUT)->description = "Normalled to 10V";
		getInputInfo(B2_INPUT)->description = "Normalled to 5V";
		getInputInfo(C2_INPUT)->description = "Normalled to 10V";
 
		configOutput(OUT1_OUTPUT, "Out 1");
		configOutput(OUT2_OUTPUT, "Out 2");
		
		getOutputInfo(OUT1_OUTPUT)->description = "Normalled to Out 2";
	}

	void processSection(const ProcessArgs& args, int& lastChannels, float_4* lastOut, ParamIds levelB, ParamIds levelC, InputIds inputA, InputIds inputB, InputIds inputC, OutputIds output, LightIds outLight) {
		// Compute polyphony channels
		int channels = std::max(lastChannels, inputs[inputA].getChannels());
		channels = std::max(channels, inputs[inputB].getChannels());
		channels = std::max(channels, inputs[inputC].getChannels());
		lastChannels = channels;

		// Get param levels
		float gainB = 2.f * exponentialBipolar80Pade_5_4(params[levelB].getValue());
		float gainC = exponentialBipolar80Pade_5_4(params[levelC].getValue());

		for (int c = 0; c < channels; c += 4) {
			// Get inputs
			float_4 inA = inputs[inputA].getPolyVoltageSimd<float_4>(c);
			float_4 inB = inputs[inputB].getNormalPolyVoltageSimd<float_4>(5.f, c) * gainB;
			float_4 inC = inputs[inputC].getNormalPolyVoltageSimd<float_4>(10.f, c) * gainC;

			// Compute and set output
			float_4 out = inA * inB / 5.f + inC;
			lastOut[c / 4] += out;
			if (outputs[output].isConnected()) {
				outputs[output].setChannels(channels);
				outputs[output].setVoltageSimd(clip(lastOut[c / 4]), c);
			}
		}

		// Set lights
		if (channels == 1) {
			float b = lastOut[0][0];
			lights[outLight + 0].setSmoothBrightness(b / 5.f, args.sampleTime);
			lights[outLight + 1].setSmoothBrightness(-b / 5.f, args.sampleTime);
			lights[outLight + 2].setBrightness(0.f);
		}
		else {
			// RMS of output
			float b = 0.f;
			for (int c = 0; c < channels; c++)
				b += std::pow(lastOut[c / 4][c % 4], 2);
			b = std::sqrt(b);
			lights[outLight + 0].setBrightness(0.0f);
			lights[outLight + 1].setBrightness(0.0f);
			lights[outLight + 2].setBrightness(b);
		}
	}

	void process(const ProcessArgs& args) override {
		int channels = 1;
		float_4 out[4] = {};

		// Section A
		processSection(args, channels, out, B1_LEVEL_PARAM, C1_LEVEL_PARAM, A1_INPUT, B1_INPUT, C1_INPUT, OUT1_OUTPUT, OUT1_LIGHT);

		// Break summing if output A is patched
		if (outputs[OUT1_OUTPUT].isConnected()) {
			channels = 1;
			std::memset(out, 0, sizeof(out));
		}

		// Section B
		processSection(args, channels, out, B2_LEVEL_PARAM, C2_LEVEL_PARAM, A2_INPUT, B2_INPUT, C2_INPUT, OUT2_OUTPUT, OUT2_LIGHT);
	}
};


struct ABCWidget : ModuleWidget {
	ABCWidget(ABC* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/ABC.svg")));

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
