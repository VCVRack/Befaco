#include "plugin.hpp"

using simd::float_4;

struct Percall : Module {
	enum ParamIds {
		ENUMS(VOL_PARAMS, 4),
		ENUMS(DECAY_PARAMS, 4),
		ENUMS(CHOKE_PARAMS, 2),
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(CH_INPUTS, 4),
		STRENGTH_INPUT,
		ENUMS(TRIG_INPUTS, 4),
		ENUMS(CV_INPUTS, 4),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(CH_OUTPUTS, 4),
		ENUMS(ENV_OUTPUTS, 4),
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(LEDS, 4),
		NUM_LIGHTS
	};

	ADEnvelope envs[4];

	float gains[4] = {};

	dsp::SchmittTrigger trigger[4];
	dsp::ClockDivider cvDivider;
	dsp::ClockDivider lightDivider;
	const int LAST_CHANNEL_ID = 3;

	const float attackTime = 1.5e-3;
	const float minDecayTime = 4.5e-3;
	const float maxDecayTime = 4.f;

	Percall() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		for (int i = 0; i < 4; i++) {
			configParam(VOL_PARAMS + i, 0.f, 1.f, 1.f, string::f("Channel %d level", i + 1), "%", 0, 100);
			configParam(DECAY_PARAMS + i, 0.f, 1.f, 0.f, string::f("Channel %d decay time", i + 1));
			configInput(CH_INPUTS + i, string::f("Channel %d", i + 1));
			configInput(TRIG_INPUTS + i, string::f("Channel %d trigger", i + 1));
			configInput(CV_INPUTS + i, string::f("Channel %d CV", i + 1));
			configOutput(ENV_OUTPUTS + i, string::f("Channel %d envelope", i + 1));

			envs[i].attackTime = attackTime;
			envs[i].attackShape = 0.5f;
			envs[i].decayShape = 2.0f;
		}

		configInput(STRENGTH_INPUT, string::f("Overall gain (also affects Env Outs)"));

		for (int i = 0; i < 2; i++) {
			configParam(CHOKE_PARAMS + i, 0.f, 1.f, 0.f, string::f("Choke %d to %d", 2 * i + 1, 2 * i + 2));
		}

		cvDivider.setDivision(16);
		lightDivider.setDivision(128);
	}

	void process(const ProcessArgs& args) override {

		float strength = 1.0f;
		if (inputs[STRENGTH_INPUT].isConnected()) {
			strength = std::sqrt(clamp(inputs[STRENGTH_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f));
		}

		// only calculate gains/decays every 16 samples
		if (cvDivider.process()) {
			for (int i = 0; i < 4; i++) {
				gains[i] = std::pow(params[VOL_PARAMS + i].getValue(), 2.f) * strength;

				float fallCv = inputs[CV_INPUTS + i].getVoltage() * 0.05f + params[DECAY_PARAMS + i].getValue();
				envs[i].decayTime = rescale(std::pow(clamp(fallCv, 0.f, 1.0f), 2.f), 0.f, 1.f, minDecayTime, maxDecayTime);
			}
		}

		float_4 mix[4] = {};
		int maxPolyphonyChannels = 1;

		// Mixer channels
		for (int i = 0; i < 4; i++) {

			if (trigger[i].process(rescale(inputs[TRIG_INPUTS + i].getVoltage(), 0.1f, 2.f, 0.f, 1.f))) {
				envs[i].trigger();
			}
			// if choke is enabled, and current channel is odd and left channel is in attack
			if ((i % 2) && params[CHOKE_PARAMS + i / 2].getValue() && envs[i - 1].stage == ADEnvelope::STAGE_ATTACK) {
				// TODO: is there a more graceful way to choke, e.g. rapid envelope?
				// TODO: this will just silence it instantly, maybe switch to STAGE_DECAY and modify fall time
				envs[i].stage = ADEnvelope::STAGE_OFF;
			}

			envs[i].process(args.sampleTime);

			int polyphonyChannels = 1;
			float_4 in[4] = {};
			bool inputIsConnected = inputs[CH_INPUTS + i].isConnected();
			bool inputIsNormed = !inputIsConnected && (i % 2) && inputs[CH_INPUTS + i - 1].isConnected();
			if ((inputIsConnected || inputIsNormed)) {
				int channelToReadFrom = inputIsNormed ? CH_INPUTS + i - 1 : CH_INPUTS + i;
				polyphonyChannels = inputs[channelToReadFrom].getChannels();

				// an input only counts towards the main output polyphony count if it's not taken out of the mix
				// (i.e. an output is patched in). the final input should always count towards polyphony count.
				if (i == CH_INPUTS_LAST || !outputs[CH_OUTPUTS + i].isConnected()) {
					maxPolyphonyChannels = std::max(maxPolyphonyChannels, polyphonyChannels);
				}

				// only process input audio if envelope is active
				if (envs[i].stage != ADEnvelope::STAGE_OFF) {
					float gain = gains[i] * envs[i].env;
					for (int c = 0; c < polyphonyChannels; c += 4) {
						in[c / 4] = inputs[channelToReadFrom].getVoltageSimd<float_4>(c) * gain;
					}
				}
			}

			if (i != LAST_CHANNEL_ID) {
				// if connected, output via the jack (and don't add to mix)
				if (outputs[CH_OUTPUTS + i].isConnected()) {
					outputs[CH_OUTPUTS + i].setChannels(polyphonyChannels);
					for (int c = 0; c < polyphonyChannels; c += 4) {
						outputs[CH_OUTPUTS + i].setVoltageSimd(in[c / 4], c);
					}
				}
				else {
					// else add to mix
					for (int c = 0; c < polyphonyChannels; c += 4) {
						mix[c / 4] += in[c / 4];
					}
				}
			}
			// otherwise if it is the final channel and it's wired in
			else if (outputs[CH_OUTPUTS + i].isConnected()) {

				outputs[CH_OUTPUTS + i].setChannels(maxPolyphonyChannels);

				// last channel must always go into mix
				for (int c = 0; c < polyphonyChannels; c += 4) {
					mix[c / 4] += in[c / 4];
				}

				for (int c = 0; c < maxPolyphonyChannels; c += 4) {
					outputs[CH_OUTPUTS + i].setVoltageSimd(mix[c / 4], c);
				}
			}

			// set env output
			if (outputs[ENV_OUTPUTS + i].isConnected()) {
				outputs[ENV_OUTPUTS + i].setVoltage(10.f * strength * envs[i].env);
			}
		}

		if (lightDivider.process()) {
			for (int i = 0; i < 4; i++) {
				lights[LEDS + i].setBrightness(envs[i].env);
			}
		}

	}
};


