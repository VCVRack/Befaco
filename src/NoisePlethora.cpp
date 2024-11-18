#include "plugin.hpp"
#include "noise-plethora/plugins/NoisePlethoraPlugin.hpp"
#include "noise-plethora/plugins/ProgramSelector.hpp"

enum FilterMode {
	LOWPASS,
	HIGHPASS,
	BANDPASS,
	NUM_TYPES
};


// based on Chapter 4 of THE ART OF VA FILTER DESIGN and
// Chap 12.4 of "Designing Audio Effect Plugins in C++" Will Pirkle
class StateVariableFilter2ndOrder {
public:

	StateVariableFilter2ndOrder() {
		setParameters(0.f, M_SQRT1_2);
	}

	void setParameters(float fc, float q) {
		// avoid repeated evaluations of tanh if not needed
		if (fc != fcCached || q != qCached) {

			fcCached = fc;
			qCached = q;

			const double g = std::tan(M_PI * fc);
			const double R = 1.0f / (2 * q);

			alpha0 = 1.0 / (1.0 + 2.0 * R * g + g * g);
			alpha = g;
			rho = 2.0 * R + g;
		}
	}

	void process(float input) {
		hp = (input - rho * mem1 - mem2) * alpha0;
		bp = alpha * hp + mem1;
		lp = alpha * bp + mem2;
		mem1 = alpha * hp + bp;
		mem2 = alpha * bp + lp;
	}

	float output(FilterMode mode) {
		switch (mode) {
			case LOWPASS: return lp;
			case HIGHPASS: return hp;
			case BANDPASS: return bp;
			default: return 0.0;
		}
	}

private:
	float alpha, alpha0, rho;

	float fcCached = -1.f, qCached = -1.f;

	float hp = 0.0f, bp = 0.0f, lp = 0.0f, mem1 = 0.0f, mem2 = 0.0f;
};

class StateVariableFilter4thOrder {

public:
	StateVariableFilter4thOrder() {

	}

	void setParameters(float fc, float q) {
		float rootQ = std::sqrt(q);
		stage1.setParameters(fc, rootQ);
		stage2.setParameters(fc, rootQ);
	}

	float process(float input, FilterMode mode) {
		stage1.process(input);
		float s1 = stage1.output(mode);

		stage2.process(s1);
		float s2 = stage2.output(mode);

		return s2;
	}

private:
	StateVariableFilter2ndOrder stage1, stage2;
};


struct NoisePlethora : Module {
	enum ParamIds {
		// A params
		X_A_PARAM,
		Y_A_PARAM,
		CUTOFF_A_PARAM,
		RES_A_PARAM,
		CUTOFF_CV_A_PARAM,
		FILTER_TYPE_A_PARAM,
		// misc
		PROGRAM_PARAM,
		// B params
		X_B_PARAM,
		Y_B_PARAM,
		CUTOFF_B_PARAM,
		RES_B_PARAM,
		CUTOFF_CV_B_PARAM,
		FILTER_TYPE_B_PARAM,
		// C params
		GRIT_PARAM,
		RES_C_PARAM,
		CUTOFF_C_PARAM,
		CUTOFF_CV_C_PARAM,
		FILTER_TYPE_C_PARAM,
		SOURCE_C_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		X_A_INPUT,
		Y_A_INPUT,
		CUTOFF_A_INPUT,
		PROG_A_INPUT,
		PROG_B_INPUT,
		CUTOFF_B_INPUT,
		X_B_INPUT,
		Y_B_INPUT,
		GRIT_INPUT,
		CUTOFF_C_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		A_OUTPUT,
		B_OUTPUT,
		GRITTY_OUTPUT,
		FILTERED_OUTPUT,
		WHITE_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		BANK_LIGHT,
		NUM_LIGHTS
	};

	enum Section {
		SECTION_A,
		SECTION_B,
		SECTION_C,
		NUM_SECTIONS
	};

	enum ProgramKnobMode {
		PROGRAM_MODE,
		BANK_MODE
	};
	ProgramKnobMode programKnobMode = PROGRAM_MODE;
	// one full turn of the program knob corresponds to an increment of dialResolution to the bank/program
	static constexpr int dialResolution = 8;
	// variable to store what the program knob was prior to the start of dragging (used to calculate deltas)
	float programKnobReferenceState = 0.f;

	// section A/B
	bool bypassFilters = false;
	std::shared_ptr<NoisePlethoraPlugin> algorithm[2] {nullptr, nullptr}; 	// pointer to actual algorithm
	std::string_view algorithmName[2] {"", ""};				// variable to cache which algorithm is active (after program CV applied)
	std::map<std::string_view, std::shared_ptr<NoisePlethoraPlugin>> A_algorithms{};
	std::map<std::string_view, std::shared_ptr<NoisePlethoraPlugin>> B_algorithms{};

