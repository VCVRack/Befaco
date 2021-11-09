#include "plugin.hpp"


struct BefacoADSREnvelope {

	enum Stage {
		STAGE_OFF,
		STAGE_ATTACK,
		STAGE_DECAY,
		STAGE_SUSTAIN,
		STAGE_RELEASE
	};

	Stage stage = STAGE_OFF;
	float env = 0.f;
	float releaseValue;
	float timeInCurrentStage = 0.f;
	float attackTime = 0.1, decayTime = 0.1, releaseTime = 0.1;
	float attackShape = 1.0, decayShape = 1.0, releaseShape = 1.0;
	float sustainLevel;

	BefacoADSREnvelope() { };

	void retrigger() {
		stage = STAGE_ATTACK;
		// get the linear value of the envelope
		timeInCurrentStage = attackTime * std::pow(env, 1.0f / attackShape);
	}

	void processTransitionsGateMode(const bool& gateHeld) {
		if (gateHeld) {
			// calculate stage transitions
			switch (stage) {
				case  STAGE_OFF: {
					env = 0.0f;
					timeInCurrentStage = 0.f;
					stage = STAGE_ATTACK;
					break;
				}
				case STAGE_ATTACK: {
					if (env >= 1.f) {
						timeInCurrentStage = 0.f;
						stage = STAGE_DECAY;
					}
					break;
				}
				case STAGE_DECAY: {
					if (timeInCurrentStage >= decayTime) {
						timeInCurrentStage = 0.f;
						stage = STAGE_SUSTAIN;
					}
					break;
				}
				case STAGE_SUSTAIN: {
					break;
				}
				case STAGE_RELEASE: {
					stage = STAGE_ATTACK;
					timeInCurrentStage = attackTime * env;
					break;
				}
			}
		}
		else {
			if (stage == STAGE_ATTACK || stage == STAGE_DECAY || stage == STAGE_SUSTAIN) {
				timeInCurrentStage = 0.f;
				stage = STAGE_RELEASE;
				releaseValue = env;
			}
			else if (stage == STAGE_RELEASE) {
				if (timeInCurrentStage >= releaseTime) {
					stage = STAGE_OFF;
					timeInCurrentStage = 0.f;
				}
			}
		}
	}

	void processTransitionsTriggerMode(const bool& gateHeld) {

		// calculate stage transitions
		switch (stage) {
			case STAGE_ATTACK: {
				if (env >= 1.f) {
					timeInCurrentStage = 0.f;
					if (gateHeld) {
						stage = STAGE_DECAY;
					}
					else {
						stage = STAGE_RELEASE;
						releaseValue = 1.f;
					}
				}
				break;
			}
			case STAGE_DECAY: {
				if (timeInCurrentStage >= decayTime) {
					timeInCurrentStage = 0.f;
					if (gateHeld) {
						stage = STAGE_SUSTAIN;
					}
					else {
						stage = STAGE_RELEASE;
						releaseValue = env;
					}
				}
				break;
			}
			case STAGE_OFF:
			case STAGE_RELEASE:
			case STAGE_SUSTAIN: {
				break;
			}
		}

		if (!gateHeld) {
			if (stage == STAGE_DECAY || stage == STAGE_SUSTAIN) {
				timeInCurrentStage = 0.f;
				stage = STAGE_RELEASE;
				releaseValue = env;
			}
			else if (stage == STAGE_RELEASE) {
				if (timeInCurrentStage >= releaseTime) {
					stage = STAGE_OFF;
					timeInCurrentStage = 0.f;
				}
			}
		}
	}

	void evolveEnvelope(const float& sampleTime) {
		switch (stage) {
			case  STAGE_OFF: {
				env = 0.0f;
				break;
			}
			case STAGE_ATTACK: {
				timeInCurrentStage += sampleTime;
				env = std::min(timeInCurrentStage / attackTime, 1.f);
				env = std::pow(env, attackShape);
				break;
			}
			case STAGE_DECAY: {
				timeInCurrentStage += sampleTime;
				env = std::pow(1.f - std::min(1.f, timeInCurrentStage / decayTime), decayShape);
				env = sustainLevel + (1.f - sustainLevel) * env;
				break;
			}
			case STAGE_SUSTAIN: {
				env = sustainLevel;
				break;
			}
			case STAGE_RELEASE: {
				timeInCurrentStage += sampleTime;
				env = std::min(1.0f, timeInCurrentStage / releaseTime);
				env = releaseValue * std::pow(1.0f - env, releaseShape);
				break;
			}
		}
	}

