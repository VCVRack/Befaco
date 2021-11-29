#include "plugin.hpp"
#include "noise-plethora/plugins/PluginFactory.hpp"
#include "noise-plethora/plugins/ProgramSelector.hpp"

enum FilterMode {
	LOWPASS,
	HIGHPASS,
	BANDPASS,
	NUM_TYPES
};

class StateVariableFilter2ndOrder {
public:

	StateVariableFilter2ndOrder() {

	}

	void setParameters(float fc, float q) {
		// avoid repeated evaluations of tanh if not needed
		if (fc != fcCached) {
			g = std::tan(M_PI * fc);
			fcCached = fc;
		}
		R = 1.0f / q;
	}

	void process(float input) {
		hp = (input - (R + g) * mem1 - mem2) / (1.0f + g * (R + g));
		bp = g * hp + mem1;
		lp = g * bp + mem2;
		mem1 = g * hp + bp;
		mem2 = g * bp + lp;
	}

	float output(FilterMode mode) {
		switch (mode) {
			case LOWPASS: return lp;
			case HIGHPASS: return hp;
			case BANDPASS: return bp * 2.f * R;
			default: return 0.0;
		}
	}

private:
	float g = 0.f;
	float R = 0.5f;

	float fcCached = -1.f;

	float hp = 0.0f, bp = 0.0f, lp = 0.0f, mem1 = 0.0f, mem2 = 0.0f;
};

class StateVariableFilter4thOrder {

public:
	StateVariableFilter4thOrder() {

	}