	// filters for A/B
	StateVariableFilter2ndOrder svfFilter[2];
	bool blockDC = true;
	DCBlocker blockDCFilter[3];

	ProgramSelector programSelector; 		// tracks banks and programs for both sections A/B, including which is the "active" section
	ProgramSelector programSelectorWithCV; 	// as above, but also with CV for program applied as an offset - works like Plaits Model CV input
	// UI / UX for A/B
	std::string textDisplayA = " ", textDisplayB = " ";
	bool isDisplayActiveA = false, isDisplayActiveB = false;
	bool programButtonHeld = false;
	bool programButtonDragged = false;
	dsp::BooleanTrigger programHoldTrigger;
	dsp::Timer programHoldTimer;

	dsp::PulseGenerator updateParamsTimer;
	const float updateTimeSecs = 0.0029f;

	// section C
	AudioSynthNoiseWhiteFloat whiteNoiseSource;
	AudioSynthNoiseGritFloat gritNoiseSource;
	StateVariableFilter4thOrder svfFilterC;
	FilterMode typeMappingSVF[3] = {LOWPASS, BANDPASS, HIGHPASS};

	NoisePlethora()  {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(X_A_PARAM, 0.f, 1.f, 0.5f, "XA");
		configParam(RES_A_PARAM, 0.f, 1.f, 0.f, "Resonance A");
		configParam(CUTOFF_A_PARAM, 0.f, 1.f, 1.f, "Cutoff A");
		configParam(Y_A_PARAM, 0.f, 1.f, 0.5f, "YA");
		configParam(CUTOFF_CV_A_PARAM, 0.f, 1.f, 0.f, "Cutoff CV A");
		configSwitch(FILTER_TYPE_A_PARAM, 0.f, 2.f, 0.f, "Filter type", {"Lowpass", "Bandpass", "Highpass"});
		configParam(PROGRAM_PARAM, -INFINITY, +INFINITY, 0.f, "Program/Bank selection");
		configSwitch(FILTER_TYPE_B_PARAM, 0.f, 2.f, 0.f, "Filter type", {"Lowpass", "Bandpass", "Highpass"});
		configParam(CUTOFF_CV_B_PARAM, 0.f, 1.f, 0.f, "Cutoff CV B");
		configParam(X_B_PARAM, 0.f, 1.f, 0.5f, "XB");
		configParam(CUTOFF_B_PARAM, 0.f, 1.f, 1.f, "Cutoff B");
		configParam(RES_B_PARAM, 0.f, 1.f, 0.f, "Resonance B");
		configParam(Y_B_PARAM, 0.f, 1.f, 0.5f, "YB");
		configSwitch(FILTER_TYPE_C_PARAM, 0.f, 2.f, 0.f, "Filter type", {"Lowpass", "Bandpass", "Highpass"});
		configParam(CUTOFF_C_PARAM, 0.f, 1.f, 1.f, "Cutoff C");
		configParam(GRIT_PARAM, 0.f, 1.f, 0.f, "Grit Quantity");
		configParam(RES_C_PARAM, 0.f, 1.f, 0.f, "Resonance C");
		configParam(CUTOFF_CV_C_PARAM, 0.f, 1.f, 0.f, "Cutoff CV C");
		configSwitch(SOURCE_C_PARAM, 0.f, 1.f, 0.f, "Filter source", {"Gritty", "White"});

		configInput(X_A_INPUT, "XA CV");
		configInput(Y_A_INPUT, "YA CV");
		configInput(CUTOFF_A_INPUT, "Cutoff CV A");
		configInput(PROG_A_INPUT, "Program select A");
		configInput(PROG_B_INPUT, "Program select B");
		configInput(CUTOFF_B_INPUT, "Cutoff CV B");
		configInput(X_B_INPUT, "XB CV");
		configInput(Y_B_INPUT, "YB CV");
		configInput(GRIT_INPUT, "Grit Quantity CV");
		configInput(CUTOFF_C_INPUT, "Cutoff CV C");

		configOutput(A_OUTPUT, "Algorithm A");
		configOutput(B_OUTPUT, "Algorithm B");
		configOutput(GRITTY_OUTPUT, "Gritty noise");
		configOutput(FILTERED_OUTPUT, "Filtered noise");
		configOutput(WHITE_OUTPUT, "White noise");

		configLight(BANK_LIGHT, "Bank mode");

		getInputInfo(PROG_A_INPUT)->description = "CV sums with active program (0.5V increments)";
		getInputInfo(PROG_B_INPUT)->description = "CV sums with active program (0.5V increments)";

		for (auto const& entry : MyFactory::Instance()->factoryFunctionRegistry) {
			A_algorithms[entry.first] = MyFactory::Instance()->Create(entry.first);
			B_algorithms[entry.first] = MyFactory::Instance()->Create(entry.first);
		}

		setAlgorithm(SECTION_B, "radioOhNo");
		setAlgorithm(SECTION_A, "radioOhNo");
		onSampleRateChange();
	}