	void process(const float& sampleTime, const bool& gateHeld, const bool& triggerMode) {

		if (triggerMode) {
			processTransitionsTriggerMode(gateHeld);
		}
		else {
			processTransitionsGateMode(gateHeld);
		}

		evolveEnvelope(sampleTime);
	}
};

struct ADSR : Module {
	enum ParamIds {
		TRIGG_GATE_TOGGLE_PARAM,
		MANUAL_TRIGGER_PARAM,
		SHAPE_PARAM,
		ATTACK_PARAM,
		DECAY_PARAM,
		SUSTAIN_PARAM,
		RELEASE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		TRIGGER_INPUT,
		CV_ATTACK_INPUT,
		CV_DECAY_INPUT,
		CV_SUSTAIN_INPUT,
		CV_RELEASE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_OUTPUT,
		STAGE_ATTACK_OUTPUT,
		STAGE_DECAY_OUTPUT,
		STAGE_SUSTAIN_OUTPUT,
		STAGE_RELEASE_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		LED_LIGHT,
		LED_ATTACK_LIGHT,
		LED_DECAY_LIGHT,
		LED_SUSTAIN_LIGHT,
		LED_RELEASE_LIGHT,
		NUM_LIGHTS
	};
	enum EnvelopeMode {
		GATE_MODE,
		TRIGGER_MODE
	};

	BefacoADSREnvelope envelope;
	dsp::SchmittTrigger gateTrigger;
	dsp::ClockDivider cvDivider;
	float shape;

	static constexpr float minStageTime = 0.003f;  // in seconds
	static constexpr float maxStageTime = 10.f;  // in seconds

	// given a value from the slider and/or cv (rescaled to range 0 to 1), transform into the appropriate time in seconds
	static float convertCVToTimeInSeconds(float cv) {		
		return minStageTime * std::pow(maxStageTime / minStageTime, cv);
	}

	ADSR() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configSwitch(TRIGG_GATE_TOGGLE_PARAM, GATE_MODE, TRIGGER_MODE, GATE_MODE, "Mode", {"Gate", "Trigger"});
		configButton(MANUAL_TRIGGER_PARAM, "Trigger envelope");
		configParam(SHAPE_PARAM, 0.f, 1.f, 0.f, "Envelope shape");

		configParam(ATTACK_PARAM, 0.f, 1.f, 0.f, "Attack time", "s", maxStageTime / minStageTime, minStageTime);
		configParam(DECAY_PARAM, 0.f, 1.f, 0.f, "Decay time", "s", maxStageTime / minStageTime, minStageTime);
		configParam(SUSTAIN_PARAM, 0.f, 1.f, 0.f, "Sustain level", "%", 0.f, 100.f);
		configParam(RELEASE_PARAM, 0.f, 1.f, 0.f, "Release time", "s", maxStageTime / minStageTime, minStageTime);

		configInput(TRIGGER_INPUT, "Trigger");
		configInput(CV_ATTACK_INPUT, "Attack CV");
		configInput(CV_DECAY_INPUT, "Decay CV");
		configInput(CV_SUSTAIN_INPUT, "Sustain CV");
		configInput(CV_RELEASE_INPUT, "Release CV");

		configOutput(OUT_OUTPUT, "Envelope");
		configOutput(STAGE_ATTACK_OUTPUT, "Attack stage");
		configOutput(STAGE_DECAY_OUTPUT, "Decay stage");
		configOutput(STAGE_SUSTAIN_OUTPUT, "Sustain stage");
		configOutput(STAGE_RELEASE_OUTPUT, "Release stage");
		
