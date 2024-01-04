#include "plugin.hpp"

using simd::float_4;

// an implementation of a performable, 3-stage switch, i.e. where
// the state triggers after being dragged a certain distance
struct BefacoSwitchMomentary : SvgSwitch {
	BefacoSwitchMomentary() {
		momentary = true;
		addFrame(APP->window->loadSvg(asset::system("res/ComponentLibrary/BefacoSwitch_0.svg")));
		addFrame(APP->window->loadSvg(asset::system("res/ComponentLibrary/BefacoSwitch_1.svg")));
		addFrame(APP->window->loadSvg(asset::system("res/ComponentLibrary/BefacoSwitch_2.svg")));
	}

	void onDragStart(const event::DragStart& e) override {
		latched = false;
		startMouseY = APP->scene->rack->getMousePos().y;
		ParamWidget::onDragStart(e);
	}

	void onDragMove(const event::DragMove& e) override {
		ParamQuantity* paramQuantity = getParamQuantity();
		float diff = APP->scene->rack->getMousePos().y - startMouseY;

		// Once the user has dragged the mouse a "threshold" distance, latch
		// to disallow further changes of state until the mouse is released.
		// We don't just setValue(1) (default/rest state) because this creates a
		// jarring UI experience
		if (diff < -10 && !latched) {
			paramQuantity->setValue(2);
			latched = true;
		}
		if (diff > 10 && !latched) {
			paramQuantity->setValue(0);
			latched = true;
		}

		ParamWidget::onDragMove(e);
	}

	void onDragEnd(const event::DragEnd& e) override {
		// on release, the switch resets to default/neutral/middle position
		getParamQuantity()->setValue(1);
		latched = false;
		ParamWidget::onDragEnd(e);
	}

	float startMouseY = 0.f;
	bool latched = false;
};


// Class which can yield a divided clock state, specifically where the
// gate is generated at request time through getGate(), rather than during
// process() - this means that different divisions of clock can be requested
// at any point in time. In contrast, the division/multiplication setting for
// ClockMultDiv cannot easily be changed _during_ a clock tick.
struct MultiGateClock {

	float remaining = 0.f;
	float fullPulseLength = 0.f;

	/** Immediately disables the pulse */
	void reset(float newfullPulseLength) {
		fullPulseLength = newfullPulseLength;
		remaining = fullPulseLength;
	}

	/** Advances the state by `deltaTime`. Returns whether the pulse is in the HIGH state. */
	bool process(float deltaTime) {
		if (remaining > 0.f) {
			remaining -= deltaTime;
			return true;
		}
		return false;
	}

	bool getGate(int gateMode) {

		if (gateMode == 0) {
			// always on (special case)
			return true;
		}
		else if (gateMode < 0 || remaining <= 0) {
			// disabled (or elapsed)
			return false;
		}

		const float multiGateOnLength = fullPulseLength / ((gateMode > 0) ? (2.f * gateMode) : 1.0f);
		const bool isOddPulse = int(floor(remaining / multiGateOnLength)) % 2;

		return isOddPulse;
	}
};


// Class for generating a clock sequence after setting a clock multiplication or division,
// given a stream of clock pulses as the "base" clock.
// Implementation is heavily inspired by BogAudio RGate, with modification
struct MultDivClock {

	// convention: negative values are used for division (1/mult), positive for multiplication (x mult)
	// multDiv = 0 should not be used, but if it is it will result in no modification to the clock
	int multDiv = 1;
	float secondsSinceLastClock = -1.0f;
	float inputClockLengthSeconds = -1.0f;

	// count how many divisions we've had
	int dividerCount = 0;

	float dividedProgressSeconds = 0.f;

	// returns the gated clock signal, returns true when high
	bool process(float deltaTime, bool clockPulseReceived) {

		if (clockPulseReceived) {
			// update our record of the incoming clock spacing
			if (secondsSinceLastClock > 0.0f) {
				inputClockLengthSeconds = secondsSinceLastClock;
			}
			secondsSinceLastClock = 0.0f;
		}

		bool out = false;
		if (secondsSinceLastClock >= 0.0f) {
			secondsSinceLastClock += deltaTime;

			// negative values are used for division (x 1/mult), positive for multiplication (x mult)
			const int division = std::max(-multDiv, 1);
			const int multiplication = std::max(multDiv, 1);

			if (clockPulseReceived) {
				if (dividerCount < 1) {
					dividedProgressSeconds = 0.0f;
				}
				else {
					dividedProgressSeconds += deltaTime;
				}
				++dividerCount;
				if (dividerCount >= division) {
					dividerCount = 0;
				}
			}
			else {
				dividedProgressSeconds += deltaTime;
			}

			// lengths of the mult/div versions of the clock
			const float dividedSeconds = inputClockLengthSeconds * (float) division;
			const float multipliedSeconds = dividedSeconds / (float) multiplication;

			// length of the output gate (s)
			const float gateSeconds = std::max(0.001f, multipliedSeconds * 0.5f);

			if (dividedProgressSeconds < dividedSeconds) {
				float multipliedProgressSeconds = dividedProgressSeconds / multipliedSeconds;
				multipliedProgressSeconds -= (float)(int)multipliedProgressSeconds;
				multipliedProgressSeconds *= multipliedSeconds;
				out = (multipliedProgressSeconds <= gateSeconds);
			}
		}
		return out;
	}

	float getEffectiveClockLength() {
		// negative values are used for division (x 1/mult), positive for multiplication (x mult)
		const int division = std::max(-multDiv, 1);
		const int multiplication = std::max(multDiv, 1);

		// lengths of the mult/div versions of the clock
		const float dividedSeconds = inputClockLengthSeconds * (float) division;
		const float multipliedSeconds = dividedSeconds / (float) multiplication;

		return multipliedSeconds;
	}
};

static const std::vector<int> clockOptionsQuadratic = {-16, -8, -4, -2, 1, 2, 4, 8, 16};
static const std::vector<int> clockOptionsAll = {-16, -15, -14, -13, -12, -11, -10, -9, -8, -7, -6, -5, -4, -3, -2, 1,
                                                 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
                                                };

inline std::string getClockOptionString(const int clockOption) {
	return (clockOption < 0) ? ("x 1/" + std::to_string(-clockOption)) : ("x " + std::to_string(clockOption));
}