	void onReset(const ResetEvent& e) override {
		setAlgorithm(SECTION_B, "radioOhNo");
		setAlgorithm(SECTION_A, "radioOhNo");
		Module::onReset(e);
	}

	void onSampleRateChange() override {
		// set ~20Hz DC blocker
		const float fc = 22.05f / APP->engine->getSampleRate();

		blockDCFilter[SECTION_A].setFrequency(fc);
		blockDCFilter[SECTION_B].setFrequency(fc);
		blockDCFilter[SECTION_C].setFrequency(fc);

		if (algorithm[SECTION_A]) {
			algorithm[SECTION_A]->init();
		}
		if (algorithm[SECTION_B]) {
			algorithm[SECTION_B]->init();
		}
	}

	void process(const ProcessArgs& args) override {

		// we only periodically update parameters of each algorithm (once per block, ~2.9ms at 44100Hz)
		bool updateParams = false;
		if (!updateParamsTimer.process(args.sampleTime)) {
			updateParams = true;
			updateParamsTimer.trigger(updateTimeSecs);
		}

		// process A, B and C
		processTopSection(SECTION_A, X_A_PARAM, Y_A_PARAM,
		                  FILTER_TYPE_A_PARAM, CUTOFF_A_PARAM, CUTOFF_CV_A_PARAM, RES_A_PARAM,
		                  PROG_A_INPUT, X_A_INPUT, Y_A_INPUT, CUTOFF_A_INPUT, A_OUTPUT, args, updateParams);
		processTopSection(SECTION_B, X_B_PARAM, Y_B_PARAM,
		                  FILTER_TYPE_B_PARAM, CUTOFF_B_PARAM, CUTOFF_CV_B_PARAM, RES_B_PARAM,
		                  PROG_B_INPUT, X_B_INPUT, Y_B_INPUT, CUTOFF_B_INPUT, B_OUTPUT, args, updateParams);
		processBottomSection(args);

		// UI
		updateDataForLEDDisplay();
		processProgramBankKnobLogic(args);
	}

	// process CV for section, specifically: work out the offset relative to the current
	// program and see if this is a new algorithm
	void processCVOffsets(Section SECTION, InputIds PROG_INPUT) {

		const int offset = 2 * inputs[PROG_INPUT].getVoltage();

		const int bank = programSelector.getSection(SECTION).getBank();
		const int numProgramsForBank = getBankForIndex(bank).getSize();

		const int programWithoutCV = programSelector.getSection(SECTION).getProgram();
		const int programWithCV = unsigned_modulo(programWithoutCV + offset, numProgramsForBank);

		// duplicate key settings to programSelectorWithCV (expect modified program)
		programSelectorWithCV.setMode(programSelector.getMode());
		programSelectorWithCV.getSection(SECTION).setBank(bank);
		programSelectorWithCV.getSection(SECTION).setProgram(programWithCV);

		std::string_view newAlgorithmName = programSelectorWithCV.getSection(SECTION).getCurrentProgramName();

		// this is just a caching check to avoid constantly re-initialisating the algorithms
		if (newAlgorithmName != algorithmName[SECTION]) {

			algorithm[SECTION] = SECTION == Section::SECTION_A ? A_algorithms[newAlgorithmName] : B_algorithms[newAlgorithmName];
			algorithmName[SECTION] = newAlgorithmName;

			if (algorithm[SECTION]) {
				algorithm[SECTION]->init();
			}
			else {
				DEBUG("WARNING: Failed to initialise %s in programSelector", newAlgorithmName.data());
			}
		}
	}