struct PercallWidget : ModuleWidget {
	PercallWidget(Percall* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/Percall.svg")));

		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<Knurlie>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(8.048, 41.265)), module, Percall::VOL_PARAMS + 0));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(22.879, 41.265)), module, Percall::VOL_PARAMS + 1));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(37.709, 41.265)), module, Percall::VOL_PARAMS + 2));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(52.54, 41.265)), module, Percall::VOL_PARAMS + 3));
		addParam(createParam<BefacoSlidePot>(mm2px(Vec(5.385, 53.912)), module, Percall::DECAY_PARAMS + 0));
		addParam(createParam<BefacoSlidePot>(mm2px(Vec(20.292, 53.912)), module, Percall::DECAY_PARAMS + 1));
		addParam(createParam<BefacoSlidePot>(mm2px(Vec(35.173, 53.912)), module, Percall::DECAY_PARAMS + 2));
		addParam(createParam<BefacoSlidePot>(mm2px(Vec(49.987, 53.912)), module, Percall::DECAY_PARAMS + 3));

		addParam(createParam<CKSS>(mm2px(Vec(13.365, 58.672)), module, Percall::CHOKE_PARAMS + 0));
		addParam(createParam<CKSS>(mm2px(Vec(42.993, 58.672)), module, Percall::CHOKE_PARAMS + 1));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(7.15, 12.905)), module, Percall::CH_INPUTS + 0));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(20.298, 12.905)), module, Percall::CH_INPUTS + 1));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(40.266, 12.905)), module, Percall::CH_INPUTS + 2));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(53.437, 12.905)), module, Percall::CH_INPUTS + 3));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(30.282, 18.221)), module, Percall::STRENGTH_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(7.15, 24.827)), module, Percall::TRIG_INPUTS + 0));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(18.488, 23.941)), module, Percall::TRIG_INPUTS + 1));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(42.171, 23.95)), module, Percall::TRIG_INPUTS + 2));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(53.437, 24.827)), module, Percall::TRIG_INPUTS + 3));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(5.037, 101.844)), module, Percall::CV_INPUTS + 0));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(15.159, 101.844)), module, Percall::CV_INPUTS + 1));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(25.28, 101.844)), module, Percall::CV_INPUTS + 2));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(35.402, 101.844)), module, Percall::CV_INPUTS + 3));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(45.524, 101.844)), module, Percall::CH_OUTPUTS + 0));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(55.645, 101.844)), module, Percall::CH_OUTPUTS + 1));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(45.524, 113.766)), module, Percall::CH_OUTPUTS + 2));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(55.645, 113.766)), module, Percall::CH_OUTPUTS + 3));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(5.037, 113.766)), module, Percall::ENV_OUTPUTS + 0));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(15.159, 113.766)), module, Percall::ENV_OUTPUTS + 1));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(25.28, 113.766)), module, Percall::ENV_OUTPUTS + 2));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(35.402, 113.766)), module, Percall::ENV_OUTPUTS + 3));

		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(8.107, 49.221)), module, Percall::LEDS + 0));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(22.934, 49.221)), module, Percall::LEDS + 1));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(37.762, 49.221)), module, Percall::LEDS + 2));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(52.589, 49.221)), module, Percall::LEDS + 3));
	}
};


Model* modelPercall = createModel<Percall, PercallWidget>("Percall");