struct Muxlicer : Module {
	enum ParamIds {
		PLAY_PARAM,
		ADDRESS_PARAM,
		GATE_MODE_PARAM,
		DIV_MULT_PARAM,
		ENUMS(LEVEL_PARAMS, 8),
		NUM_PARAMS
	};
	enum InputIds {
		GATE_MODE_INPUT,
		ADDRESS_INPUT,
		CLOCK_INPUT,
		RESET_INPUT,
		COM_INPUT,
		ENUMS(MUX_INPUTS, 8),
		ALL_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		CLOCK_OUTPUT,
		ALL_GATES_OUTPUT,
		EOC_OUTPUT,
		ENUMS(GATE_OUTPUTS, 8),
		ENUMS(MUX_OUTPUTS, 8),
		COM_OUTPUT,
		ALL_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		CLOCK_LIGHT,
		ENUMS(GATE_LIGHTS, 8),
		NUM_LIGHTS
	};

	enum ModeCOMIO {
		COM_1_IN_8_OUT,
		COM_8_IN_1_OUT
	};

	enum PlayState {
		STATE_PLAY_ONCE,
		STATE_STOPPED,
		STATE_PLAY
	};

	/*
	This shows how the values of the gate mode knob + CV map onto gate triggers.
	See also getGateMode()
	value   | description     	    | quadratic only mode
	   -1 	   no gate  	    	|	    ✔
	    0      gate (full timestep) |       x
	   +1      half timestep      	|	    ✔
	    2      two gates       	    | 	    ✔
	    3      three gates     	    |       x
	    4      four gates      	    |       ✔
	    5      five gates      	    |       x
	    6      six gates       	    |       x
	    7      seven gates     	    |       x
	    8      eight gates     	    |       ✔
	*/
	const int possibleQuadraticGates[5] = {-1, 1, 2, 4, 8};
	bool quadraticGatesOnly = false;
	bool outputClockFollowsPlayMode = false;

	PlayState playState = STATE_STOPPED;
	dsp::BooleanTrigger playStateTrigger;

	uint32_t runIndex; 	// which step are we on (0 to 7)
	uint32_t addressIndex = 0;
	bool tapped = false;

	enum ResetStatus {
		RESET_NOT_REQUESTED,
		RESET_AND_PLAY_ONCE,
		RESET_AND_PLAY
	};
	// Used to track if a reset has been triggered. Can be from the CV input, or the momentary switch. Note
	// that behaviour depends on if the Muxlicer is clocked or not. If clocked, the playhead resets but waits
	// for the next clock tick to start. If not clocked, then the sequence will start immediately (i.e. the
	// internal clock will be synced at the point where `resetIsRequested` is first true.
	ResetStatus resetRequested = RESET_NOT_REQUESTED;
	// used to detect when `resetRequested` first becomes active
	dsp::BooleanTrigger detectResetTrigger;

	// used to track the clock (e.g. if external clock is not connected). NOTE: this clock
	// is defined _prior_ to any clock division/multiplication logic
	float internalClockProgress = 0.f;
	float internalClockLength = 0.25f;

	float tapTime = 99999;	// used to track the time between clock pulses (or taps?)
	dsp::SchmittTrigger inputClockTrigger;	// to detect incoming clock pulses
	dsp::BooleanTrigger mainClockTrigger;	// to detect when divided/multiplied version of the clock signal has rising edge
	dsp::SchmittTrigger resetTrigger; 		// to detect the reset signal
	dsp::PulseGenerator endOfCyclePulse; 	// fire a signal at the end of cycle
	dsp::BooleanTrigger tapTempoTrigger;	// to only trigger tap tempo when push is first detected
	MultDivClock mainClockMultDiv;			// to produce a divided/multiplied version of the (internal or external) clock signal
	MultDivClock outputClockMultDiv;		// to produce a divided/multiplied version of the output clock signal
	MultiGateClock multiClock;				// to easily produce a divided version of the main clock (where division can be changed at any point)
	bool usingExternalClock = false; 		// is there a clock plugged into clock in (external) or not (internal)

	const static int SEQUENCE_LENGTH = 8;
	ModeCOMIO modeCOMIO = COM_8_IN_1_OUT;	// are we in 1-in-8-out mode, or 8-in-1-out mode
	int allInNormalVoltage = 10;			// what voltage is normalled into the "All In" input, selectable via context menu
	Module* rightModule;					// for the expander

	// these are class variables, rather than scoped to process(...), to allow expanders to read
	// all gate output and clock output
	bool isAllGatesOutHigh = false;
	bool isOutputClockHigh = false;

	struct DivMultKnobParamQuantity : ParamQuantity {
		std::string getDisplayValueString() override {
			Muxlicer* moduleMuxlicer = reinterpret_cast<Muxlicer*>(module);
			if (moduleMuxlicer != nullptr) {
				return getClockOptionString(moduleMuxlicer->getClockOptionFromParam());
			}
			else {
				return "";
			}
		}
	};

	struct GateModeParamQuantity : ParamQuantity {
		std::string getDisplayValueString() override {
			Muxlicer* moduleMuxlicer = reinterpret_cast<Muxlicer*>(module);

			if (moduleMuxlicer != nullptr) {
				bool attenuatorMode = moduleMuxlicer->inputs[GATE_MODE_INPUT].isConnected();
				if (attenuatorMode) {
					return ParamQuantity::getDisplayValueString();
				}
				else {
					const int gate = moduleMuxlicer->getGateMode();
					if (gate < 0) {
						return "No gate";
					}
					else if (gate == 0) {
						return "1 gate (100\% duty)";
					}
					else if (gate == 1) {
						return "1 gate (50\% duty)";
					}
					else {
						return string::f("%d gate(s)", gate);
					}
				}
			}
			else {
				return ParamQuantity::getDisplayValueString();
			}
		}
	};

	// given param (in range 0 to 1), return the clock option from an array of choices
	int getClockOptionFromParam() {
		if (quadraticGatesOnly) {
			const int clockOptionIndex = round(params[Muxlicer::DIV_MULT_PARAM].getValue() * (clockOptionsQuadratic.size() - 1));
			return clockOptionsQuadratic[clockOptionIndex];
		}
		else {
			const int clockOptionIndex = round(params[Muxlicer::DIV_MULT_PARAM].getValue() * (clockOptionsAll.size() - 1));
			return clockOptionsAll[clockOptionIndex];
		}
	}

	// given a the mult/div setting for the main clock, find the index of this from an array of valid choices,
	// and convert to a value between 0 and 1 (update the DIV_MULT_PARAM param)
	void updateParamFromMainClockMultDiv() {

		auto const& arrayToSearch = quadraticGatesOnly ? clockOptionsQuadratic : clockOptionsAll;
		auto const it = std::find(arrayToSearch.begin(), arrayToSearch.end(), mainClockMultDiv.multDiv);

		// try to find the index in the array of valid clock mults/divs
		if (it != arrayToSearch.end()) {
			int index = it - arrayToSearch.begin();
			float paramIndex = (float) index / (arrayToSearch.size() - 1);
			params[Muxlicer::DIV_MULT_PARAM].setValue(paramIndex);
		}
		// if not, default to 0.5 (which should correspond to x1, no mult/div)
		else {
			params[Muxlicer::DIV_MULT_PARAM].setValue(0.5);
		}
	}