	// exactly the same for A and B
	void processTopSection(Section SECTION, ParamIds X_PARAM, ParamIds Y_PARAM, ParamIds FILTER_TYPE_PARAM,
	                       ParamIds CUTOFF_PARAM, ParamIds CUTOFF_CV_PARAM, ParamIds RES_PARAM,
	                       InputIds PROG_INPUT, InputIds X_INPUT, InputIds Y_INPUT, InputIds CUTOFF_INPUT, OutputIds OUTPUT,
	                       const ProcessArgs& args, bool updateParams) {

		// periodically work out how CV should modify the current sections algorithm
		if (updateParams) {
			processCVOffsets(SECTION, PROG_INPUT);
		}

		float out = 0.f;
		if (algorithm[SECTION] && outputs[OUTPUT].isConnected()) {
			float cvX = params[X_PARAM].getValue() + rescale(inputs[X_INPUT].getVoltage(), -10.f, +10.f, -1.f, 1.f);
			float cvY = params[Y_PARAM].getValue() + rescale(inputs[Y_INPUT].getVoltage(), -10.f, +10.f, -1.f, 1.f);

			// update parameters of the algorithm
			if (updateParams) {
				algorithm[SECTION]->process(clamp(cvX, 0.f, 1.f), clamp(cvY, 0.f, 1.f));
			}
			// process the audio graph
			out = algorithm[SECTION]->processGraph();
			// each algorithm has a specific gain factor
			out = out * programSelectorWithCV.getSection(SECTION).getCurrentProgramGain();

			// if filters are active
			if (!bypassFilters) {

				// set parameters
				const float freqCV = std::pow(params[CUTOFF_CV_PARAM].getValue(), 2) * inputs[CUTOFF_INPUT].getVoltage();
				const float pitch = rescale(params[CUTOFF_PARAM].getValue(), 0, 1, -5.5, +5.5) + freqCV;
				const float cutoff = clamp(dsp::FREQ_C4 * std::pow(2.f, pitch), 1.f, 20000.);
				const float cutoffNormalised = clamp(cutoff / args.sampleRate, 0.f, 0.49f);
				const float q = M_SQRT1_2 + std::pow(params[RES_PARAM].getValue(), 2) * 10.f;
				const FilterMode mode = typeMappingSVF[(int) params[FILTER_TYPE_PARAM].getValue()];
				svfFilter[SECTION].setParameters(cutoffNormalised, q);

				// apply filter
				svfFilter[SECTION].process(out);
				// and retrieve relevant output
				out = svfFilter[SECTION].output(mode);
			}

			if (blockDC) {
				// cascaded Biquad (4th order highpass at ~20Hz)
				out = blockDCFilter[SECTION].process(out);
			}
		}

		outputs[OUTPUT].setVoltage(Saturator<float>::process(out) * 5.f);
	}

	// process section C
	void processBottomSection(const ProcessArgs& args) {

		float gritCv = rescale(clamp(inputs[GRIT_INPUT].getVoltage(), -10.f, 10.f), -10.f, 10.f, -1.f, 1.f);
		float gritAmount = clamp(params[GRIT_PARAM].getValue() + gritCv, 0.f, 1.f);
		float gritFrequency = 0.1 + std::pow(gritAmount, 2) * 20000;
		gritNoiseSource.setDensity(gritFrequency);
		float gritNoise = gritNoiseSource.process(args.sampleTime);
		outputs[GRITTY_OUTPUT].setVoltage(gritNoise * 5.f);

		float whiteNoise = whiteNoiseSource.process();
		outputs[WHITE_OUTPUT].setVoltage(whiteNoise * 5.f);

		float out = 0.f;
		if (outputs[FILTERED_OUTPUT].isConnected() && !bypassFilters) {

			const float freqCV = std::pow(params[CUTOFF_CV_C_PARAM].getValue(), 2) * inputs[CUTOFF_C_INPUT].getVoltage();
			const float pitch = rescale(params[CUTOFF_C_PARAM].getValue(), 0, 1, -5.f, +6.4f) + freqCV;
			const float cutoff = clamp(dsp::FREQ_C4 * std::pow(2.f, pitch), 1.f, 44100. / 2.f);
			const float cutoffNormalised = clamp(cutoff / args.sampleRate, 0.f, 0.49f);
			const float Q = 0.5 + std::pow(params[RES_C_PARAM].getValue(), 2) * 20.f;
			const FilterMode mode = typeMappingSVF[(int) params[FILTER_TYPE_C_PARAM].getValue()];
			svfFilterC.setParameters(cutoffNormalised, Q);

			float toFilter = params[SOURCE_C_PARAM].getValue() ? whiteNoise : gritNoise;
			out = svfFilterC.process(toFilter, mode);

			// assymetric saturator, to get those lovely even harmonics
			out = Saturator<float>::process(out + 0.33);

			if (blockDC) {
				// cascaded Biquad (4th order highpass at ~20Hz)
				out = blockDCFilter[SECTION_C].process(out);
			}
		}
		else if (bypassFilters) {
			out = params[SOURCE_C_PARAM].getValue() ? whiteNoise : gritNoise;
		}

		outputs[FILTERED_OUTPUT].setVoltage(out * 5.f);
	}

	// set which text NoisePlethoraWidget should display on the 7 segment display
	void updateDataForLEDDisplay() {

		if (programKnobMode == PROGRAM_MODE) {
			textDisplayA = std::to_string(programSelectorWithCV.getA().getProgram());
		}
		else if (programKnobMode == BANK_MODE) {
			textDisplayA = 'A' + programSelectorWithCV.getA().getBank();
		}
		isDisplayActiveA = programSelectorWithCV.getMode() == SECTION_A;

		if (programKnobMode == PROGRAM_MODE) {
			textDisplayB = std::to_string(programSelectorWithCV.getB().getProgram());
		}
		else if (programKnobMode == BANK_MODE) {
			textDisplayB = 'A' + programSelectorWithCV.getB().getBank();
		}
		isDisplayActiveB = programSelectorWithCV.getMode() == SECTION_B;
	}