	void setParameters(float fc, float q) {
		stage1.setParameters(fc, 1.0);
		stage2.setParameters(fc, q);
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
		A_OUTPUT_ALT,
		B_OUTPUT,
		B_OUTPUT_ALT,
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

	// section A/B
	bool applyFilters = false;
	std::shared_ptr<NoisePlethoraPlugin> algorithm[2];
	// filters for A/B
	StateVariableFilter2ndOrder svfFilter[2];


	// section C
	AudioSynthNoiseWhiteFloat whiteNoiseSource;
	AudioSynthNoiseGritFloat gritNoiseSource;
	StateVariableFilter4thOrder svfFilterC;
	FilterMode typeMappingSVF[3] = {LOWPASS, BANDPASS, HIGHPASS};

	ProgramSelector programSelector;

	std::string textDisplayA = " ", textDisplayB = " ";
	bool isDisplayActiveA = false, isDisplayActiveB = false;

	bool programButtonHeld = false;
	bool programButtonDragged = false;
	dsp::BooleanTrigger programHoldTrigger;
	dsp::Timer programHoldTimer;

	dsp::PulseGenerator updateParamsTimer;
	const float updateTimeSecs = 0.0029f;

	NoisePlethora()  {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(X_A_PARAM, 0.f, 1.f, 0.5f, "XA");
		configParam(RES_A_PARAM, 0.f, 1.f, 0.f, "Resonance A");
		configParam(CUTOFF_A_PARAM, 0.f, 1.f, 0.f, "Cutoff A");
		configParam(Y_A_PARAM, 0.f, 1.f, 0.5f, "YA");
		configParam(CUTOFF_CV_A_PARAM, 0.f, 1.f, 0.f, "Cutoff CV A");
		configSwitch(FILTER_TYPE_A_PARAM, 0.f, 2.f, 0.f, "Filter type", {"Lowpass", "Bandpass", "Highpass"});
		configParam(PROGRAM_PARAM, 0.f, 10.f, 0.f, "Program/Bank selection");
		configSwitch(FILTER_TYPE_B_PARAM, 0.f, 2.f, 0.f, "Filter type", {"Lowpass", "Bandpass", "Highpass"});
		configParam(CUTOFF_CV_B_PARAM, 0.f, 1.f, 0.f, "Cutoff B");
		configParam(X_B_PARAM, 0.f, 1.f, 0.5f, "XB");
		configParam(CUTOFF_B_PARAM, 0.f, 1.f, 0.f, "Cutoff CV B");
		configParam(RES_B_PARAM, 0.f, 1.f, 0.f, "Resonance B");
		configParam(Y_B_PARAM, 0.f, 1.f, 0.5f, "YB");
		configSwitch(FILTER_TYPE_C_PARAM, 0.f, 2.f, 0.f, "Filter type", {"Lowpass", "Bandpass", "Highpass"});
		configParam(CUTOFF_C_PARAM, 0.f, 1.f, 0.f, "Cutoff C");
		configParam(GRIT_PARAM, 0.f, 1.f, 0.f, "Grit Quantity");
		configParam(RES_C_PARAM, 0.f, 1.f, 0.f, "Resonance C");
		configParam(CUTOFF_CV_C_PARAM, 0.f, 1.f, 0.f, "Cutoff CV C");
		configSwitch(SOURCE_C_PARAM, 0.f, 1.f, 0.f, "Filter source", {"Gritty", "White"});

		setAlgorithm(SECTION_A, "TestPlugin");
		setAlgorithm(SECTION_B, "TestPlugin");

		for (auto item : MyFactory::Instance()->factoryFunctionRegistry) {
			DEBUG(string::f("found plugin: %s", item.first.c_str()).c_str());
		}
		DEBUG("Constructor complete!");
	}

	void process(const ProcessArgs& args) override {

		if (applyFilters) {
			const float freqCV = std::pow(params[CUTOFF_CV_A_PARAM].getValue(), 2) * inputs[CUTOFF_A_INPUT].getVoltage();
			const float pitch = rescale(params[CUTOFF_A_PARAM].getValue(), 0, 1, -5, +5) + freqCV;
			const float cutoff = clamp(dsp::FREQ_C4 * simd::pow(2.f, pitch), 1.f, 44100. / 4.f);
			const float cutoffNormalised = clamp(cutoff / args.sampleRate, 0.f, 0.5f);
			const float q = rescale(params[RES_A_PARAM].getValue(), 0.f, 1.f, 1.f, 10.f);
			svfFilter[SECTION_A].setParameters(cutoffNormalised, q);
		}

		if (applyFilters) {
			const float freqCV = std::pow(params[CUTOFF_CV_B_PARAM].getValue(), 2) * inputs[CUTOFF_B_INPUT].getVoltage();
			const float pitch = rescale(params[CUTOFF_B_PARAM].getValue(), 0, 1, -5, +5) + freqCV;
			const float cutoff = clamp(dsp::FREQ_C4 * simd::pow(2.f, pitch), 1.f, 44100. / 4.f);
			const float cutoffNormalised = clamp(cutoff / args.sampleRate, 0.f, 0.5f);
			const float q = rescale(params[RES_B_PARAM].getValue(), 0.f, 1.f, 1.f, 10.f);
			svfFilter[SECTION_B].setParameters(cutoffNormalised, q);
		}

		if (applyFilters) {
			const float freqCV = std::pow(params[CUTOFF_CV_C_PARAM].getValue(), 2) * inputs[CUTOFF_C_INPUT].getVoltage();
			const float pitch = rescale(params[CUTOFF_C_PARAM].getValue(), 0, 1, -6.f, +6.f) + freqCV;
			const float cutoff = clamp(dsp::FREQ_C4 * simd::pow(2.f, pitch), 1.f, 44100. / 2.f);
			const float cutoffNormalised = clamp(cutoff / args.sampleRate, 0.f, 0.49f);
			const float Q = rescale(params[RES_C_PARAM].getValue(), 0, 1, 1, 10);
			svfFilterC.setParameters(cutoffNormalised, Q);
		}


		float outA = 0.f;
		float outB = 0.f;

		float outAAlt = 0.f;
		float outBAlt = 0.f;

		// we only periodically update the process() call of each algorithm
		bool updateParams = false;
		if (!updateParamsTimer.process(args.sampleTime)) {
			updateParams = true;
			updateParamsTimer.trigger(updateTimeSecs);
		}

		if (algorithm[SECTION_A] && outputs[A_OUTPUT].isConnected()) {
			float cvX = params[X_A_PARAM].getValue() + rescale(inputs[X_A_INPUT].getVoltage(), -10.f, +10.f, -1.f, 1.f);
			float cvY = params[Y_A_PARAM].getValue() + rescale(inputs[Y_A_INPUT].getVoltage(), -10.f, +10.f, -1.f, 1.f);

			// update parameters of the algorithm
			if (updateParams) {
				algorithm[SECTION_A]->process(clamp(cvX, 0.f, 1.f), clamp(cvY, 0.f, 1.f));
			}
			// process the audio graph
			outA = algorithm[SECTION_A]->processGraph(args.sampleTime);
			outAAlt = algorithm[SECTION_A]->getAlternativeOutput();
			if (applyFilters) {
				svfFilter[SECTION_A].process(outA);
				FilterMode mode = typeMappingSVF[(int) params[FILTER_TYPE_A_PARAM].getValue()];
				outA = svfFilter[SECTION_A].output(mode);
			}
		}
		if (algorithm[SECTION_B] && outputs[B_OUTPUT].isConnected()) {
			float cvX = params[X_B_PARAM].getValue() + rescale(inputs[X_B_INPUT].getVoltage(), -10.f, +10.f, -1.f, 1.f);
			float cvY = params[Y_B_PARAM].getValue() + rescale(inputs[Y_B_INPUT].getVoltage(), -10.f, +10.f, -1.f, 1.f);

			// update parameters of the algorithm
			if (updateParams) {
				algorithm[SECTION_B]->process(clamp(cvX, 0.f, 1.f), clamp(cvY, 0.f, 1.f));
			}
			outB = algorithm[SECTION_B]->processGraph(args.sampleTime);
			outBAlt = algorithm[SECTION_B]->getAlternativeOutput();

			if (applyFilters) {
				svfFilter[SECTION_B].process(outB);
				FilterMode mode = typeMappingSVF[(int) params[FILTER_TYPE_B_PARAM].getValue()];
				outB = svfFilter[SECTION_B].output(mode);
			}

		}
		outputs[A_OUTPUT].setVoltage(outA * 5.f);
		outputs[B_OUTPUT].setVoltage(outB * 5.f);

		outputs[A_OUTPUT_ALT].setVoltage(outAAlt * 5.f);
		outputs[B_OUTPUT_ALT].setVoltage(outBAlt * 5.f);


		if (true /*outputs[FILTERED_OUTPUT].isConnected() */) {

			float gritCv = rescale(clamp(inputs[GRIT_INPUT].getVoltage(), -10.f, 10.f), -10.f, 10.f, -1.f, 1.f);
			float gritAmount = clamp(1.f - params[GRIT_PARAM].getValue() + gritCv, 0.f, 1.f);
			float gritFrequency = rescale(gritAmount, 0, 1, 0.1, 20000);
			gritNoiseSource.setDensity(gritFrequency);
			float gritNoise = gritNoiseSource.process(args.sampleTime);

			float whiteNoise = whiteNoiseSource.process();

			float toFilter = params[SOURCE_C_PARAM].getValue() ? whiteNoise : gritNoise;

			FilterMode mode = typeMappingSVF[(int) params[FILTER_TYPE_C_PARAM].getValue()];

			float filtered = svfFilterC.process(toFilter, mode);

			outputs[GRITTY_OUTPUT].setVoltage(gritNoise * 5.f);
			outputs[WHITE_OUTPUT].setVoltage(whiteNoise * 5.f);
			outputs[FILTERED_OUTPUT].setVoltage(filtered * 5.f);
		}

		updateDataForLEDDisplay();

		processProgramBankKnobLogic(args);

		checkForProgramChangeCV();
	}

	void updateDataForLEDDisplay() {

		if (programKnobMode == PROGRAM_MODE) {
			textDisplayA = std::to_string(programSelector.getA().getProgram());
		}
		else if (programKnobMode == BANK_MODE) {
			textDisplayA = 'A' + programSelector.getA().getBank();
		}
		isDisplayActiveA = programSelector.getMode() == SECTION_A;

		if (programKnobMode == PROGRAM_MODE) {
			textDisplayB = std::to_string(programSelector.getB().getProgram());
		}
		else if (programKnobMode == BANK_MODE) {
			textDisplayB = 'A' + programSelector.getB().getBank();
		}
		isDisplayActiveB = programSelector.getMode() == SECTION_B;
	}

	void processProgramBankKnobLogic(const ProcessArgs& args) {

		// program knob will either change program for current bank...
		if (programKnobMode == PROGRAM_MODE) {
			const int currentProgramFromKnob = params[PROGRAM_PARAM].getValue();
			if (currentProgramFromKnob != programSelector.getCurrent().getProgram()) {
				setAlgorithmViaProgram(currentProgramFromKnob);
			}
		}
		// ...or change bank, (trying to) keep program the same
		else {
			const int currentBankFromKnob = params[PROGRAM_PARAM].getValue();
			if (currentBankFromKnob != programSelector.getCurrent().getBank()) {
				setAlgorithmViaBank(currentBankFromKnob);
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

				// if we've held for at least 0.75 seconds, switch into "bank mode"
				if (programHoldTimer.time > 0.75f) {
					programButtonHeld = false;
					programHoldTimer.reset();

					if (programKnobMode == PROGRAM_MODE) {
						programKnobMode = BANK_MODE;
						params[PROGRAM_PARAM].setValue(programSelector.getCurrent().getBank());
					}
					else {
						programKnobMode = PROGRAM_MODE;
						params[PROGRAM_PARAM].setValue(programSelector.getCurrent().getProgram());
					}

					lights[BANK_LIGHT].setBrightness(programKnobMode == BANK_MODE);
				}
			}
			// no longer held, but has been held for non-zero time (without being dragged), toggle "active" section (A or B)
			else if (programHoldTimer.time > 0.f) {
				programSelector.setMode(!programSelector.getMode());
				programHoldTimer.reset();

				if (programKnobMode == PROGRAM_MODE) {
					params[PROGRAM_PARAM].setValue(programSelector.getCurrent().getProgram());
				}
				else {
					params[PROGRAM_PARAM].setValue(programSelector.getCurrent().getBank());
				}
			}
		}
	}

	void checkForProgramChangeCV() {
		const int currentBank = programSelector.getSection(SECTION_A).getBank();
		const int currentProgram = programSelector.getSection(SECTION_A).getProgram();

		// TODO:		
		// const int newProgram = currentProgram + (int) inputs[PROG_A_INPUT].getVoltage();	
	}

	

	void setAlgorithmViaProgram(int newProgram) {

		const int currentBank = programSelector.getCurrent().getBank();
		const std::string algorithmName = getBankForIndex(currentBank).getProgramName(newProgram);
		const int section = programSelector.getMode();

		setAlgorithm(section, algorithmName);
	}

	void setAlgorithmViaBank(int newBank) {
		newBank = clamp(newBank, 0, numBanks);
		const int currentProgram = programSelector.getCurrent().getProgram();
		// the new bank may not have as many algorithms
		const int currentProgramInNewBank = clamp(currentProgram, 0, getBankForIndex(newBank).getSize() - 1);
		const std::string algorithmName = getBankForIndex(newBank).getProgramName(currentProgramInNewBank);
		const int section = programSelector.getMode();

		DEBUG(string::f("setAlgorithmViaBank: using %s", algorithmName.c_str()).c_str());

		setAlgorithm(section, algorithmName);
	}

	void setAlgorithm(int section, std::string algorithmName) {

		if (section > 1) {
			return;
		}

		for (int bank = 0; bank < numBanks; ++bank) {
			for (int program = 0; program < getBankForIndex(bank).getSize(); ++program) {
				if (getBankForIndex(bank).getProgramName(program) == algorithmName) {

					DEBUG(string::f("Setting algorithm[%d] (bank: %d, program: %d, name: %s)", section, bank, program, algorithmName.c_str()).c_str());

					programSelector.setMode(section);
					programSelector.getCurrent().setBank(bank);
					programSelector.getCurrent().setProgram(program);

					// update dial
					params[PROGRAM_PARAM].setValue(programKnobMode == PROGRAM_MODE ? program : bank);

					algorithm[section] = MyFactory::Instance()->Create(algorithmName);

					if (algorithm[section]) {
						algorithm[section]->init();
					}
					else {
						DEBUG(string::f("Failed to create %s", algorithmName.c_str()).c_str());
					}

					return;
				}
			}
		}

		DEBUG(string::f("Didn't find %s in programSelector", algorithmName.c_str()).c_str());
	}

	void dataFromJson(json_t* rootJ) override {
		DEBUG("JSON load starting");

		json_t* bankAJ = json_object_get(rootJ, "algorithmA");
		setAlgorithm(SECTION_A, json_string_value(bankAJ));

		json_t* bankBJ = json_object_get(rootJ, "algorithmB");
		setAlgorithm(SECTION_B, json_string_value(bankBJ));

		json_t* applyFiltersJ = json_object_get(rootJ, "applyFilters");
		applyFilters = json_boolean_value(applyFiltersJ);

		DEBUG("JSON load complete");
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();

		json_object_set_new(rootJ, "algorithmA", json_string(programSelector.getA().getCurrentProgramName().c_str()));
		json_object_set_new(rootJ, "algorithmB", json_string(programSelector.getB().getCurrentProgramName().c_str()));

		json_object_set_new(rootJ, "applyFilters", json_boolean(applyFilters));

		return rootJ;
	}
};


struct BefacoTinyKnobSnapPress : BefacoTinyKnobBlack {
	BefacoTinyKnobSnapPress() {
		snap = true;
		minAngle = -0.80 * M_PI;
		maxAngle = 0.80 * M_PI;
	}