	Muxlicer() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configSwitch(Muxlicer::PLAY_PARAM, STATE_PLAY_ONCE, STATE_PLAY, STATE_STOPPED, "Play switch", {"Play Once/Reset", "", "Play/Stop"});
		getParamQuantity(Muxlicer::PLAY_PARAM)->randomizeEnabled = false;
		configParam(Muxlicer::ADDRESS_PARAM, -1.f, 7.f, -1.f, "Address");
		getParamQuantity(Muxlicer::ADDRESS_PARAM)->randomizeEnabled = false;

		configParam<GateModeParamQuantity>(Muxlicer::GATE_MODE_PARAM, -1.f, 8.f, 1.f, "Gate mode");
		configParam<DivMultKnobParamQuantity>(Muxlicer::DIV_MULT_PARAM, 0, 1, 0.5, "Main clock mult/div");

		for (int i = 0; i < SEQUENCE_LENGTH; ++i) {
			configParam(Muxlicer::LEVEL_PARAMS + i, 0.0, 1.0, 1.0, string::f("Gain step %d", i + 1));
			configInput(Muxlicer::MUX_INPUTS + i, string::f("Step %d", i + 1));
			configOutput(Muxlicer::GATE_OUTPUTS + i, string::f("Gate step %d", i + 1));
			configOutput(Muxlicer::MUX_OUTPUTS + i, string::f("Step %d", i + 1));
			configLight(Muxlicer::GATE_LIGHTS + i, string::f("Step %d gates", i + 1));
		}
		configOutput(Muxlicer::EOC_OUTPUT, "End of cycle trigger");
		configOutput(Muxlicer::CLOCK_OUTPUT, "Clock");
		configOutput(Muxlicer::ALL_GATES_OUTPUT, "All gates");
		configOutput(Muxlicer::ALL_OUTPUT, "All");
		configOutput(Muxlicer::COM_OUTPUT, "COM I/O");

		configInput(Muxlicer::GATE_MODE_INPUT, "Gate mode CV");
		configInput(Muxlicer::ADDRESS_INPUT, "Address CV");
		configInput(Muxlicer::CLOCK_INPUT, "Clock");
		configInput(Muxlicer::RESET_INPUT, "One shot/reset");
		configInput(Muxlicer::COM_INPUT, "COM I/O");
		configInput(Muxlicer::ALL_INPUT, "All");