	// handle convoluted logic for the multifunction Program knob
	void processProgramBankKnobLogic(const ProcessArgs& args) {

		// program knob will either change program for current bank...
		if (programButtonDragged) {
			// work out the change (in discrete increments) since the program/bank knob started being dragged
			const int delta = (int)(dialResolution * (params[PROGRAM_PARAM].getValue() - programKnobReferenceState));

			if (programKnobMode == PROGRAM_MODE) {
				const int numProgramsForCurrentBank = getBankForIndex(programSelector.getCurrent().getBank()).getSize();

				if (delta != 0) {
					const int newProgramFromKnob = unsigned_modulo(programSelector.getCurrent().getProgram() + delta, numProgramsForCurrentBank);
					programKnobReferenceState = params[PROGRAM_PARAM].getValue();
					setAlgorithmViaProgram(newProgramFromKnob);
				}
			}
			// ...or change bank, (trying to) keep program the same
			else {

				if (delta != 0) {
					const int newBankFromKnob = unsigned_modulo(programSelector.getCurrent().getBank() + delta, numBanks);
					programKnobReferenceState = params[PROGRAM_PARAM].getValue();
					setAlgorithmViaBank(newBankFromKnob);
				}
			}
		}

		// if we have a new "press" on the knob, time it
		if (programHoldTrigger.process(programButtonHeld)) {
			programHoldTimer.reset();
		}

		// but cancel if it's actually a knob drag
		if (programButtonDragged) {
			programHoldTimer.reset();
			programButtonHeld = false;
		}
		else {
			if (programButtonHeld) {
				programHoldTimer.process(args.sampleTime);

				// if we've held for at least 0.5 seconds, switch into "bank mode"
				if (programHoldTimer.time > 0.5f) {
					programButtonHeld = false;
					programHoldTimer.reset();

					if (programKnobMode == PROGRAM_MODE) {
						programKnobMode = BANK_MODE;
					}
					else {
						programKnobMode = PROGRAM_MODE;
					}

					lights[BANK_LIGHT].setBrightness(programKnobMode == BANK_MODE);
				}
			}
			// no longer held, but has been held for non-zero time (without being dragged), toggle "active" section (A or B),
			// this is effectively just a "click"
			else if (programHoldTimer.time > 0.f) {
				programSelector.setMode(!programSelector.getMode());
				programHoldTimer.reset();
			}
		}

		if (!programButtonDragged) {
			programKnobReferenceState = params[PROGRAM_PARAM].getValue();
		}
	}

	void setAlgorithmViaProgram(int newProgram) {

		const int currentBank = programSelector.getCurrent().getBank();
		std::string_view algorithmName = getBankForIndex(currentBank).getProgramName(newProgram);
		const int section = programSelector.getMode();

		setAlgorithm(section, algorithmName);
	}

	void setAlgorithmViaBank(int newBank) {
		newBank = clamp(newBank, 0, numBanks - 1);
		const int currentProgram = programSelector.getCurrent().getProgram();
		// the new bank may not have as many algorithms
		const int currentProgramInNewBank = clamp(currentProgram, 0, getBankForIndex(newBank).getSize() - 1);
		const std::string_view algorithmName = getBankForIndex(newBank).getProgramName(currentProgramInNewBank);
		const int section = programSelector.getMode();

		setAlgorithm(section, algorithmName);
	}

	void setAlgorithm(int section, std::string_view algorithmName) {

		if (section > 1) {
			return;
		}

		for (int bank = 0; bank < numBanks; ++bank) {
			for (int program = 0; program < getBankForIndex(bank).getSize(); ++program) {
				if (getBankForIndex(bank).getProgramName(program) == algorithmName) {
					programSelector.setMode(section);
					programSelector.getCurrent().setBank(bank);
					programSelector.getCurrent().setProgram(program);

					return;
				}
			}
		}

		DEBUG("WARNING: Didn't find %s in programSelector", algorithmName.data());
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* bankAJ = json_object_get(rootJ, "algorithmA");
		if (bankAJ) {
			setAlgorithm(SECTION_A, json_string_value(bankAJ));
		}

		json_t* bankBJ = json_object_get(rootJ, "algorithmB");
		if (bankBJ) {
			setAlgorithm(SECTION_B, json_string_value(bankBJ));
		}

		json_t* bypassFiltersJ = json_object_get(rootJ, "bypassFilters");
		if (bypassFiltersJ) {
			bypassFilters = json_boolean_value(bypassFiltersJ);
		}

		json_t* blockDCJ = json_object_get(rootJ, "blockDC");
		if (blockDCJ) {
			blockDC = json_boolean_value(blockDCJ);
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();

		json_object_set_new(rootJ, "algorithmA", json_string(programSelector.getA().getCurrentProgramName().data()));
		json_object_set_new(rootJ, "algorithmB", json_string(programSelector.getB().getCurrentProgramName().data()));

		json_object_set_new(rootJ, "bypassFilters", json_boolean(bypassFilters));
		json_object_set_new(rootJ, "blockDC", json_boolean(blockDC));

		return rootJ;
	}
};


struct BefacoTinyKnobSnapPress : BefacoTinyKnobBlack {
	BefacoTinyKnobSnapPress() {	}

