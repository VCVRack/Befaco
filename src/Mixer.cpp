#include "plugin.hpp"

using simd::float_4;

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
		OUT_BLUE_LIGHT,
		NUM_LIGHTS
	};

	Mixer() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(CH1_PARAM, 0.0, 1.0, 0.0, "Ch 1 level", "%", 0, 100);
		configParam(CH2_PARAM, 0.0, 1.0, 0.0, "Ch 2 level", "%", 0, 100);
		configParam(CH3_PARAM, 0.0, 1.0, 0.0, "Ch 3 level", "%", 0, 100);
		configParam(CH4_PARAM, 0.0, 1.0, 0.0, "Ch 4 level", "%", 0, 100);

		configOutput(OUT1_OUTPUT, "Main");
		configOutput(OUT2_OUTPUT, "Inverted");
	}

	void process(const ProcessArgs& args) override {
		int channels1 = inputs[IN1_INPUT].getChannels();
		int channels2 = inputs[IN2_INPUT].getChannels();
		int channels3 = inputs[IN3_INPUT].getChannels();
		int channels4 = inputs[IN4_INPUT].getChannels();

		int out_channels = 1;
		out_channels = std::max(out_channels, channels1);
		out_channels = std::max(out_channels, channels2);
		out_channels = std::max(out_channels, channels3);
		out_channels = std::max(out_channels, channels4);

		float_4 out[4] = {};

		if (inputs[IN1_INPUT].isConnected()) {
			for (int c = 0; c < channels1; c += 4)
				out[c / 4] += inputs[IN1_INPUT].getVoltageSimd<float_4>(c) * params[CH1_PARAM].getValue();
		}

		if (inputs[IN2_INPUT].isConnected()) {
			for (int c = 0; c < channels2; c += 4)
				out[c / 4] += inputs[IN2_INPUT].getVoltageSimd<float_4>(c) * params[CH2_PARAM].getValue();
		}

		if (inputs[IN3_INPUT].isConnected()) {
			for (int c = 0; c < channels3; c += 4)
				out[c / 4] += inputs[IN3_INPUT].getVoltageSimd<float_4>(c) * params[CH3_PARAM].getValue();
		}

		if (inputs[IN4_INPUT].isConnected()) {
			for (int c = 0; c < channels4; c += 4)
				out[c / 4] += inputs[IN4_INPUT].getVoltageSimd<float_4>(c) * params[CH4_PARAM].getValue();
		}

		outputs[OUT1_OUTPUT].setChannels(out_channels);
		outputs[OUT2_OUTPUT].setChannels(out_channels);

		for (int c = 0; c < out_channels; c += 4) {
			outputs[OUT1_OUTPUT].setVoltageSimd(out[c / 4], c);
			out[c / 4] *= -1.f;
			outputs[OUT2_OUTPUT].setVoltageSimd(out[c / 4], c);
		}

		if (out_channels == 1) {
			float light = outputs[OUT1_OUTPUT].getVoltage();
			lights[OUT_POS_LIGHT].setSmoothBrightness(light / 5.f, args.sampleTime);
			lights[OUT_NEG_LIGHT].setSmoothBrightness(-light / 5.f, args.sampleTime);
		}
		else {
			float light = 0.0f;
			for (int c = 0; c < out_channels; c++) {
				float tmp = outputs[OUT1_OUTPUT].getVoltage(c);
				light += tmp * tmp;
			}
			light = std::sqrt(light);
			lights[OUT_POS_LIGHT].setBrightness(0.0f);
			lights[OUT_NEG_LIGHT].setBrightness(0.0f);
			lights[OUT_BLUE_LIGHT].setSmoothBrightness(light / 5.f, args.sampleTime);
		}
	}
};


struct MixerWidget : ModuleWidget {
	MixerWidget(Mixer* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/Mixer.svg")));

		addChild(createWidget<Knurlie>(Vec(15, 0)));
		addChild(createWidget<Knurlie>(Vec(15, 365)));

		addParam(createParam<Davies1900hWhiteKnob>(Vec(19, 32), module, Mixer::CH1_PARAM));
		addParam(createParam<Davies1900hWhiteKnob>(Vec(19, 85), module, Mixer::CH2_PARAM));
		addParam(createParam<Davies1900hWhiteKnob>(Vec(19, 137), module, Mixer::CH3_PARAM));
		addParam(createParam<Davies1900hWhiteKnob>(Vec(19, 190), module, Mixer::CH4_PARAM));

		addInput(createInput<BefacoInputPort>(Vec(7, 242), module, Mixer::IN1_INPUT));
		addInput(createInput<BefacoInputPort>(Vec(43, 242), module, Mixer::IN2_INPUT));

		addInput(createInput<BefacoInputPort>(Vec(7, 281), module, Mixer::IN3_INPUT));
		addInput(createInput<BefacoInputPort>(Vec(43, 281), module, Mixer::IN4_INPUT));

		addOutput(createOutput<BefacoOutputPort>(Vec(7, 324), module, Mixer::OUT1_OUTPUT));
		addOutput(createOutput<BefacoOutputPort>(Vec(43, 324), module, Mixer::OUT2_OUTPUT));

		addChild(createLight<MediumLight<RedGreenBlueLight>>(Vec(32.7, 310), module, Mixer::OUT_POS_LIGHT));
	}
};


Model* modelMixer = createModel<Mixer, MixerWidget>("Mixer");