		onReset();
	}

	void onReset() override {
		internalClockLength = 0.250f;
		internalClockProgress = 0;
		runIndex = 0;
		mainClockMultDiv.multDiv = 1;
		outputClockMultDiv.multDiv = 1;
		quadraticGatesOnly = false;
		playState = STATE_STOPPED;
	}

	void process(const ProcessArgs& args) override {

		usingExternalClock = inputs[CLOCK_INPUT].isConnected();

		bool externalClockPulseReceived = false;
		// a clock pulse does two things: 1) sets the internal clock (based on timing between two pulses), which
		// would continue were the clock input to be removed, and 2) synchronises/drives the clock (if clock input present)
		if (usingExternalClock && inputClockTrigger.process(rescale(inputs[CLOCK_INPUT].getVoltage(), 0.1f, 2.f, 0.f, 1.f))) {
			externalClockPulseReceived = true;
		}
		// this can also be sent by tap tempo
		else if (!usingExternalClock && tapTempoTrigger.process(tapped)) {
			externalClockPulseReceived = true;
			tapped = false;
		}

		mainClockMultDiv.multDiv = getClockOptionFromParam();

		processPlayResetLogic();

		const float address = params[ADDRESS_PARAM].getValue() + inputs[ADDRESS_INPUT].getVoltage();
		const bool isAddressInRunMode = address < 0.f;

		// even if we have an external clock, use its pulses to time/sync the internal clock
		// so that it will remain running even after CLOCK_INPUT is disconnected
		if (externalClockPulseReceived) {
			// track length between received clock pulses (using external clock) or taps
			// of the tap-tempo menu item (if sufficiently short)
			if (usingExternalClock || tapTime < 2.f) {
				internalClockLength = tapTime;
			}
			tapTime = 0;
			internalClockProgress = 0.f;
		}

		// If we get a reset signal (which can come from CV or various modes of the switch), and the clock has only
		// just started to tick (internalClockProgress < 1ms), we assume that the reset signal is slightly delayed
		// due to the 1 sample delay that Rack introduces. If this is the case, the internal clock trigger detector,
		// `detectResetTrigger`, which advances the sequence, will not be "primed" to detect a rising edge for another
		// whole clock tick, meaning the first step is repeated. See: https://github.com/VCVRack/Befaco/issues/32
		// Also see https://vcvrack.com/manual/VoltageStandards#Timing for 0.001 seconds justification.
		if (detectResetTrigger.process(resetRequested != RESET_NOT_REQUESTED) && internalClockProgress < 1e-3) {
			// NOTE: the sequence must also be stopped for this to come into effect. In hardware, if the Nth step Gate Out
			// is patched back into the reset, that step should complete before the sequence restarts.
			if (playState == STATE_STOPPED) {
				mainClockTrigger.state = false;
			}
		}
		tapTime += args.sampleTime;
		internalClockProgress += args.sampleTime;

		// track if the internal clock has "ticked"
		const bool internalClockPulseReceived = (internalClockProgress >= internalClockLength);
		if (internalClockPulseReceived) {
			internalClockProgress = 0.f;
		}

		// we can be in one of two clock modes:
		// * external (decided by pulses to CLOCK_INPUT)
		// * internal (decided by internalClockProgress exceeding the internal clock length)
		//
		// choose which clock source we are to use
		const bool clockPulseReceived = usingExternalClock ? externalClockPulseReceived : internalClockPulseReceived;
		// apply the main clock div/mult logic to whatever clock source we're using - mainClockMultDiv outputs a gate sequence
		// so we must use a BooleanTrigger on the divided/mult'd signal in order to detect rising edge / when to advance the sequence
		const bool dividedMultipliedClockPulseReceived = mainClockTrigger.process(mainClockMultDiv.process(args.sampleTime, clockPulseReceived));

		if (dividedMultipliedClockPulseReceived) {

			if (resetRequested != RESET_NOT_REQUESTED) {
				runIndex = 7;

				if (resetRequested == RESET_AND_PLAY) {
					playState = STATE_PLAY;
				}
				else if (resetRequested == RESET_AND_PLAY_ONCE) {
					playState = STATE_PLAY_ONCE;

				}
			}

			if (isAddressInRunMode && playState != STATE_STOPPED) {

				runIndex++;

				if (runIndex >= SEQUENCE_LENGTH) {

					runIndex = 0;

					// the sequence resets by placing the play head at the final step (so that the next clock pulse
					// ticks over onto the first step) - so if we are on the final step _because_ we've reset,
					// then don't fire EOC, just clear the reset status
					if (resetRequested != RESET_NOT_REQUESTED) {
						resetRequested = RESET_NOT_REQUESTED;
					}
					// otherwise we've naturally arrived at the last step so do fire EOC etc
					else {
						endOfCyclePulse.trigger(1e-3);

						// stop on this step if in one shot mode
						if (playState == STATE_PLAY_ONCE) {
							playState = STATE_STOPPED;
						}
					}
				}
			}

			multiClock.reset(mainClockMultDiv.getEffectiveClockLength());

			if (isAddressInRunMode) {
				addressIndex = runIndex;
			}
			else {
				addressIndex = clamp((int) roundf(address), 0, SEQUENCE_LENGTH - 1);
			}

			for (int i = 0; i < 8; i++) {
				outputs[GATE_OUTPUTS + i].setVoltage(0.f);
			}
		}

		// Gates
		for (int i = 0; i < 8; i++) {
			outputs[GATE_OUTPUTS + i].setVoltage(0.f);
			lights[GATE_LIGHTS + i].setBrightness(0.f);
		}
		outputs[ALL_GATES_OUTPUT].setVoltage(0.f);

		multiClock.process(args.sampleTime);
		const int gateMode = getGateMode();

		// current gate output _and_ "All Gates" output both get the gate pattern from multiClock
		// NOTE: isAllGatesOutHigh can also be read by expanders
		isAllGatesOutHigh = multiClock.getGate(gateMode) && (playState != STATE_STOPPED);
		outputs[GATE_OUTPUTS + addressIndex].setVoltage(isAllGatesOutHigh * 10.f);
		lights[GATE_LIGHTS + addressIndex].setBrightness(isAllGatesOutHigh * 1.f);
		outputs[ALL_GATES_OUTPUT].setVoltage(isAllGatesOutHigh * 10.f);

		if (modeCOMIO == COM_1_IN_8_OUT) {
			const int numActiveEngines = std::max(inputs[ALL_INPUT].getChannels(), inputs[COM_INPUT].getChannels());
			const float stepVolume = params[LEVEL_PARAMS + addressIndex].getValue();

			for (int c = 0; c < numActiveEngines; c += 4) {
				// Mux outputs (all zero, except active step, if playing)
				for (int i = 0; i < 8; i++) {
					outputs[MUX_OUTPUTS + i].setVoltageSimd<float_4>(0.f, c);
				}

				const float_4 com_input = inputs[COM_INPUT].getPolyVoltageSimd<float_4>(c);
				if (outputs[MUX_OUTPUTS + addressIndex].isConnected()) {
					outputs[MUX_OUTPUTS + addressIndex].setVoltageSimd(stepVolume * com_input, c);
					outputs[ALL_OUTPUT].setVoltageSimd<float_4>(0.f, c);
				}
				else if (outputs[ALL_OUTPUT].isConnected()) {
					outputs[ALL_OUTPUT].setVoltageSimd(stepVolume * com_input, c);
				}
			}

			for (int i = 0; i < 8; i++) {
				outputs[MUX_OUTPUTS + i].setChannels(numActiveEngines);
			}
			outputs[ALL_OUTPUT].setChannels(numActiveEngines);
		}
		else if (modeCOMIO == COM_8_IN_1_OUT) {
			// we need at least one active engine, even if nothing is connected
			// as we want the voltage that is normalled to All In to be processed
			int numActiveEngines = std::max(1, inputs[ALL_INPUT].getChannels());
			for (int i = 0; i < 8; i++) {
				numActiveEngines = std::max(numActiveEngines, inputs[MUX_INPUTS + i].getChannels());
			}

			const float stepVolume = params[LEVEL_PARAMS + addressIndex].getValue();
			for (int c = 0; c < numActiveEngines; c += 4) {
				const float_4 allInValue = inputs[ALL_INPUT].getNormalPolyVoltageSimd<float_4>((float_4) allInNormalVoltage, c);
				const float_4 stepValue = inputs[MUX_INPUTS + addressIndex].getNormalPolyVoltageSimd<float_4>(allInValue, c) * stepVolume;
				outputs[COM_OUTPUT].setVoltageSimd(stepValue, c);
			}
			outputs[COM_OUTPUT].setChannels(numActiveEngines);
		}

		// there is an option to stop output clock when play stops
		const bool playStateMask = !outputClockFollowsPlayMode || (playState != STATE_STOPPED);
		// NOTE: outputClockOut can also be read by expanders
		isOutputClockHigh = outputClockMultDiv.process(args.sampleTime, clockPulseReceived) && playStateMask;
		outputs[CLOCK_OUTPUT].setVoltage(isOutputClockHigh * 10.f);
		lights[CLOCK_LIGHT].setBrightness(isOutputClockHigh * 1.f);

		// end of cycle trigger trigger
		outputs[EOC_OUTPUT].setVoltage(endOfCyclePulse.process(args.sampleTime) ? 10.f : 0.f);

	}

	void processPlayResetLogic() {

		const bool resetPulseInReceived = resetTrigger.process(rescale(inputs[RESET_INPUT].getVoltage(), 0.1f, 2.f, 0.f, 1.f));
		if (resetPulseInReceived) {

			switch (playState) {
				case STATE_STOPPED: resetRequested = RESET_AND_PLAY_ONCE; break;
				case STATE_PLAY_ONCE: resetRequested = RESET_AND_PLAY_ONCE; break;
				case STATE_PLAY: resetRequested = RESET_AND_PLAY; break;
			}
		}

		// if the play switch has effectively been activated for the first time,
		// i.e. it's not just still being held
		const bool switchIsActive = params[PLAY_PARAM].getValue() != STATE_STOPPED;
		if (playStateTrigger.process(switchIsActive) && switchIsActive) {

			// if we were stopped, check for activation (normal, up or one-shot, down)
			if (playState == STATE_STOPPED) {
				if (params[PLAY_PARAM].getValue() == STATE_PLAY) {
					resetRequested = RESET_AND_PLAY;
				}
				else if (params[PLAY_PARAM].getValue() == STATE_PLAY_ONCE) {
					resetRequested = RESET_AND_PLAY_ONCE;
				}
			}
			// otherwise we are in play mode (and we've not just held onto the play switch),
			// so check for stop (switch up) or reset (switch down)
			else {

				// top switch will stop
				if (params[PLAY_PARAM].getValue() == STATE_PLAY) {
					playState = STATE_STOPPED;
				}
				// bottom will reset
				else if (params[PLAY_PARAM].getValue() == STATE_PLAY_ONCE) {
					resetRequested = RESET_AND_PLAY_ONCE;
				}
			}
		}
	}


	// determines how many gates to yield per step
	int getGateMode() {

		int gate;

		if (inputs[GATE_MODE_INPUT].isConnected()) {
			// with gate acting as attenuator, hardware reacts in 1V increments,
			// where x V -> (x + 1) V yields (x - 1) gates in that time
			float gateCV = clamp(inputs[GATE_MODE_INPUT].getVoltage(), 0.f, 10.f);
			float knobAttenuation = rescale(params[GATE_MODE_PARAM].getValue(), -1.f, 8.f, 0.f, 1.f);

			gate = int (floor(gateCV * knobAttenuation)) - 1;
		}
		else {
			gate = (int) roundf(params[GATE_MODE_PARAM].getValue());
		}

		// should be respected, but make sure
		gate = clamp(gate, -1, 8);

		if (quadraticGatesOnly) {
			int quadraticGateIndex = int(floor(rescale(gate, -1.f, 8.f, 0.f, 4.99f)));
			return possibleQuadraticGates[clamp(quadraticGateIndex, 0, 4)];
		}
		else {
			return gate;
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "modeCOMIO", json_integer(modeCOMIO));
		json_object_set_new(rootJ, "quadraticGatesOnly", json_boolean(quadraticGatesOnly));
		json_object_set_new(rootJ, "allInNormalVoltage", json_integer(allInNormalVoltage));
		json_object_set_new(rootJ, "mainClockMultDiv", json_integer(mainClockMultDiv.multDiv));
		json_object_set_new(rootJ, "outputClockMultDiv", json_integer(outputClockMultDiv.multDiv));
		json_object_set_new(rootJ, "playState", json_integer(playState));
		json_object_set_new(rootJ, "outputClockFollowsPlayMode", json_boolean(outputClockFollowsPlayMode));

		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* modeJ = json_object_get(rootJ, "modeCOMIO");
		if (modeJ) {
			modeCOMIO = (Muxlicer::ModeCOMIO) json_integer_value(modeJ);
		}

		json_t* quadraticJ = json_object_get(rootJ, "quadraticGatesOnly");
		if (quadraticJ) {
			quadraticGatesOnly = json_boolean_value(quadraticJ);
		}

		json_t* allInNormalVoltageJ = json_object_get(rootJ, "allInNormalVoltage");
		if (allInNormalVoltageJ) {
			allInNormalVoltage = json_integer_value(allInNormalVoltageJ);
		}

		json_t* mainClockMultDivJ = json_object_get(rootJ, "mainClockMultDiv");
		if (mainClockMultDivJ) {
			mainClockMultDiv.multDiv = json_integer_value(mainClockMultDivJ);
		}

		json_t* outputClockMultDivJ = json_object_get(rootJ, "outputClockMultDiv");
		if (outputClockMultDivJ) {
			outputClockMultDiv.multDiv = json_integer_value(outputClockMultDivJ);
		}

		json_t* playStateJ = json_object_get(rootJ, "playState");
		if (playStateJ) {
			playState = (PlayState) json_integer_value(playStateJ);
		}

		json_t* outputClockFollowsPlayModeJ = json_object_get(rootJ, "outputClockFollowsPlayMode");
		if (outputClockFollowsPlayModeJ) {
			outputClockFollowsPlayMode = json_boolean_value(outputClockFollowsPlayModeJ);
		}

		updateParamFromMainClockMultDiv();
	}

};