	// this seems convoluted but I can't see how to achieve the following (say) with onAction:
	// a) need to support standard knob dragging behaviour
	// b) need to support click detection (not drag)
	// c) need to distinguish short click from long click
	//
	// To achieve this we have 2 state variables, held and dragged. The Module thread will
	// time how long programButtonHeld is "true" and switch between bank and program mode if
	// longer than X secs. A drag, as measured here as a displacement greater than Y, will cancel
	// this timer and we're back to normal param mode. See processProgramBankKnobLogic(...) for details
	void onDragStart(const DragStartEvent& e) override {
		NoisePlethora* noisePlethoraModule = static_cast<NoisePlethora*>(module);
		if (noisePlethoraModule) {
			noisePlethoraModule->programButtonHeld = true;
			noisePlethoraModule->programButtonDragged = false;
		}

		pos = Vec(0, 0);
		Knob::onDragStart(e);
	}

	void onDragMove(const DragMoveEvent& e) override {
		pos += e.mouseDelta;
		NoisePlethora* noisePlethoraModule = static_cast<NoisePlethora*>(module);

		// cancel if we're doing a drag
		if (noisePlethoraModule && std::sqrt(pos.square()) > 5) {
			noisePlethoraModule->programButtonHeld = false;
			noisePlethoraModule->programButtonDragged = true;
		}

		Knob::onDragMove(e);
	}

	void onDragEnd(const DragEndEvent& e) override {

		NoisePlethora* noisePlethoraModule = static_cast<NoisePlethora*>(module);
		if (noisePlethoraModule) {
			noisePlethoraModule->programButtonHeld = false;
		}

		Knob::onDragEnd(e);
	}

	// suppress double click
	void onDoubleClick(const DoubleClickEvent& e) override {}

	Vec pos;
};

// dervied from https://github.com/countmodula/VCVRackPlugins/blob/v2.0.0/src/components/CountModulaLEDDisplay.hpp
struct NoisePlethoraLEDDisplay : LightWidget {
	float fontSize = 28;
	Vec textPos = Vec(2, 25);
	int numChars = 1;
	bool activeDisplay = true;
	NoisePlethora* module;
	NoisePlethora::Section section = NoisePlethora::SECTION_A;
	ui::Tooltip* tooltip;

	NoisePlethoraLEDDisplay() {
		box.size = mm2px(Vec(7.236, 10));
	}

	void onEnter(const EnterEvent& e) override {
		LightWidget::onEnter(e);
		setTooltip();
	}

	void setTooltip() {
		std::string_view activeName = module->programSelector.getSection(section).getCurrentProgramName();
		tooltip = new ui::Tooltip;
		tooltip->text = activeName;
		APP->scene->addChild(tooltip);
	}

	void onHover(const event::Hover& e) override {
		LightWidget::onHover(e);
		e.consume(this);
	}

	void onLeave(const event::Leave& e) override {
		LightWidget::onLeave(e);

		APP->scene->removeChild(tooltip);
		this->tooltip->requestDelete();
		this->tooltip = NULL;
	}

	void setCentredPos(Vec pos) {
		box.pos.x = pos.x - box.size.x / 2;
		box.pos.y = pos.y - box.size.y / 2;
	}

	void drawBackground(const DrawArgs& args) override {
		// Background
		NVGcolor backgroundColor = nvgRGB(0x24, 0x14, 0x14);
		NVGcolor borderColor = nvgRGB(0x10, 0x10, 0x10);
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0.0, 0.0, box.size.x, box.size.y, 2.0);
		nvgFillColor(args.vg, backgroundColor);
		nvgFill(args.vg);
		nvgStrokeWidth(args.vg, 1.0);
		nvgStrokeColor(args.vg, borderColor);
		nvgStroke(args.vg);
	}

