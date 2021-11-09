#include "plugin.hpp"

using simd::float_4;

struct STMix : Module {

	// NOTE: not including auxiliary channel
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
		ENUMS(LEFT_LED, 3),
		ENUMS(RIGHT_LED, 3),
		NUM_LIGHTS
	};

	STMix() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		for (int i = 0; i < numMixerChannels; ++i) {
			configParam(GAIN_PARAM + i, 0.f, 1.f, 0.f, string::f("Gain %d", i + 1));
			configInput(LEFT_INPUT + i, string::f("Channel %d left", i + 1));
			configInput(RIGHT_INPUT + i, string::f("Channel %d right", i + 1));
		}
		configInput(LEFT_INPUT + numMixerChannels, "Channel left (aux)");
		configInput(RIGHT_INPUT + numMixerChannels, "Channel right (aux)");
		configOutput(LEFT_OUTPUT, "Left");
		configOutput(RIGHT_OUTPUT, "Right");
	}

	void process(const ProcessArgs& args) override {
		float_4 out_left[4] = {};
		float_4 out_right[4] = {};

		int numActivePolyphonyEngines = 1;
		for (int i = 0; i < numMixerChannels + 1; ++i) {
			const int stereoPolyChannels = std::max(inputs[LEFT_INPUT + i].getChannels(),
			                                        inputs[RIGHT_INPUT + i].getChannels());
			numActivePolyphonyEngines = std::max(numActivePolyphonyEngines, stereoPolyChannels);
		}

		for (int i = 0; i < numMixerChannels + 1; ++i) {

			const float gain = (i < numMixerChannels) ? exponentialBipolar80Pade_5_4(params[GAIN_PARAM + i].getValue()) : 1.f;

			for (int c = 0; c < numActivePolyphonyEngines; c += 4) {
				const float_4 in_left = inputs[LEFT_INPUT + i].getNormalVoltageSimd<float_4>(0.f, c);
				const float_4 in_right = inputs[RIGHT_INPUT + i].getNormalVoltageSimd<float_4>(in_left, c);

				out_left[c / 4] += in_left * gain;
				out_right[c / 4] += in_right * gain;
			}
		}

		outputs[LEFT_OUTPUT].setChannels(numActivePolyphonyEngines);
		outputs[RIGHT_OUTPUT].setChannels(numActivePolyphonyEngines);

		for (int c = 0; c < numActivePolyphonyEngines; c += 4) {
			outputs[LEFT_OUTPUT].setVoltageSimd(out_left[c / 4], c);
			outputs[RIGHT_OUTPUT].setVoltageSimd(out_right[c / 4], c);
		}

		if (numActivePolyphonyEngines == 1) {
			lights[LEFT_LED + 0].setSmoothBrightness(outputs[LEFT_OUTPUT].getVoltage() / 5.f, args.sampleTime);
			lights[RIGHT_LED + 0].setSmoothBrightness(outputs[RIGHT_OUTPUT].getVoltage() / 5.f, args.sampleTime);
			lights[LEFT_LED + 1].setBrightness(0.f);
			lights[RIGHT_LED + 1].setBrightness(0.f);
			lights[LEFT_LED + 2].setBrightness(0.f);
			lights[RIGHT_LED + 2].setBrightness(0.f);
		}
		else {
			lights[LEFT_LED + 0].setBrightness(0.f);
			lights[RIGHT_LED + 0].setBrightness(0.f);
			lights[LEFT_LED + 1].setBrightness(0.f);
			lights[RIGHT_LED + 1].setBrightness(0.f);

			float b_left = 0.f;
			float b_right = 0.f;
			for (int c = 0; c < numActivePolyphonyEngines; c++) {
				b_left += std::pow(out_left[c / 4][c % 4], 2);
				b_right += std::pow(out_right[c / 4][c % 4], 2);
			}
			b_left = std::sqrt(b_left) / 5.f;
			b_right = std::sqrt(b_right) / 5.f;
			lights[LEFT_LED + 2].setSmoothBrightness(b_left, args.sampleTime);
			lights[RIGHT_LED + 2].setSmoothBrightness(b_right, args.sampleTime);
		}
	}
};


struct STMixWidget : ModuleWidget {
	STMixWidget(STMix* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/STMix.svg")));

		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(21.0, 18.141)), module, STMix::GAIN_PARAM + 0));
		addParam(createParamCentered<Davies1900hLightGreyKnob>(mm2px(Vec(21.0, 41.451)), module, STMix::GAIN_PARAM + 1));
		addParam(createParamCentered<Davies1900hDarkGreyKnob>(mm2px(Vec(21.0, 64.318)), module, STMix::GAIN_PARAM + 2));
		addParam(createParamCentered<Davies1900hDarkBlackAlt>(mm2px(Vec(21.0, 87.124)), module, STMix::GAIN_PARAM + 3));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.3, 13.108)), module, STMix::LEFT_INPUT + 0));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.3, 36.175)), module, STMix::LEFT_INPUT + 1));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.3, 59.243)), module, STMix::LEFT_INPUT + 2));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.3, 82.311)), module, STMix::LEFT_INPUT + 3));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.3, 105.378)), module, STMix::LEFT_INPUT + 4));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.3, 23.108)), module, STMix::RIGHT_INPUT + 0));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.3, 46.354)), module, STMix::RIGHT_INPUT + 1));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.3, 69.237)), module, STMix::RIGHT_INPUT + 2));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.3, 92.132)), module, STMix::RIGHT_INPUT + 3));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.3, 115.379)), module, STMix::RIGHT_INPUT + 4));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(23.8, 105.422)), module, STMix::LEFT_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(23.8, 115.392)), module, STMix::RIGHT_OUTPUT));

		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(16.8, 103.0)), module, STMix::LEFT_LED));
		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(16.8, 113.0)), module, STMix::RIGHT_LED));
	}
};


Model* modelSTMix = createModel<STMix, STMixWidget>("STMix");