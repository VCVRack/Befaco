#include "plugin.hpp"

#define MAX_REPETITIONS 32                  /// max number of repetitions
#define TRIGGER_TIME 0.001

// a tempo/clock calculator that responds to pings - this sets the base tempo, multiplication/division of 
// this tempo occurs in the BurstEngine
struct PingableClock {

	dsp::Timer timer;                   // time the gap between pings
	dsp::PulseGenerator clockTimer;     // counts down from tempo length to zero
	dsp::BooleanTrigger clockExpiry;    // checks for when the clock timer runs out

	float pingDuration = 0.5f;          // used for calculating and updating tempo (default 2Hz / 120 bpm)
	float tempo = 0.5f;                 // actual current tempo of clock

	PingableClock() {
		clockTimer.trigger(tempo);
	}

	void process(bool pingRecieved, float sampleTime) {
		timer.process(sampleTime);

		bool clockRestarted = false;

		if (pingRecieved) {

			bool tempoShouldBeUpdated = true;
			float duration = timer.getTime();

			// if the ping was unusually different to last time
			bool outlier = duration > (pingDuration * 2) || duration < (pingDuration / 2);
			// if there is a previous estimate of tempo, but it's an outlier
			if ((pingDuration && outlier)) {
				// don't calculate tempo from this; prime so future pings will update
				tempoShouldBeUpdated = false;
				pingDuration = 0;
			}
			else {
				pingDuration = duration;
			}
			timer.reset();

			if (tempoShouldBeUpdated) {
				// if the tempo should be updated, do so
				tempo = pingDuration;
				clockRestarted = true;
			}
		}

		// we restart the clock if a) a new valid ping arrived OR b) the current clock expired
		clockRestarted = clockExpiry.process(!clockTimer.process(sampleTime)) || clockRestarted;
		if (clockRestarted) {
			clockTimer.reset();
			clockTimer.trigger(tempo);
		}
	}

	bool isTempoOutHigh() {
		// give a 1ms pulse as tempo out
		return clockTimer.remaining > tempo - TRIGGER_TIME;
	}
};

// engine that generates a burst when triggered
struct BurstEngine {

	dsp::PulseGenerator eocOutput;      // for generating EOC trigger
	dsp::PulseGenerator burstOutput;    // for generating triggers for each occurance of the burst
	dsp::Timer burstTimer;              // for timing how far through the current burst we are

	float timings[MAX_REPETITIONS + 1] = {};        // store timings (calculated once on burst trigger)

	int triggersOccurred = 0;       // how many triggers have been
	int triggersRequested = 0;      // how many bursts have been requested (fixed over course of burst)
	bool active = true;             // is there a burst active
	bool wasInhibited = false;      // was this burst inhibited (i.e. just the first trigger sent)

	std::tuple<float, float, bool> process(float sampleTime) {

		if (active) {
			burstTimer.process(sampleTime);
		}

		bool eocTriggered = false;
		if (burstTimer.time > timings[triggersOccurred]) {
			if (triggersOccurred < triggersRequested) {
				burstOutput.reset();
				burstOutput.trigger(TRIGGER_TIME);
			}
			else if (triggersOccurred == triggersRequested) {
				eocOutput.reset();
				eocOutput.trigger(TRIGGER_TIME);
				active = false;
				eocTriggered = true;
			}
			triggersOccurred++;
		}

		const float burstOut = burstOutput.process(sampleTime);
		// NOTE: we don't get EOC if the burst was inhibited
		const float eocOut = eocOutput.process(sampleTime) * !wasInhibited;
		return std::make_tuple(burstOut, eocOut, eocTriggered);
	}