	void drawLight(const DrawArgs& args) override {
		// Background
		NVGcolor backgroundColor = nvgRGB(0x24, 0x14, 0x14);
		NVGcolor borderColor = nvgRGB(0x10, 0x10, 0x10);
		NVGcolor textColor = nvgRGB(0xaa, 0x20, 0x20);

		// use slightly different LED colour if CV is connected as visual cue
		if (module) {
			NoisePlethora::InputIds inputId = (section == NoisePlethora::SECTION_A) ? NoisePlethora::PROG_A_INPUT : NoisePlethora::PROG_B_INPUT;
			if (module->inputs[inputId].isConnected()) {
				textColor = nvgRGB(0xff, 0x40, 0x40);
			}
		}

		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0.0, 0.0, box.size.x, box.size.y, 2.0);
		nvgFillColor(args.vg, backgroundColor);
		nvgFill(args.vg);
		nvgStrokeWidth(args.vg, 1.0);
		nvgStrokeColor(args.vg, borderColor);
		nvgStroke(args.vg);

		std::shared_ptr<Font> font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/Segment7Standard.otf"));

		if (font && font->handle >= 0) {

			std::string text = "A";  // fallback if module not yet defined
			if (module) {
				text = (section == NoisePlethora::SECTION_A) ? module->textDisplayA : module->textDisplayB;
			}
			char buffer[numChars + 1];
			int l = text.size();
			if (l > numChars)
				l = numChars;

			nvgGlobalTint(args.vg, color::WHITE);

			text.copy(buffer, l);
			buffer[numChars] = '\0';

			nvgFontSize(args.vg, fontSize);
			nvgFontFaceId(args.vg, font->handle);
			nvgTextLetterSpacing(args.vg, 1);

			// render the "off" segments
			nvgFillColor(args.vg, nvgTransRGBA(textColor, 18));
			nvgText(args.vg, textPos.x, textPos.y, "8", NULL);

			// render the "on segments"
			nvgFillColor(args.vg, textColor);

			nvgText(args.vg, textPos.x, textPos.y, buffer, NULL);
		}

		if (module) {
			const bool isSectionDisplayActive = (section == NoisePlethora::SECTION_A) ? module->isDisplayActiveA : module->isDisplayActiveB;

			// active bank dot
			nvgBeginPath(args.vg);
			nvgCircle(args.vg, 18, 26, 1.5);
			nvgFillColor(args.vg, isSectionDisplayActive ? textColor : nvgTransRGBA(textColor, 18));
			nvgFill(args.vg);
		}
	}
};