struct MuxlicerWidget : ModuleWidget {
	MuxlicerWidget(Muxlicer* module) {

		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/Muxlicer.svg")));

		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<Knurlie>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParam<BefacoSwitchMomentary>(mm2px(Vec(35.72963, 10.008)), module, Muxlicer::PLAY_PARAM));
		addParam(createParam<BefacoTinyKnobDarkGrey>(mm2px(Vec(3.84112, 10.90256)), module, Muxlicer::ADDRESS_PARAM));
		addParam(createParam<BefacoTinyKnobDarkGrey>(mm2px(Vec(67.83258, 10.86635)), module, Muxlicer::GATE_MODE_PARAM));
		addParam(createParam<BefacoTinyKnob>(mm2px(Vec(28.12238, 24.62151)), module, Muxlicer::DIV_MULT_PARAM));

		addParam(createParam<BefacoSlidePot>(mm2px(Vec(2.32728, 40.67102)), module, Muxlicer::LEVEL_PARAMS + 0));
		addParam(createParam<BefacoSlidePot>(mm2px(Vec(12.45595, 40.67102)), module, Muxlicer::LEVEL_PARAMS + 1));
		addParam(createParam<BefacoSlidePot>(mm2px(Vec(22.58462, 40.67102)), module, Muxlicer::LEVEL_PARAMS + 2));
		addParam(createParam<BefacoSlidePot>(mm2px(Vec(32.7133, 40.67102)), module, Muxlicer::LEVEL_PARAMS + 3));
		addParam(createParam<BefacoSlidePot>(mm2px(Vec(42.74195, 40.67102)), module, Muxlicer::LEVEL_PARAMS + 4));
		addParam(createParam<BefacoSlidePot>(mm2px(Vec(52.97062, 40.67102)), module, Muxlicer::LEVEL_PARAMS + 5));
		addParam(createParam<BefacoSlidePot>(mm2px(Vec(63.0993, 40.67102)), module, Muxlicer::LEVEL_PARAMS + 6));
		addParam(createParam<BefacoSlidePot>(mm2px(Vec(73.22797, 40.67102)), module, Muxlicer::LEVEL_PARAMS + 7));