	void trigger(int numBursts, int multDiv, float baseTimeWindow, float distribution, bool inhibitBurst, bool includeOriginalTrigger) {

		active = true;
		wasInhibited = inhibitBurst;

		// the window in which the burst fits is a multiple (or division) of the base tempo
		int divisions = multDiv + (multDiv > 0 ? 1 : multDiv < 0 ? -1 : 0); 	// skip 2/-2
		float actualTimeWindow = baseTimeWindow;
		if (divisions > 0) {
			actualTimeWindow = baseTimeWindow * divisions;
		}
		else if (divisions < 0) {
			actualTimeWindow = baseTimeWindow / (-divisions);
		}

		// calculate the times at which triggers should fire, will be skewed by distribution
		const float power = 1 + std::abs(distribution) * 2;
		for (int i = 0; i <= numBursts; ++i) {
			if (distribution >= 0) {
				timings[i] = actualTimeWindow * std::pow((float)i / numBursts, power);
			}
			else {
				timings[i] = actualTimeWindow * std::pow((float)i / numBursts, 1 / power);
			}
		}

		triggersOccurred = includeOriginalTrigger ? 0 : 1;
		triggersRequested = inhibitBurst ? 1 : numBursts;
		burstTimer.reset();
	}
};

struct Burst : Module {
	enum ParamIds {
		CYCLE_PARAM,
		QUANTITY_PARAM,
		TRIGGER_PARAM,
		QUANTITY_CV_PARAM,
		DISTRIBUTION_PARAM,
		TIME_PARAM,
		PROBABILITY_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		QUANTITY_INPUT,
		DISTRIBUTION_INPUT,
		PING_INPUT,
		TIME_INPUT,
		PROBABILITY_INPUT,
		TRIGGER_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		TEMPO_OUTPUT,
		EOC_OUTPUT,
		OUT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(QUANTITY_LIGHTS, 16),
		TEMPO_LIGHT,
		EOC_LIGHT,
		OUT_LIGHT,
		NUM_LIGHTS
	};


	dsp::SchmittTrigger pingTrigger; 	// for detecting Ping in
	dsp::SchmittTrigger triggTrigger;	// for detecting Trigg in
	dsp::BooleanTrigger buttonTrigger;	// for detecting when the trigger button is pressed
	dsp::ClockDivider ledUpdate; 		// for only updating LEDs every N samples
	const int ledUpdateRate = 16; 		// LEDs updated every N = 16 samples

	PingableClock pingableClock;
	BurstEngine burstEngine;
	bool includeOriginalTrigger = true;

	Burst() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configSwitch(Burst::CYCLE_PARAM, 0.0, 1.0, 0.0, "Mode", {"One-shot", "Cycle"});
		auto quantityParam = configParam(Burst::QUANTITY_PARAM, 1, MAX_REPETITIONS, 0, "Number of bursts");
		quantityParam->snapEnabled = true;
		configButton(Burst::TRIGGER_PARAM, "Manual Trigger");
		configParam(Burst::QUANTITY_CV_PARAM, 0.0, 1.0, 1.0, "Quantity CV");
		configParam(Burst::DISTRIBUTION_PARAM, -1.0, 1.0, 0.0, "Distribution");
		auto timeParam = configParam(Burst::TIME_PARAM, -4.0, 4.0, 0.0, "Time Division/Multiplication");
		timeParam->snapEnabled = true;
		configParam(Burst::PROBABILITY_PARAM, 0.0, 1.0, 0.0, "Probability", "%", 0.f, -100, 100.);

		configInput(QUANTITY_INPUT, "Quantity CV");
		configInput(DISTRIBUTION_INPUT, "Distribution");
		configInput(PING_INPUT, "Ping");
		configInput(TIME_INPUT, "Time Division/Multiplication");
		configInput(PROBABILITY_INPUT, "Probability");
		configInput(TRIGGER_INPUT, "Trigger");
		
