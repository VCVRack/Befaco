#include "plugin.hpp"
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

	static constexpr float FREQ_A0 = 27.5f;
	static constexpr float FREQ_B2 = 123.471f;
	static constexpr float minVolumeDecay = 0.075f;
	static constexpr float maxVolumeDecay = 4.f;
	static constexpr float minPitchDecay = 0.0075f;
	static constexpr float maxPitchDecay = 1.f;
	static constexpr float bendRange = 10000;
	float phase = 0.f;
	ADEnvelope volume;
	ADEnvelope pitch;

	dsp::SchmittTrigger gateTrigger;
	dsp::BooleanTrigger buttonTrigger;

	static const int UPSAMPLE = 8;
	chowdsp::Oversampling<UPSAMPLE> oversampler;

	Kickall() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		// TODO: review this mapping, using displayBase multiplier seems more normal
		configParam(TUNE_PARAM, FREQ_A0, FREQ_B2, 0.5 * (FREQ_A0 + FREQ_B2), "Tune", "Hz");
		configButton(TRIGG_BUTTON_PARAM, "Manual trigger");
		configParam(SHAPE_PARAM, 0.f, 1.f, 0.f, "Wave shape");
		configParam(DECAY_PARAM, 0.f, 1.f, 0.01f, "VCA Envelope decay time");
		configParam(TIME_PARAM, 0.f, 1.0f, 0.f, "Pitch envelope decay time");
		configParam(BEND_PARAM, 0.f, 1.f, 0.f, "Pitch envelope attenuator");

		volume.attackTime = 0.01;
		volume.attackShape = 0.5;
		volume.decayShape = 3.0;
		pitch.attackTime = 0.00165;
		pitch.decayShape = 3.0;

		configInput(TRIGG_INPUT, "Trigger");
		configInput(VOLUME_INPUT, "Gain");
		configInput(TUNE_INPUT, "Tune (V/Oct)");
		configInput(SHAPE_INPUT, "Shape CV");
		configInput(DECAY_INPUT, "Decay CV");

		configOutput(OUT_OUTPUT, "Kick");
		configLight(ENV_LIGHT, "Volume envelope");

		// calculate up/downsampling rates
		onSampleRateChange();
	}

	void onSampleRateChange() override {
		oversampler.reset(APP->engine->getSampleRate());
	}

	void process(const ProcessArgs& args) override {
		// TODO: check values
		const bool risingEdgeGate = gateTrigger.process(inputs[TRIGG_INPUT].getVoltage() / 2.0f, 0.1, 2.0);
		const bool buttonTriggered = buttonTrigger.process(params[TRIGG_BUTTON_PARAM].getValue());
		// can be triggered by either rising edge on trigger in, or a button press
		if (risingEdgeGate || buttonTriggered) {
			volume.trigger();
			pitch.trigger();
		}

		const float vcaGain = clamp(inputs[VOLUME_INPUT].getNormalVoltage(10.f) / 10.f, 0.f, 1.0f);

		// pitch envelope
		const float bend = bendRange * std::pow(params[BEND_PARAM].getValue(), 3.0);
		pitch.decayTime = rescale(params[TIME_PARAM].getValue(), 0.f, 1.0f, minPitchDecay, maxPitchDecay);
		pitch.process(args.sampleTime);

		// volume envelope
		const float volumeDecay = minVolumeDecay * std::pow(2.f, params[DECAY_PARAM].getValue() * std::log2(maxVolumeDecay / minVolumeDecay));
		volume.decayTime = clamp(volumeDecay + inputs[DECAY_INPUT].getVoltage() * 0.1f, 0.01, 10.0);
		volume.process(args.sampleTime);

		float freq = params[TUNE_PARAM].getValue();
		freq *= std::pow(2.f, inputs[TUNE_INPUT].getVoltage());

		const float kickFrequency = std::max(10.0f, freq + bend * pitch.env);
		const float phaseInc = clamp(args.sampleTime * kickFrequency / UPSAMPLE, 1e-6, 0.35f);

		const float shape = clamp(inputs[SHAPE_INPUT].getVoltage() / 10.f + params[SHAPE_PARAM].getValue(), 0.0f, 1.0f) * 0.99f;
		const float shapeB = (1.0f - shape) / (1.0f + shape);
		const float shapeA = (4.0f * shape) / ((1.0f - shape) * (1.0f + shape));

		float* inputBuf = oversampler.getOSBuffer();
		for (int i = 0; i < UPSAMPLE; ++i) {
			phase += phaseInc;
			phase -= std::floor(phase);

			inputBuf[i] = sin2pi_pade_05_5_4(phase);
			inputBuf[i] = inputBuf[i] * (shapeA + shapeB) / ((std::abs(inputBuf[i]) * shapeA) + shapeB);
		}

		const float out = volume.env * oversampler.downsample() * 5.0f * vcaGain;
		outputs[OUT_OUTPUT].setVoltage(out);

		lights[ENV_LIGHT].setBrightness(volume.env);
	}
};


struct KickallWidget : ModuleWidget {
	KickallWidget(Kickall* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/Kickall.svg")));

		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<BefacoTinyKnobDarkGrey>(mm2px(Vec(8.472, 28.97)), module, Kickall::TUNE_PARAM));
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