		addInput(createInput<BefacoInputPort>(mm2px(Vec(51.568, 11.20189)), module, Muxlicer::GATE_MODE_INPUT));
		addInput(createInput<BefacoInputPort>(mm2px(Vec(21.13974, 11.23714)), module, Muxlicer::ADDRESS_INPUT));
		addInput(createInput<BefacoInputPort>(mm2px(Vec(44.24461, 24.93662)), module, Muxlicer::CLOCK_INPUT));
		addInput(createInput<BefacoInputPort>(mm2px(Vec(12.62135, 24.95776)), module, Muxlicer::RESET_INPUT));
		addInput(createInput<BefacoInputPort>(mm2px(Vec(36.3142, 98.07911)), module, Muxlicer::COM_INPUT));
		addInput(createInput<BefacoInputPort>(mm2px(Vec(0.895950, 109.27901)), module, Muxlicer::MUX_INPUTS + 0));
		addInput(createInput<BefacoInputPort>(mm2px(Vec(11.05332, 109.29256)), module, Muxlicer::MUX_INPUTS + 1));
		addInput(createInput<BefacoInputPort>(mm2px(Vec(21.18201, 109.29256)), module, Muxlicer::MUX_INPUTS + 2));
		addInput(createInput<BefacoInputPort>(mm2px(Vec(31.27625, 109.27142)), module, Muxlicer::MUX_INPUTS + 3));
		addInput(createInput<BefacoInputPort>(mm2px(Vec(41.40493, 109.27142)), module, Muxlicer::MUX_INPUTS + 4));
		addInput(createInput<BefacoInputPort>(mm2px(Vec(51.53360, 109.27142)), module, Muxlicer::MUX_INPUTS + 5));
		addInput(createInput<BefacoInputPort>(mm2px(Vec(61.69671, 109.29256)), module, Muxlicer::MUX_INPUTS + 6));
		addInput(createInput<BefacoInputPort>(mm2px(Vec(71.82537, 109.29256)), module, Muxlicer::MUX_INPUTS + 7));
		addInput(createInput<BefacoInputPort>(mm2px(Vec(16.11766, 98.09121)), module, Muxlicer::ALL_INPUT));

		addOutput(createOutput<BefacoOutputPort>(mm2px(Vec(59.8492, 24.95776)), module, Muxlicer::CLOCK_OUTPUT));
		addOutput(createOutput<BefacoOutputPort>(mm2px(Vec(56.59663, 98.06252)), module, Muxlicer::ALL_GATES_OUTPUT));
		addOutput(createOutput<BefacoOutputPort>(mm2px(Vec(66.72661, 98.07008)), module, Muxlicer::EOC_OUTPUT));
		addOutput(createOutput<BefacoOutputPort>(mm2px(Vec(0.89595, 86.78581)), module, Muxlicer::GATE_OUTPUTS + 0));
		addOutput(createOutput<BefacoOutputPort>(mm2px(Vec(11.02463, 86.77068)), module, Muxlicer::GATE_OUTPUTS + 1));
		addOutput(createOutput<BefacoOutputPort>(mm2px(Vec(21.14758, 86.77824)), module, Muxlicer::GATE_OUTPUTS + 2));
		addOutput(createOutput<BefacoOutputPort>(mm2px(Vec(31.27625, 86.77824)), module, Muxlicer::GATE_OUTPUTS + 3));
		addOutput(createOutput<BefacoOutputPort>(mm2px(Vec(41.40493, 86.77824)), module, Muxlicer::GATE_OUTPUTS + 4));
		addOutput(createOutput<BefacoOutputPort>(mm2px(Vec(51.56803, 86.79938)), module, Muxlicer::GATE_OUTPUTS + 5));
		addOutput(createOutput<BefacoOutputPort>(mm2px(Vec(61.69671, 86.79938)), module, Muxlicer::GATE_OUTPUTS + 6));
		addOutput(createOutput<BefacoOutputPort>(mm2px(Vec(71.79094, 86.77824)), module, Muxlicer::GATE_OUTPUTS + 7));

		// these blocks are exclusive (for visibility / interactivity) and allows IO and OI within one module
		addOutput(createOutput<BefacoOutputPort>(mm2px(Vec(0.895950, 109.27901)), module, Muxlicer::MUX_OUTPUTS + 0));
		addOutput(createOutput<BefacoOutputPort>(mm2px(Vec(11.05332, 109.29256)), module, Muxlicer::MUX_OUTPUTS + 1));
		addOutput(createOutput<BefacoOutputPort>(mm2px(Vec(21.18201, 109.29256)), module, Muxlicer::MUX_OUTPUTS + 2));
		addOutput(createOutput<BefacoOutputPort>(mm2px(Vec(31.27625, 109.27142)), module, Muxlicer::MUX_OUTPUTS + 3));
		addOutput(createOutput<BefacoOutputPort>(mm2px(Vec(41.40493, 109.27142)), module, Muxlicer::MUX_OUTPUTS + 4));
		addOutput(createOutput<BefacoOutputPort>(mm2px(Vec(51.53360, 109.27142)), module, Muxlicer::MUX_OUTPUTS + 5));
		addOutput(createOutput<BefacoOutputPort>(mm2px(Vec(61.69671, 109.29256)), module, Muxlicer::MUX_OUTPUTS + 6));
		addOutput(createOutput<BefacoOutputPort>(mm2px(Vec(71.82537, 109.29256)), module, Muxlicer::MUX_OUTPUTS + 7));
		addOutput(createOutput<BefacoOutputPort>(mm2px(Vec(36.3142, 98.07911)), module, Muxlicer::COM_OUTPUT));
		addOutput(createOutput<BefacoOutputPort>(mm2px(Vec(16.11766, 98.09121)), module, Muxlicer::ALL_OUTPUT));