		ledUpdate.setDivision(ledUpdateRate);
	}

	void process(const ProcessArgs& args) override {

		const bool pingReceived = pingTrigger.process(inputs[PING_INPUT].getVoltage());
		pingableClock.process(pingReceived, args.sampleTime);

		if (ledUpdate.process()) {
			updateLEDRing(args);
		}

		const float quantityCV = params[QUANTITY_CV_PARAM].getValue() * clamp(inputs[QUANTITY_INPUT].getVoltage(), -5.0, +10.f) / 5.f;
		const int quantity = clamp((int)(params[QUANTITY_PARAM].getValue() + std::round(16 * quantityCV)), 1, MAX_REPETITIONS);

		const bool loop = params[CYCLE_PARAM].getValue();

		const float divMultCV = 4.0 * inputs[TIME_INPUT].getVoltage() / 10.f;
		const int divMult = -clamp((int)(divMultCV + params[TIME_PARAM].getValue()), -4, +4);

		const float distributionCV = inputs[DISTRIBUTION_INPUT].getVoltage() / 10.f;
		const float distribution = clamp(distributionCV + params[DISTRIBUTION_PARAM].getValue(), -1.f, +1.f);

		const bool triggerInputTriggered = triggTrigger.process(inputs[TRIGGER_INPUT].getVoltage());
		const bool triggerButtonTriggered = buttonTrigger.process(params[TRIGGER_PARAM].getValue());
		const bool startBurst = triggerInputTriggered || triggerButtonTriggered;

		if (startBurst) {
			const float prob = clamp(params[PROBABILITY_PARAM].getValue() + inputs[PROBABILITY_INPUT].getVoltage() / 10.f, 0.f, 1.f);
			const bool inhibitBurst = rack::random::uniform() < prob;

			// remember to do at current tempo
			burstEngine.trigger(quantity, divMult, pingableClock.tempo, distribution, inhibitBurst, includeOriginalTrigger);
		}

		float burstOut, eocOut;
		bool eoc;
		std::tie(burstOut, eocOut, eoc) = burstEngine.process(args.sampleTime);

		// if the burst has finished, we can also re-trigger
		if (eoc && loop) {
			const float prob = clamp(params[PROBABILITY_PARAM].getValue() + inputs[PROBABILITY_INPUT].getVoltage() / 10.f, 0.f, 1.f);
			const bool inhibitBurst = rack::random::uniform() < prob;

			// remember to do at current tempo
			burstEngine.trigger(quantity, divMult, pingableClock.tempo, distribution, inhibitBurst, includeOriginalTrigger);
		}

		const bool tempoOutHigh = pingableClock.isTempoOutHigh();
		outputs[TEMPO_OUTPUT].setVoltage(10.f * tempoOutHigh);
		lights[TEMPO_LIGHT].setBrightnessSmooth(tempoOutHigh, args.sampleTime);

		outputs[OUT_OUTPUT].setVoltage(10.f * burstOut);
		lights[OUT_LIGHT].setBrightnessSmooth(burstOut, args.sampleTime);

		outputs[EOC_OUTPUT].setVoltage(10.f * eocOut);
		lights[EOC_LIGHT].setBrightnessSmooth(eocOut, args.sampleTime);
	}

	void updateLEDRing(const ProcessArgs& args) {
		int activeLed;
		if (burstEngine.active) {
			activeLed = (burstEngine.triggersOccurred - 1) % 16;
		}
		else {
			activeLed = (((int) params[QUANTITY_PARAM].getValue() - 1) % 16);
		}
		for (int i = 0; i < 16; ++i) {
			lights[QUANTITY_LIGHTS + i].setBrightnessSmooth(i == activeLed, args.sampleTime * ledUpdateRate);
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "includeOriginalTrigger", json_boolean(includeOriginalTrigger));

		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* includeOriginalTriggerJ = json_object_get(rootJ, "includeOriginalTrigger");
		if (includeOriginalTriggerJ) {
			includeOriginalTrigger = json_boolean_value(includeOriginalTriggerJ);
		}
	}
};


