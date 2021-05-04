#include "plugin.hpp"
#include "Common.hpp"


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

	float strength = 1.0f;
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
			configParam(VOL_PARAMS + i, 0.f, 1.f, 1.f, "Ch " + std::to_string(i + 1) + " level", "%", 0, 100);
			configParam(DECAY_PARAMS + i, 0.f, 1.f, 0.f, "Ch " + std::to_string(i + 1) + " decay time");
			envs[i].attackTime = attackTime;
			envs[i].attackShape = 0.5f;
			envs[i].decayShape = 3.0f;

		}
		for (int i = 0; i < 2; i++) {
			std::string description = "Choke " + std::to_string(2 * i + 1) + " to " + std::to_string(2 * i + 2);
			configParam(CHOKE_PARAMS + i, 0.f, 1.f, 0.f, description);
		}

		cvDivider.setDivision(16);
		lightDivider.setDivision(128);
	}

	void process(const ProcessArgs& args) override {

		strength = 1.0f;
		if (inputs[STRENGTH_INPUT].isConnected()) {
			strength = clamp(inputs[STRENGTH_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);
		}

		// only calculate gains/decays every 16 samples
		if (cvDivider.process()) {
			for (int i = 0; i < 4; i++) {
				gains[i] = std::pow(params[VOL_PARAMS + i].getValue(), 2.f) * strength;

				float fallCv = inputs[CV_INPUTS + i].getVoltage() + params[DECAY_PARAMS + i].getValue() * 10.f;
				envs[i].decayTime = minDecayTime * std::pow(2.0, clamp(fallCv, 0.0f, 10.0f));
			}
		}

		simd::float_4 mix[4] = {};
		int maxChannels = 1;

		// Mixer channels
		for (int i = 0; i < 4; i++) {

			if (trigger[i].process(rescale(inputs[TRIG_INPUTS + i].getVoltage(), 0.1f, 2.f, 0.f, 1.f))) {
				envs[i].stage = ADEnvelope::STAGE_ATTACK;
			}
			// if choke is enabled, and current channel is odd and left channel is in attack
			if ((i % 2) && params[CHOKE_PARAMS + i / 2].getValue() && envs[i - 1].stage == ADEnvelope::STAGE_ATTACK) {
				// TODO: is there a more graceful way to choke, e.g. rapid envelope?
				// TODO: this will just silence it instantly, maybe switch to STAGE_DECAY and modify fall time
				envs[i].stage = ADEnvelope::STAGE_OFF;
			}

			envs[i].process(args.sampleTime);

			int channels = 1;
			simd::float_4 in[4] = {};
			bool inputIsConnected = inputs[CH_INPUTS + i].isConnected();
			bool inputIsNormed = !inputIsConnected && (i % 2) && inputs[CH_INPUTS + i - 1].isConnected();
			if ((inputIsConnected || inputIsNormed)) {
				int channel_to_read_from = inputIsNormed ? CH_INPUTS + i - 1 : CH_INPUTS + i;
				channels = inputs[channel_to_read_from].getChannels();
				maxChannels = std::max(maxChannels, channels);

				// only process input audio if envelope is active
				if (envs[i].stage != ADEnvelope::STAGE_OFF) {
					float gain = gains[i] * envs[i].env;
					for (int c = 0; c < channels; c += 4) {
						in[c / 4] = simd::float_4::load(inputs[channel_to_read_from].getVoltages(c)) * gain;
					}
				}
			}

			if (i != LAST_CHANNEL_ID) {
				// if connected, output via the jack (and don't add to mix)
				if (outputs[CH_OUTPUTS + i].isConnected()) {
					outputs[CH_OUTPUTS + i].setChannels(channels);
					for (int c = 0; c < channels; c += 4) {
						in[c / 4].store(outputs[CH_OUTPUTS + i].getVoltages(c));
					}
				}
				else {
					// else add to mix
					for (int c = 0; c < channels; c += 4) {
						mix[c / 4] += in[c / 4];
					}
				}
			}
			// otherwise if it is the final channel and it's wired in
			else if (outputs[CH_OUTPUTS + i].isConnected()) {

				outputs[CH_OUTPUTS + i].setChannels(maxChannels);

				// last channel must always go into mix
				for (int c = 0; c < channels; c += 4) {
					mix[c / 4] += in[c / 4];
				}

				for (int c = 0; c < maxChannels; c += 4) {
					mix[c / 4].store(outputs[CH_OUTPUTS + i].getVoltages(c));
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
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Percall.svg")));

		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<Knurlie>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(8.003, 41.196)), module, Percall::VOL_PARAMS + 0));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(22.829, 41.196)), module, Percall::VOL_PARAMS + 1));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(37.655, 41.196)), module, Percall::VOL_PARAMS + 2));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(52.481, 41.196)), module, Percall::VOL_PARAMS + 3));
		addParam(createParam<BefacoSlidePot>(mm2px(Vec(5.385, 52.476)), module, Percall::DECAY_PARAMS + 0));
		addParam(createParam<BefacoSlidePot>(mm2px(Vec(20.728, 52.476)), module, Percall::DECAY_PARAMS + 1));
		addParam(createParam<BefacoSlidePot>(mm2px(Vec(35.543, 52.476)), module, Percall::DECAY_PARAMS + 2));
		addParam(createParam<BefacoSlidePot>(mm2px(Vec(50.357, 52.476)), module, Percall::DECAY_PARAMS + 3));
		addParam(createParam<CKSS>(mm2px(Vec(13.365, 58.672)), module, Percall::CHOKE_PARAMS + 0));
		addParam(createParam<CKSS>(mm2px(Vec(42.993, 58.672)), module, Percall::CHOKE_PARAMS + 1));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(7.173, 12.894)), module, Percall::CH_INPUTS + 0));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(20.335, 12.894)), module, Percall::CH_INPUTS + 1));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(40.347, 12.894)), module, Percall::CH_INPUTS + 2));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(53.492, 12.894)), module, Percall::CH_INPUTS + 3));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(30.341, 18.236)), module, Percall::STRENGTH_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(7.173, 24.834)), module, Percall::TRIG_INPUTS + 0));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(18.547, 23.904)), module, Percall::TRIG_INPUTS + 1));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(42.218, 23.904)), module, Percall::TRIG_INPUTS + 2));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(53.453, 24.834)), module, Percall::TRIG_INPUTS + 3));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(5.093, 101.799)), module, Percall::CV_INPUTS + 0));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(15.22, 101.799)), module, Percall::CV_INPUTS + 1));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(25.347, 101.799)), module, Percall::CV_INPUTS + 2));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(35.474, 101.799)), module, Percall::CV_INPUTS + 3));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(45.541, 101.699)), module, Percall::CH_OUTPUTS + 0));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(55.624, 101.699)), module, Percall::CH_OUTPUTS + 1));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(45.541, 113.696)), module, Percall::CH_OUTPUTS + 2));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(55.624, 113.696)), module, Percall::CH_OUTPUTS + 3));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(5.093, 113.74)), module, Percall::ENV_OUTPUTS + 0));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(15.22, 113.74)), module, Percall::ENV_OUTPUTS + 1));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(25.347, 113.74)), module, Percall::ENV_OUTPUTS + 2));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(35.474, 113.74)), module, Percall::ENV_OUTPUTS + 3));

		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(8.107, 49.221)), module, Percall::LEDS + 0));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(22.934, 49.221)), module, Percall::LEDS + 1));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(37.762, 49.221)), module, Percall::LEDS + 2));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(52.589, 49.221)), module, Percall::LEDS + 3));
	}
};


Model* modelPercall = createModel<Percall, PercallWidget>("Percall");