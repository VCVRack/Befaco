#include "plugin.hpp"
#include "Common.hpp"
#include "ChowDSP.hpp"


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

	static const int UPSAMPLE = 8;
	chowdsp::Oversampling<UPSAMPLE> oversampler;
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

		const float kickFrequency = std::max(10.0f, freq + bendRange * bend * pitch.env);
		const float phaseInc = args.sampleTime * kickFrequency / UPSAMPLE;

		float shape = clamp(inputs[SHAPE_INPUT].getVoltage() / 2.f + params[SHAPE_PARAM].getValue() * 5.0f, 0.0f, 10.0f);
		shape = clamp(shape, 0.f, 5.0f) * 0.2f;
		shape *= 0.99f;
		const float shapeB = (1.0f - shape) / (1.0f + shape);
		const float shapeA = (4.0f * shape) / ((1.0f - shape) * (1.0f + shape));

		float* inputBuf = oversampler.getOSBuffer();
		for (int i = 0; i < UPSAMPLE; ++i) {
			phase += phaseInc;
			phase -= std::floor(phase);

			inputBuf[i] = sin2pi_pade_05_5_4(phase);
			inputBuf[i] = inputBuf[i] * (shapeA + shapeB) / ((std::abs(inputBuf[i]) * shapeA) + shapeB);
		}

		float out = volume.env * oversampler.downsample() * 5.0f * vcaGain;
		outputs[OUT_OUTPUT].setVoltage(out);

		lights[ENV_LIGHT].setBrightness(volume.env);
	}
};


struct KickallWidget : ModuleWidget {
	KickallWidget(Kickall* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Kickall.svg")));

		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<BefacoTinyKnobGrey>(mm2px(Vec(8.472, 28.97)), module, Kickall::TUNE_PARAM));
		addParam(createParamCentered<BefacoPush>(mm2px(Vec(22.409, 29.159)), module, Kickall::TRIGG_BUTTON_PARAM));
		addParam(createParamCentered<Davies1900hLargeGreyKnob>(mm2px(Vec(15.526, 49.292)), module, Kickall::SHAPE_PARAM));
		addParam(createParam<BefacoSlidePot>(mm2px(Vec(19.667, 63.897)), module, Kickall::DECAY_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(8.521, 71.803)), module, Kickall::TIME_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(8.521, 93.517)), module, Kickall::BEND_PARAM));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(15.501, 14.508)), module, Kickall::VOLUME_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(5.499, 14.536)), module, Kickall::TRIGG_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(25.525, 113.191)), module, Kickall::DECAY_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(5.499, 113.208)), module, Kickall::TUNE_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(15.485, 113.208)), module, Kickall::SHAPE_INPUT));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(25.525, 14.52)), module, Kickall::OUT_OUTPUT));

		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(15.535, 34.943)), module, Kickall::ENV_LIGHT));
	}
};


Model* modelKickall = createModel<Kickall, KickallWidget>("Kickall");