		updatePortVisibilityForIOMode(Muxlicer::COM_8_IN_1_OUT);

		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(71.28361, 28.02644)), module, Muxlicer::CLOCK_LIGHT));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(3.99336, 81.86801)), module, Muxlicer::GATE_LIGHTS + 0));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(14.09146, 81.86801)), module, Muxlicer::GATE_LIGHTS + 1));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(24.22525, 81.86801)), module, Muxlicer::GATE_LIGHTS + 2));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(34.35901, 81.86801)), module, Muxlicer::GATE_LIGHTS + 3));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(44.49277, 81.86801)), module, Muxlicer::GATE_LIGHTS + 4));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(54.62652, 81.86801)), module, Muxlicer::GATE_LIGHTS + 5));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(64.76028, 81.86801)), module, Muxlicer::GATE_LIGHTS + 6));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(74.89404, 81.86801)), module, Muxlicer::GATE_LIGHTS + 7));
	}

	void draw(const DrawArgs& args) override {
		Muxlicer* module = dynamic_cast<Muxlicer*>(this->module);

		if (module != nullptr) {
			updatePortVisibilityForIOMode(module->modeCOMIO);
		}
		else {
			// module can be null, e.g. if populating the module browser with screenshots,
			// in which case just assume the default (8 in, 1 out)
			updatePortVisibilityForIOMode(Muxlicer::COM_8_IN_1_OUT);
		}

		ModuleWidget::draw(args);
	}

	struct IOMenuItem : MenuItem {
		Muxlicer* module;
		MuxlicerWidget* widget;
		void onAction(const event::Action& e) override {
			module->modeCOMIO = Muxlicer::COM_1_IN_8_OUT;
			widget->updatePortVisibilityForIOMode(module->modeCOMIO);
			widget->clearCables();
		}
	};
	struct OIMenuItem : MenuItem {
		Muxlicer* module;
		MuxlicerWidget* widget;
		void onAction(const event::Action& e) override {
			module->modeCOMIO = Muxlicer::COM_8_IN_1_OUT;
			widget->updatePortVisibilityForIOMode(module->modeCOMIO);
			widget->clearCables();
		}
	};

	struct OutputRangeChildItem : MenuItem {
		Muxlicer* module;
		int allInNormalVoltage;
		void onAction(const event::Action& e) override {
			module->allInNormalVoltage = allInNormalVoltage;
		}
	};

	struct OutputRangeItem : MenuItem {
		Muxlicer* module;

		Menu* createChildMenu() override {
			Menu* menu = new Menu;

			std::vector<int> voltageOptions = {1, 5, 10};
			for (auto voltageOption : voltageOptions) {
				OutputRangeChildItem* rangeItem = createMenuItem<OutputRangeChildItem>(std::to_string(voltageOption) + "V",
				                                  CHECKMARK(module->allInNormalVoltage == voltageOption));
				rangeItem->allInNormalVoltage = voltageOption;
				rangeItem->module = module;
				menu->addChild(rangeItem);
			}

			return menu;
		}
	};

	struct OutputClockScalingItem : MenuItem {
		Muxlicer* module;

		struct OutputClockScalingChildItem : MenuItem {
			Muxlicer* module;
			int clockOutMulDiv;
			void onAction(const event::Action& e) override {
				module->outputClockMultDiv.multDiv = clockOutMulDiv;
			}
		};

		Menu* createChildMenu() override {
			Menu* menu = new Menu;

			for (int clockOption : module->quadraticGatesOnly ? clockOptionsQuadratic : clockOptionsAll) {
				std::string optionString = getClockOptionString(clockOption);
				OutputClockScalingChildItem* clockItem = createMenuItem<OutputClockScalingChildItem>(optionString,
				    CHECKMARK(module->outputClockMultDiv.multDiv == clockOption));
				clockItem->clockOutMulDiv = clockOption;
				clockItem->module = module;
				menu->addChild(clockItem);
			}

			return menu;
		}
	};

	struct MainClockScalingItem : MenuItem {
		Muxlicer* module;

		struct MainClockScalingChildItem : MenuItem {
			Muxlicer* module;
			int mainClockMulDiv, mainClockMulDivIndex;

			void onAction(const event::Action& e) override {
				module->mainClockMultDiv.multDiv = mainClockMulDiv;
				module->updateParamFromMainClockMultDiv();
			}
		};

		Menu* createChildMenu() override {
			Menu* menu = new Menu;

			int i = 0;

			for (int clockOption : module->quadraticGatesOnly ? clockOptionsQuadratic : clockOptionsAll) {
				std::string optionString = getClockOptionString(clockOption);
				MainClockScalingChildItem* clockItem = createMenuItem<MainClockScalingChildItem>(optionString,
				                                       CHECKMARK(module->mainClockMultDiv.multDiv == clockOption));

				clockItem->mainClockMulDiv = clockOption;
				clockItem->mainClockMulDivIndex = i;
				clockItem->module = module;
				menu->addChild(clockItem);
				++i;
			}

			return menu;
		}
	};

	struct QuadraticGatesMenuItem : MenuItem {
		Muxlicer* module;
		void onAction(const event::Action& e) override {
			module->quadraticGatesOnly ^= true;
			module->updateParamFromMainClockMultDiv();
		}
	};

	struct TapTempoItem : MenuItem {
		Muxlicer* module;
		void onAction(const event::Action& e) override {
			e.consume(NULL);
			module->tapped = true;
			e.unconsume();
		}
	};

	void appendContextMenu(Menu* menu) override {
		Muxlicer* module = dynamic_cast<Muxlicer*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuLabel<MenuLabel>("Clock Multiplication/Division"));

		if (module->usingExternalClock) {
			menu->addChild(createMenuLabel<MenuLabel>("Using external clock"));
		}
		else {
			TapTempoItem* tapTempoItem = createMenuItem<TapTempoItem>("Tap to set internal tempo...");
			tapTempoItem->module = module;
			menu->addChild(tapTempoItem);
		}


		MainClockScalingItem* mainClockScaleItem = createMenuItem<MainClockScalingItem>("Input clock", "▸");
		mainClockScaleItem->module = module;
		menu->addChild(mainClockScaleItem);

		OutputClockScalingItem* outputClockScaleItem = createMenuItem<OutputClockScalingItem>("Output clock", "▸");
		outputClockScaleItem->module = module;
		menu->addChild(outputClockScaleItem);

		QuadraticGatesMenuItem* quadraticGatesItem = createMenuItem<QuadraticGatesMenuItem>("Quadratic only mode", CHECKMARK(module->quadraticGatesOnly));
		quadraticGatesItem->module = module;
		menu->addChild(quadraticGatesItem);

		menu->addChild(new MenuSeparator());

		if (module->modeCOMIO == Muxlicer::COM_8_IN_1_OUT) {
			OutputRangeItem* outputRangeItem = createMenuItem<OutputRangeItem>("All In Normalled Value", "▸");
			outputRangeItem->module = module;
			menu->addChild(outputRangeItem);
		}
		else {
			menu->addChild(createMenuLabel<MenuLabel>("All In Normalled Value (disabled)"));
		}

		menu->addChild(createBoolPtrMenuItem("Output clock follows play/stop", "", &module->outputClockFollowsPlayMode));

		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuLabel<MenuLabel>("Input/Output mode"));

		IOMenuItem* ioItem = createMenuItem<IOMenuItem>("1 input ▸ 8 outputs",
		                     CHECKMARK(module->modeCOMIO == Muxlicer::COM_1_IN_8_OUT));
		ioItem->module = module;
		ioItem->widget = this;
		menu->addChild(ioItem);

		OIMenuItem* oiItem = createMenuItem<OIMenuItem>("8 inputs ▸ 1 output",
		                     CHECKMARK(module->modeCOMIO == Muxlicer::COM_8_IN_1_OUT));
		oiItem->module = module;
		oiItem->widget = this;
		menu->addChild(oiItem);
	}

	void clearCables() {
		for (int i = Muxlicer::MUX_OUTPUTS; i <= Muxlicer::MUX_OUTPUTS_LAST; ++i) {
			APP->scene->rack->clearCablesOnPort(this->getOutput(i));
		}
		APP->scene->rack->clearCablesOnPort(this->getInput(Muxlicer::COM_INPUT));
		APP->scene->rack->clearCablesOnPort(this->getInput(Muxlicer::ALL_INPUT));

		for (int i = Muxlicer::MUX_INPUTS; i <= Muxlicer::MUX_INPUTS_LAST; ++i) {
			APP->scene->rack->clearCablesOnPort(this->getInput(i));
		}
		APP->scene->rack->clearCablesOnPort(this->getOutput(Muxlicer::COM_OUTPUT));
		APP->scene->rack->clearCablesOnPort(this->getOutput(Muxlicer::ALL_OUTPUT));
	}

	// set ports visibility, either for 1 input -> 8 outputs or 8 inputs -> 1 output
	void updatePortVisibilityForIOMode(Muxlicer::ModeCOMIO mode) {

		bool visibleToggle = (mode == Muxlicer::COM_1_IN_8_OUT);

		for (int i = Muxlicer::MUX_OUTPUTS; i <= Muxlicer::MUX_OUTPUTS_LAST; ++i) {
			this->getOutput(i)->visible = visibleToggle;
		}
		this->getInput(Muxlicer::COM_INPUT)->visible = visibleToggle;
		this->getOutput(Muxlicer::ALL_OUTPUT)->visible = visibleToggle;

		for (int i = Muxlicer::MUX_INPUTS; i <= Muxlicer::MUX_INPUTS_LAST; ++i) {
			this->getInput(i)->visible = !visibleToggle;
		}
		this->getOutput(Muxlicer::COM_OUTPUT)->visible = !visibleToggle;
		this->getInput(Muxlicer::ALL_INPUT)->visible = !visibleToggle;
	}

};


