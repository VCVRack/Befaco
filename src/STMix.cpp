#include "plugin.hpp"


struct STMix : Module {

	// NOTE: not including auxilary channel
	static const int numMixerChannels = 4;

	enum ParamIds {
		ENUMS(GAIN_PARAM, numMixerChannels),
		NUM_PARAMS
	};
	enum InputIds {
		// +1 for aux
		ENUMS(LEFT_INPUT, numMixerChannels + 1),
		ENUMS(RIGHT_INPUT, numMixerChannels + 1),
		NUM_INPUTS
	};
	enum OutputIds {
		LEFT_OUTPUT,
		RIGHT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		LEFT_LED,
		RIGHT_LED,
		NUM_LIGHTS
	};

	STMix() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		for (int i = 0; i < numMixerChannels; ++i) {
			configParam(GAIN_PARAM + i, 0.f, 1.f, 0.f, string::f("Gain %d", i + 1));
		}

	}

	void process(const ProcessArgs& args) override {
		float out[2] = {};

		for (int i = 0; i < numMixerChannels + 1; ++i) {

			const float in_left = inputs[LEFT_INPUT + i].getNormalVoltage(0.f);
			const float in_right = inputs[RIGHT_INPUT + i].getNormalVoltage(in_left);
			const float gain = (i < numMixerChannels) ? params[GAIN_PARAM + i].getValue() : 1.f;

			out[0] += in_left * gain;
			out[1] += in_right * gain;
		}

		outputs[LEFT_OUTPUT].setVoltage(out[0]);
		outputs[RIGHT_OUTPUT].setVoltage(out[1]);

		lights[LEFT_LED].setSmoothBrightness(outputs[LEFT_OUTPUT].getVoltage() / 5.f, args.sampleTime);
		lights[RIGHT_LED].setSmoothBrightness(outputs[RIGHT_OUTPUT].getVoltage() / 5.f, args.sampleTime);
	}
};


struct STMixWidget : ModuleWidget {
	STMixWidget(STMix* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/STMix.svg")));

		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(20.892, 18.141)), module, STMix::GAIN_PARAM + 0));
		addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(20.982, 41.451)), module, STMix::GAIN_PARAM + 1));
		addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(20.927, 64.318)), module, STMix::GAIN_PARAM + 2));
		addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(21.0, 87.124)), module, STMix::GAIN_PARAM + 3));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.308, 13.108)), module, STMix::LEFT_INPUT + 0));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.308, 36.175)), module, STMix::LEFT_INPUT + 1));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.3, 59.243)), module, STMix::LEFT_INPUT + 2));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.279, 82.311)), module, STMix::LEFT_INPUT + 3));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.287, 105.378)), module, STMix::LEFT_INPUT + 4));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.308, 23.108)), module, STMix::RIGHT_INPUT + 0));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.313, 46.354)), module, STMix::RIGHT_INPUT + 1));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.308, 69.237)), module, STMix::RIGHT_INPUT + 2));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.308, 92.132)), module, STMix::RIGHT_INPUT + 3));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.279, 115.379)), module, STMix::RIGHT_INPUT + 4));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(23.813, 105.422)), module, STMix::LEFT_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(23.792, 115.392)), module, STMix::RIGHT_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(16.8, 103.0)), module, STMix::LEFT_LED));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(16.8, 113.0)), module, STMix::RIGHT_LED));
	}
};


Model* modelSTMix = createModel<STMix, STMixWidget>("STMix");