struct BurstWidget : ModuleWidget {
	BurstWidget(Burst* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/Burst.svg")));

		addChild(createWidget<Knurlie>(Vec(15, 0)));
		addChild(createWidget<Knurlie>(Vec(15, 365)));

		addParam(createParam<BefacoSwitch>(mm2px(Vec(28.44228, 10.13642)), module, Burst::CYCLE_PARAM));
		addParam(createParam<Davies1900hWhiteKnobEndless>(mm2px(Vec(9.0322, 16.21467)), module, Burst::QUANTITY_PARAM));
		addParam(createParam<BefacoPush>(mm2px(Vec(28.43253, 29.6592)), module, Burst::TRIGGER_PARAM));
		addParam(createParam<BefacoTinyKnobLightGrey>(mm2px(Vec(17.26197, 41.95461)), module, Burst::QUANTITY_CV_PARAM));
		addParam(createParam<BefacoTinyKnobDarkGrey>(mm2px(Vec(22.85243, 58.45676)), module, Burst::DISTRIBUTION_PARAM));
		addParam(createParam<BefacoTinyKnobBlack>(mm2px(Vec(28.47229, 74.91607)), module, Burst::TIME_PARAM));
		addParam(createParam<BefacoTinyKnobDarkGrey>(mm2px(Vec(22.75115, 91.35201)), module, Burst::PROBABILITY_PARAM));

		addInput(createInput<BananutBlack>(mm2px(Vec(2.02153, 42.27628)), module, Burst::QUANTITY_INPUT));
		addInput(createInput<BananutBlack>(mm2px(Vec(7.90118, 58.74959)), module, Burst::DISTRIBUTION_INPUT));
		addInput(createInput<BananutBlack>(mm2px(Vec(2.05023, 75.25163)), module, Burst::PING_INPUT));
		addInput(createInput<BananutBlack>(mm2px(Vec(13.7751, 75.23049)), module, Burst::TIME_INPUT));
		addInput(createInput<BananutBlack>(mm2px(Vec(7.89545, 91.66642)), module, Burst::PROBABILITY_INPUT));
		addInput(createInput<BananutBlack>(mm2px(Vec(1.11155, 109.30346)), module, Burst::TRIGGER_INPUT));

		addOutput(createOutput<BananutRed>(mm2px(Vec(11.07808, 109.30346)), module, Burst::TEMPO_OUTPUT));
		addOutput(createOutput<BananutRed>(mm2px(Vec(21.08452, 109.32528)), module, Burst::EOC_OUTPUT));
		addOutput(createOutput<BananutRed>(mm2px(Vec(31.01113, 109.30346)), module, Burst::OUT_OUTPUT));

		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(14.03676, 9.98712)), module, Burst::QUANTITY_LIGHTS + 0));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(18.35846, 10.85879)), module, Burst::QUANTITY_LIGHTS + 1));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(22.05722, 13.31827)), module, Burst::QUANTITY_LIGHTS + 2));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(24.48707, 16.96393)), module, Burst::QUANTITY_LIGHTS + 3));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(25.38476, 21.2523)), module, Burst::QUANTITY_LIGHTS + 4));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(24.48707, 25.5354)), module, Burst::QUANTITY_LIGHTS + 5));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(22.05722, 29.16905)), module, Burst::QUANTITY_LIGHTS + 6));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(18.35846, 31.62236)), module, Burst::QUANTITY_LIGHTS + 7));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(14.03676, 32.48786)), module, Burst::QUANTITY_LIGHTS + 8));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(9.74323, 31.62236)), module, Burst::QUANTITY_LIGHTS + 9));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(6.10149, 29.16905)), module, Burst::QUANTITY_LIGHTS + 10));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(3.68523, 25.5354)), module, Burst::QUANTITY_LIGHTS + 11));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(2.85312, 21.2523)), module, Burst::QUANTITY_LIGHTS + 12));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(3.68523, 16.96393)), module, Burst::QUANTITY_LIGHTS + 13));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(6.10149, 13.31827)), module, Burst::QUANTITY_LIGHTS + 14));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(9.74323, 10.85879)), module, Burst::QUANTITY_LIGHTS + 15));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(14.18119, 104.2831)), module, Burst::TEMPO_LIGHT));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(24.14772, 104.2831)), module, Burst::EOC_LIGHT));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(34.11425, 104.2831)), module, Burst::OUT_LIGHT));
	}

	void appendContextMenu(Menu* menu) override {
		Burst* module = dynamic_cast<Burst*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator());
		menu->addChild(createBoolPtrMenuItem("Include original trigger in output", "", &module->includeOriginalTrigger));
	}
};


Model* modelBurst = createModel<Burst, BurstWidget>("Burst");