	void onDragStart(const DragStartEvent& e) override {
		Knob::onDragStart(e);

		NoisePlethora* noisePlethoraModule = static_cast<NoisePlethora*>(module);
		if (noisePlethoraModule) {
			noisePlethoraModule->programButtonHeld = true;
			noisePlethoraModule->programButtonDragged = false;
		}

		DEBUG(string::f("onDragStart %d %d", e.button, noisePlethoraModule == nullptr).c_str());
		pos = Vec(0, 0);
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
		DEBUG(string::f("onDragEnd %d", e.button).c_str());

		NoisePlethora* noisePlethoraModule = static_cast<NoisePlethora*>(module);
		if (noisePlethoraModule) {
			noisePlethoraModule->programButtonHeld = false;
		}

		Knob::onDragEnd(e);
	}

	bool dragged = false;
	Vec pos;
};

// dervied from https://github.com/countmodula/VCVRackPlugins/blob/v2.0.0/src/components/CountModulaLEDDisplay.hpp
struct NoisePlethoraLEDDisplay : LightWidget {
	std::shared_ptr<Font> font;
	float fontSize = 28;
	Vec textPos = Vec(2, 25);
	int numChars = 1;
	bool activeDisplay = true;
	NoisePlethora* module;
	NoisePlethora::Section section = NoisePlethora::SECTION_A;
	ui::Tooltip* tooltip;