		cvDivider.setDivision(16);
	}

	void process(const ProcessArgs& args) override {


		if (cvDivider.process()) {
			shape = params[SHAPE_PARAM].getValue();
			envelope.decayShape = 1.f + shape;
			envelope.attackShape = 1.f - shape / 2.f;
			envelope.releaseShape = 1.f + shape;

			const float attackCV =  clamp(params[ATTACK_PARAM].getValue() + inputs[CV_ATTACK_INPUT].getVoltage() / 10.f, 0.f, 1.f);
			envelope.attackTime = convertCVToTimeInSeconds(attackCV);

			const float decayCV =  clamp(params[DECAY_PARAM].getValue() + inputs[CV_DECAY_INPUT].getVoltage() / 10.f, 0.f, 1.f);
			envelope.decayTime = convertCVToTimeInSeconds(decayCV);

			const float sustainCV =  clamp(params[SUSTAIN_PARAM].getValue() + inputs[CV_SUSTAIN_INPUT].getVoltage() / 10.f, 0.f, 1.f);
			envelope.sustainLevel = sustainCV;

			const float releaseCV =  clamp(params[RELEASE_PARAM].getValue() + inputs[CV_RELEASE_INPUT].getVoltage() / 10.f, 0.f, 1.f);
			envelope.releaseTime = convertCVToTimeInSeconds(releaseCV);
		}

		const bool triggered = gateTrigger.process(rescale(params[MANUAL_TRIGGER_PARAM].getValue() * 10.f + inputs[TRIGGER_INPUT].getVoltage(), 0.1f, 2.f, 0.f, 1.f));
		const bool gateOn = gateTrigger.isHigh() || params[MANUAL_TRIGGER_PARAM].getValue();
		const bool triggerMode = params[TRIGG_GATE_TOGGLE_PARAM].getValue() == 1;

		if (triggerMode) {
			if (triggered) {
				envelope.retrigger();
			}
		}

		envelope.process(args.sampleTime, gateOn, triggerMode);

		outputs[OUT_OUTPUT].setVoltage(envelope.env * 10.f);

		outputs[STAGE_ATTACK_OUTPUT].setVoltage(10.f * (envelope.stage == BefacoADSREnvelope::STAGE_ATTACK));
		outputs[STAGE_DECAY_OUTPUT].setVoltage(10.f * (envelope.stage == BefacoADSREnvelope::STAGE_DECAY));
		outputs[STAGE_SUSTAIN_OUTPUT].setVoltage(10.f * (envelope.stage == BefacoADSREnvelope::STAGE_SUSTAIN));
		outputs[STAGE_RELEASE_OUTPUT].setVoltage(10.f * (envelope.stage == BefacoADSREnvelope::STAGE_RELEASE));

		lights[LED_ATTACK_LIGHT].setBrightness((envelope.stage == BefacoADSREnvelope::STAGE_ATTACK));
		lights[LED_DECAY_LIGHT].setBrightness((envelope.stage == BefacoADSREnvelope::STAGE_DECAY));
		lights[LED_SUSTAIN_LIGHT].setBrightness((envelope.stage == BefacoADSREnvelope::STAGE_SUSTAIN));
		lights[LED_RELEASE_LIGHT].setBrightness((envelope.stage == BefacoADSREnvelope::STAGE_RELEASE));
		lights[LED_LIGHT].setBrightness((float) gateOn);
	}
};


struct ADSRWidget : ModuleWidget {
	ADSRWidget(ADSR* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/ADSR.svg")));

		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<Knurlie>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<BefacoSwitch>(mm2px(Vec(20.263, 17.128)), module, ADSR::TRIGG_GATE_TOGGLE_PARAM));
		addParam(createParamCentered<BefacoPush>(mm2px(Vec(11.581, 32.473)), module, ADSR::MANUAL_TRIGGER_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(29.063, 32.573)), module, ADSR::SHAPE_PARAM));
		addParam(createParam<BefacoSlidePot>(mm2px(Vec(2.294, 45.632)), module, ADSR::ATTACK_PARAM));
		addParam(createParam<BefacoSlidePot>(mm2px(Vec(12.422, 45.632)), module, ADSR::DECAY_PARAM));
		addParam(createParam<BefacoSlidePot>(mm2px(Vec(22.551, 45.632)), module, ADSR::SUSTAIN_PARAM));
		addParam(createParam<BefacoSlidePot>(mm2px(Vec(32.68, 45.632)), module, ADSR::RELEASE_PARAM));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.841, 15.5)), module, ADSR::TRIGGER_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(5.022, 113.506)), module, ADSR::CV_ATTACK_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(15.195, 113.506)), module, ADSR::CV_DECAY_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(25.368, 113.506)), module, ADSR::CV_SUSTAIN_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(35.541, 113.506)), module, ADSR::CV_RELEASE_INPUT));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(33.721, 15.479)), module, ADSR::OUT_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(5.022, 100.858)), module, ADSR::STAGE_ATTACK_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(15.195, 100.858)), module, ADSR::STAGE_DECAY_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(25.368, 100.858)), module, ADSR::STAGE_SUSTAIN_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(35.541, 100.858)), module, ADSR::STAGE_RELEASE_OUTPUT));

		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(20.254, 40.864)), module, ADSR::LED_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(5.001, 92.893)), module, ADSR::LED_ATTACK_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(15.174, 92.893)), module, ADSR::LED_DECAY_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(25.347, 92.893)), module, ADSR::LED_SUSTAIN_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(35.52, 92.893)), module, ADSR::LED_RELEASE_LIGHT));
	}
};


Model* modelADSR = createModel<ADSR, ADSRWidget>("ADSR");