struct NoisePlethoraWidget : ModuleWidget {
	NoisePlethoraWidget(NoisePlethora* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/NoisePlethora.svg")));

		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<Knurlie>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// A params
		addParam(createParamCentered<BefacoTinyKnobDarkGrey>(mm2px(Vec(22.325, 16.09)), module, NoisePlethora::X_A_PARAM));
		addParam(createParamCentered<BefacoTinyKnobDarkGrey>(mm2px(Vec(22.325, 30.595)), module, NoisePlethora::Y_A_PARAM));
		addParam(createParamCentered<Davies1900hLargeGreyKnob>(mm2px(Vec(43.248, 23.058)), module, NoisePlethora::CUTOFF_A_PARAM));
		addParam(createParamCentered<BefacoTinyKnobDarkGrey>(mm2px(Vec(63.374, 16.09)), module, NoisePlethora::RES_A_PARAM));
		addParam(createParamCentered<BefacoTinyKnobDarkGrey>(mm2px(Vec(63.374, 30.595)), module, NoisePlethora::CUTOFF_CV_A_PARAM));
		addParam(createParam<CKSSNarrow3>(mm2px(Vec(41.494, 38.579)), module, NoisePlethora::FILTER_TYPE_A_PARAM));

		// (bank)
		addParam(createParamCentered<BefacoTinyKnobSnapPress>(mm2px(Vec(30.866, 49.503)), module, NoisePlethora::PROGRAM_PARAM));

		// B params
		addParam(createParamCentered<BefacoTinyKnobLightGrey>(mm2px(Vec(22.345, 68.408)), module, NoisePlethora::X_B_PARAM));
		addParam(createParamCentered<BefacoTinyKnobLightGrey>(mm2px(Vec(22.345, 82.695)), module, NoisePlethora::Y_B_PARAM));
		addParam(createParamCentered<Davies1900hLargeLightGreyKnob>(mm2px(Vec(43.248, 75.551)), module, NoisePlethora::CUTOFF_B_PARAM));
		addParam(createParamCentered<BefacoTinyKnobLightGrey>(mm2px(Vec(63.383, 82.686)), module, NoisePlethora::RES_B_PARAM));
		addParam(createParamCentered<BefacoTinyKnobLightGrey>(mm2px(Vec(63.36, 68.388)), module, NoisePlethora::CUTOFF_CV_B_PARAM));
		addParam(createParam<CKSSNarrow3>(mm2px(Vec(41.494, 53.213)), module, NoisePlethora::FILTER_TYPE_B_PARAM));

		// C params
		addParam(createParamCentered<BefacoTinyKnobBlack>(mm2px(Vec(7.6, 99.584)), module, NoisePlethora::GRIT_PARAM));
		addParam(createParamCentered<BefacoTinyKnobDarkGrey>(mm2px(Vec(22.366, 99.584)), module, NoisePlethora::RES_C_PARAM));
		addParam(createParamCentered<Davies1900hDarkGreyKnob>(mm2px(Vec(47.536, 98.015)), module, NoisePlethora::CUTOFF_C_PARAM));
		addParam(createParamCentered<BefacoTinyKnobDarkGrey>(mm2px(Vec(63.36, 99.584)), module, NoisePlethora::CUTOFF_CV_C_PARAM));
		addParam(createParam<CKSSNarrow3>(mm2px(Vec(33.19, 92.506)), module, NoisePlethora::FILTER_TYPE_C_PARAM));
		addParam(createParam<CKSSHoriz2>(mm2px(Vec(31.707, 104.225)), module, NoisePlethora::SOURCE_C_PARAM));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.431, 16.102)), module, NoisePlethora::X_A_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.431, 29.959)), module, NoisePlethora::Y_A_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(52.845, 42.224)), module, NoisePlethora::CUTOFF_A_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.431, 43.212)), module, NoisePlethora::PROG_A_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.431, 55.801)), module, NoisePlethora::PROG_B_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(52.845, 56.816)), module, NoisePlethora::CUTOFF_B_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.431, 68.398)), module, NoisePlethora::X_B_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.431, 82.729)), module, NoisePlethora::Y_B_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(7.555, 114.8)), module, NoisePlethora::GRIT_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(63.36, 114.8)), module, NoisePlethora::CUTOFF_C_INPUT));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(64.909, 44.397)), module, NoisePlethora::A_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(64.915, 54.608)), module, NoisePlethora::B_OUTPUT));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(22.345, 114.852)), module, NoisePlethora::GRITTY_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(34.981, 114.852)), module, NoisePlethora::FILTERED_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(47.536, 114.852)), module, NoisePlethora::WHITE_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(30.866, 37.422)), module, NoisePlethora::BANK_LIGHT));


		NoisePlethoraLEDDisplay* displayA = createWidget<NoisePlethoraLEDDisplay>(mm2px(Vec(13.106, 38.172)));
		displayA->module = module;
		displayA->section = NoisePlethora::SECTION_A;
		addChild(displayA);

		NoisePlethoraLEDDisplay* displayB = createWidget<NoisePlethoraLEDDisplay>(mm2px(Vec(13.106, 50.712)));
		displayB->module = module;
		displayB->section = NoisePlethora::SECTION_B;
		addChild(displayB);
	}

	void appendContextMenu(Menu* menu) override {
		NoisePlethora* module = dynamic_cast<NoisePlethora*>(this->module);
		assert(module);

		// build the two algorithm selection menus programmatically
		menu->addChild(createMenuLabel("Algorithms"));
		std::vector<std::string> bankAliases = {"Textures", "HH Clusters", "Harsh & Wild", "Test"};
		char programNames[] = "AB";
		for (int sectionId = 0; sectionId < 2; ++sectionId) {

			menu->addChild(createSubmenuItem(string::f("Program %c", programNames[sectionId]), "",
			[ = ](Menu * menu) {
				for (int i = 0; i < numBanks; i++) {
					const int currentBank = module->programSelector.getSection(sectionId).getBank();
					const int currentProgram = module->programSelector.getSection(sectionId).getProgram();

					menu->addChild(createSubmenuItem(string::f("Bank %d: %s", i + 1, bankAliases[i].c_str()), currentBank == i ? CHECKMARK_STRING : "", [ = ](Menu * menu) {
						for (int j = 0; j < getBankForIndex(i).getSize(); ++j) {
							const bool currentProgramAndBank = (currentProgram == j) && (currentBank == i);
							std::string_view algorithmName = getBankForIndex(i).getProgramName(j);

							bool implemented = false;
							for (auto item : MyFactory::Instance()->factoryFunctionRegistry) {
								if (item.first == algorithmName) {
									implemented = true;
									break;
								}
							}

							if (implemented) {
								menu->addChild(createMenuItem(algorithmName.data(), currentProgramAndBank ? CHECKMARK_STRING : "",
								[ = ]() {
									module->setAlgorithm(sectionId, algorithmName);
								}));
							}
							else {
								// placeholder text (greyed out)
								menu->addChild(createMenuLabel(algorithmName.data()));
							}
						}
					}));
				}
			}));


		}

		menu->addChild(createMenuLabel("Filters"));
		menu->addChild(createBoolPtrMenuItem("Remove DC", "", &module->blockDC));
		menu->addChild(createBoolPtrMenuItem("Bypass Filters", "", &module->bypassFilters));
	}
};


Model* modelNoisePlethora = createModel<NoisePlethora, NoisePlethoraWidget>("NoisePlethora");