	NoisePlethoraLEDDisplay() {
		font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/Segment7Standard.otf"));
		box.size = mm2px(Vec(7.236, 10));
	}

	void onEnter(const EnterEvent& e) override {
		LightWidget::onEnter(e);
		setTooltip();
	}

	void setTooltip() {
		std::string activeName = module->programSelector.getSection(section).getCurrentProgramName();
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
		NVGcolor textColor = nvgRGB(0xff, 0x10, 0x10);

		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0.0, 0.0, box.size.x, box.size.y, 2.0);
		nvgFillColor(args.vg, backgroundColor);
		nvgFill(args.vg);
		nvgStrokeWidth(args.vg, 1.0);
		nvgStrokeColor(args.vg, borderColor);
		nvgStroke(args.vg);

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
			// nvgCircle(args.vg, 10, 24, 3);
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
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(59.909, 44.397)), module, NoisePlethora::A_OUTPUT_ALT));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(64.915, 54.608)), module, NoisePlethora::B_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(59.915, 54.608)), module, NoisePlethora::B_OUTPUT_ALT));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(22.345, 114.852)), module, NoisePlethora::GRITTY_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(34.981, 114.852)), module, NoisePlethora::FILTERED_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(47.536, 114.852)), module, NoisePlethora::WHITE_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(30.866, 37.422)), module, NoisePlethora::BANK_LIGHT));


		NoisePlethoraLEDDisplay* displayA = new NoisePlethoraLEDDisplay();
		displayA->box.pos = mm2px(Vec(13.106, 38.172));
		displayA->module = module;
		displayA->section = NoisePlethora::SECTION_A;
		addChild(displayA);

		NoisePlethoraLEDDisplay* displayB = new NoisePlethoraLEDDisplay();
		displayB->box.pos = mm2px(Vec(13.106, 50.712));
		displayB->module = module;
		displayB->section = NoisePlethora::SECTION_B;
		addChild(displayB);
	}

	void appendContextMenu(Menu* menu) override {
		NoisePlethora* module = dynamic_cast<NoisePlethora*>(this->module);
		assert(module);

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
							const std::string algorithmName = getBankForIndex(i).getProgramName(j);

							bool implemented = false;
							for (auto item : MyFactory::Instance()->factoryFunctionRegistry) {
								if (item.first == algorithmName) {
									implemented = true;
									break;
								}
							}

							if (implemented) {
								menu->addChild(createMenuItem(algorithmName, currentProgramAndBank ? CHECKMARK_STRING : "",
								[ = ]() {
									module->setAlgorithm(sectionId, algorithmName);
								}));
							}
							else {
								menu->addChild(createMenuLabel(algorithmName));
							}
						}
					}));
				}
			}));


		}

		menu->addChild(createBoolPtrMenuItem("Apply Filters", "", &module->applyFilters));
	}
};


Model* modelNoisePlethora = createModel<NoisePlethora, NoisePlethoraWidget>("NoisePlethora");