Model* modelMuxlicer = createModel<Muxlicer, MuxlicerWidget>("Muxlicer");


// Mex


struct Mex : Module {
	static const int numSteps = 8;
	enum ParamIds {
		ENUMS(STEP_PARAM, numSteps),
		NUM_PARAMS
	};
	enum InputIds {
		GATE_IN_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(LED, numSteps),
		NUM_LIGHTS
	};
	enum StepState {
		GATE_IN_MODE,
		MUTE_MODE,
		MUXLICER_MODE
	};

	dsp::SchmittTrigger gateInTrigger;

	Mex() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		for (int i = 0; i < 8; ++i) {
			configSwitch(STEP_PARAM + i, 0.f, 2.f, 0.f, string::f("Step %d", i + 1), {"Gate in/Clock Out", "Muted", "All Gates"});
		}
	}

	Muxlicer* findHostModulePtr(Module* module) {
		if (module) {
			if (module->leftExpander.module) {
				// if it's Muxlicer, we're done
				if (module->leftExpander.module->model == modelMuxlicer) {
					return reinterpret_cast<Muxlicer*>(module->leftExpander.module);
				}
				// if it's Mex, keep recursing
				else if (module->leftExpander.module->model == modelMex) {
					return findHostModulePtr(module->leftExpander.module);
				}
			}
		}

		return nullptr;
	}

	void process(const ProcessArgs& args) override {

		for (int i = 0; i < 8; i++) {
			lights[i].setBrightness(0.f);
		}

		Muxlicer const* mother = findHostModulePtr(this);

		if (mother) {

			float gate = 0.f;

			if (mother->playState != Muxlicer::STATE_STOPPED) {
				const int currentStep = clamp(mother->addressIndex, 0, 7);
				StepState state = (StepState) params[STEP_PARAM + currentStep].getValue();
				if (state == MUXLICER_MODE) {
					gate = mother->isAllGatesOutHigh;
				}
				else if (state == GATE_IN_MODE) {
					// gate in will convert non-gate signals to gates (via schmitt trigger)
					// if input is present
					if (inputs[GATE_IN_INPUT].isConnected()) {
						gateInTrigger.process(inputs[GATE_IN_INPUT].getVoltage(), 0.1, 2.0);
						gate = gateInTrigger.isHigh();
					}
					// otherwise the main Muxlicer output clock (including divisions/multiplications)
					// is normalled in
					else {
						gate = mother->isOutputClockHigh;
					}
				}

				lights[currentStep].setBrightness(gate);
			}

			outputs[OUT_OUTPUT].setVoltage(gate * 10.f);
		}
	}
};


struct MexWidget : ModuleWidget {
	MexWidget(Mex* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/Mex.svg")));

		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<BefacoSwitchHorizontal>(mm2px(Vec(8.088, 13.063)),  module, Mex::STEP_PARAM + 0));
		addParam(createParamCentered<BefacoSwitchHorizontal>(mm2px(Vec(8.088, 25.706)),  module, Mex::STEP_PARAM + 1));
		addParam(createParamCentered<BefacoSwitchHorizontal>(mm2px(Vec(8.088, 38.348)),  module, Mex::STEP_PARAM + 2));
		addParam(createParamCentered<BefacoSwitchHorizontal>(mm2px(Vec(8.088, 50.990)),  module, Mex::STEP_PARAM + 3));
		addParam(createParamCentered<BefacoSwitchHorizontal>(mm2px(Vec(8.088, 63.632)),  module, Mex::STEP_PARAM + 4));
		addParam(createParamCentered<BefacoSwitchHorizontal>(mm2px(Vec(8.088, 76.274)),  module, Mex::STEP_PARAM + 5));
		addParam(createParamCentered<BefacoSwitchHorizontal>(mm2px(Vec(8.088, 88.916)),  module, Mex::STEP_PARAM + 6));
		addParam(createParamCentered<BefacoSwitchHorizontal>(mm2px(Vec(8.088, 101.559)), module, Mex::STEP_PARAM + 7));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(4.978, 113.445)), module, Mex::GATE_IN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.014, 113.4)), module, Mex::OUT_OUTPUT));

		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(17.7, 13.063)),  module, Mex::LED + 0));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(17.7, 25.706)),  module, Mex::LED + 1));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(17.7, 38.348)),  module, Mex::LED + 2));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(17.7, 50.990)),  module, Mex::LED + 3));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(17.7, 63.632)),  module, Mex::LED + 4));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(17.7, 76.274)),  module, Mex::LED + 5));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(17.7, 88.916)),  module, Mex::LED + 6));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(17.7, 101.558)), module, Mex::LED + 7));
	}
};


Model* modelMex = createModel<Mex, MexWidget>("Mex");
