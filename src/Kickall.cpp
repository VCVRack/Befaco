#include "plugin.hpp"
#include "Common.hpp"

struct ADEnvelope {

	enum Stage {
		STAGE_OFF,
		STAGE_ATTACK,
		STAGE_DECAY
	};

	Stage stage = STAGE_OFF;
	float env = 0.f;
	float attackTime = 0.1, decayTime = 0.1;

	ADEnvelope() { };

	void process(const float& sampleTime) {

		if (stage == STAGE_OFF) {
			env = 0.0f;
		}
		else if (stage == STAGE_ATTACK) {
			env += sampleTime / attackTime;
		}
		else if (stage == STAGE_DECAY) {
			env -= sampleTime / decayTime;
		}

		if (env >= 1.0f) {
			stage = STAGE_DECAY;
			env = 1.0f;
		}
		else if (env <= 0.0f) {
			stage = STAGE_OFF;
			env = 0.0f;
		}
	}
};

struct Kickall : Module {
	enum ParamIds {
		TUNE_PARAM,
		TRIGG_BUTTON_PARAM,
		SHAPE_PARAM,
		DECAY_PARAM,
		TIME_PARAM,
		BEND_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		TRIGG_INPUT,
		VOLUME_INPUT,
		TUNE_INPUT,
		SHAPE_INPUT,
		DECAY_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENV_LIGHT,
		NUM_LIGHTS
	};

	Kickall() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		// TODO: review this mapping, using displayBase multiplier seems more normal
		configParam(TUNE_PARAM, FREQ_A0, FREQ_B2, 0.5 * (FREQ_A0 + FREQ_B2), "Tune", "Hz");
		configParam(TRIGG_BUTTON_PARAM, 0.f, 1.f, 0.f, "Manual trigger");
		configParam(SHAPE_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DECAY_PARAM, 0.01f, 1.f, 0.01f, "VCA Envelope decay time");
		configParam(TIME_PARAM, 0.0079f, 0.076f, 0.0079f, "Pitch envelope decay time", "ms", 0.0f, 1000.f);
		configParam(BEND_PARAM, 0.f, 1.f, 0.f, "Pitch envelope attenuator");

		volume.attackTime = 0.00165;
		pitch.attackTime = 0.00165;

		// calculate up/downsampling rates
		onSampleRateChange();
	}

	void onSampleRateChange() override {
		// SampleRateConverter needs integer value
		int sampleRate = APP->engine->getSampleRate();
		oversampler.reset(sampleRate);
	}

	float bendRange = 600;
	float phase = 0.f;
	ADEnvelope volume;
	ADEnvelope pitch;

	dsp::SchmittTrigger trigger;

	static constexpr float FREQ_A0 = 27.5f;
	static constexpr float FREQ_B2 = 123.471f;

	static const int UPSAMPLE = 4;
	Oversampling<UPSAMPLE> oversampler;
	float shaperBuf[UPSAMPLE];

	void process(const ProcessArgs& args) override {

		// TODO: check values
		if (trigger.process(inputs[TRIGG_INPUT].getVoltage() / 2.0f + params[TRIGG_BUTTON_PARAM].getValue() * 10.0)) {
			volume.stage = ADEnvelope::STAGE_ATTACK;
			pitch.stage = ADEnvelope::STAGE_ATTACK;
		}

		float vcaGain = clamp(inputs[VOLUME_INPUT].getNormalVoltage(10.f) / 10.f, 0.f, 1.0f);

		// pitch envelope
		float bend = params[BEND_PARAM].getValue();
		pitch.decayTime = params[TIME_PARAM].getValue();
		pitch.process(args.sampleTime);

		// volume envelope TODO: wire in decay CV
		volume.decayTime = params[DECAY_PARAM].getValue();
		volume.process(args.sampleTime);

		float freq = params[TUNE_PARAM].getValue();
		freq *= std::pow(2, inputs[TUNE_INPUT].getVoltage());

		//float kickFrequency = std::max(50.0f, params[TUNE_PARAM].getValue() + bendRange * bend * pitch.env);
		float kickFrequency = std::max(50.0f, freq + bendRange * bend * pitch.env);

		phase += args.sampleTime * kickFrequency;
		if (phase > 1.0f) {
			phase -= 1.0f;
		}

		// TODO: faster approximation
		float wave = std::sin(2.0 * M_PI * phase);

		oversampler.upsample(wave);
		float* inputBuf = oversampler.getOSBuffer();

		float shape = clamp(inputs[SHAPE_INPUT].getVoltage() / 2.f + params[SHAPE_PARAM].getValue() * 5.0f, 0.0f, 10.0f);
		shape = clamp(shape, 0.f, 5.0f) * 0.2f;
		shape *= 0.99f;
		const float shapeB = (1.0f - shape) / (1.0f + shape);
		const float shapeA = (4.0f * shape) / ((1.0f - shape) * (1.0f + shape));

		for (int i = 0; i < UPSAMPLE; ++i) {
			inputBuf[i] = inputBuf[i] * (shapeA + shapeB) / ((std::abs(inputBuf[i]) * shapeA) + shapeB);
		}

		float out = volume.env * oversampler.downsample() * 5.0f * vcaGain;
		outputs[OUT_OUTPUT].setVoltage(out);

		lights[ENV_LIGHT].setBrightness(volume.stage);
	}
};


struct KickallWidget : ModuleWidget {
	KickallWidget(Kickall* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Kickall.svg")));

		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<BefacoTinyKnobGrey>(mm2px(Vec(8.76, 29.068)), module, Kickall::TUNE_PARAM));
		addParam(createParamCentered<BefacoPush>(mm2px(Vec(22.651, 29.068)), module, Kickall::TRIGG_BUTTON_PARAM));
		addParam(createParamCentered<Davies1900hLargeGreyKnob>(mm2px(Vec(15.781, 49.278)), module, Kickall::SHAPE_PARAM));
		addParam(createParam<BefacoSlidePot>(mm2px(Vec(19.913, 64.004)), module, Kickall::DECAY_PARAM));
		addParam(createParamCentered<BefacoTinyKnobWhite>(mm2px(Vec(8.977, 71.626)), module, Kickall::TIME_PARAM));
		addParam(createParamCentered<BefacoTinyKnobWhite>(mm2px(Vec(8.977, 93.549)), module, Kickall::BEND_PARAM));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(5.715, 14.651)), module, Kickall::TRIGG_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(15.748, 14.651)), module, Kickall::VOLUME_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(5.697, 113.286)), module, Kickall::TUNE_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(15.794, 113.286)), module, Kickall::SHAPE_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(25.891, 113.286)), module, Kickall::DECAY_INPUT));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(25.781, 14.651)), module, Kickall::OUT_OUTPUT));

		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(15.723, 34.971)), module, Kickall::ENV_LIGHT));
	}
};


Model* modelKickall = createModel<Kickall, KickallWidget>("Kickall");