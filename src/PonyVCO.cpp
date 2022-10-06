#include "plugin.hpp"
#include "ChowDSP.hpp"


// references:
// * "REDUCING THE ALIASING OF NONLINEAR WAVESHAPING USING CONTINUOUS-TIME CONVOLUTION" (https://www.dafx.de/paper-archive/2016/dafxpapers/20-DAFx-16_paper_41-PN.pdf)
// * "Antiderivative Antialiasing for Memoryless Nonlinearities" https://acris.aalto.fi/ws/portalfiles/portal/27135145/ELEC_bilbao_et_al_antiderivative_antialiasing_IEEESPL.pdf
// * https://ccrma.stanford.edu/~jatin/Notebooks/adaa.html
// * Pony waveshape  https://www.desmos.com/calculator/1kvahyl4ti

class FoldStage1 {
public:

	float process(float x, float xt) {
		float y;

		if (fabs(x - xPrev) < 1e-5) {
			y = f(0.5 * (xPrev + x), xt);
		}
		else {
			y = (F(x, xt) - F(xPrev, xt)) / (x - xPrev);
		}
		xPrev = x;
		return y;
	}

	// xt - threshold x
	static float f(float x, float xt) {
		if (x > xt) {
			return +5 * xt - 4 * x;
		}
		else if (x < -xt) {
			return -5 * xt - 4 * x;
		}
		else {
			return x;
		}
	}

	static float F(float x, float xt) {
		if (x > xt) {
			return 5 * xt * x - 2 * x * x - 2.5 * xt * xt;
		}
		else if (x < -xt) {
			return -5 * xt * x - 2 * x * x - 2.5 * xt * xt;

		}
		else {
			return x * x / 2.f;
		}
	}
private:
	float xPrev = 0.f;
};

class FoldStage2 {
public:
	float process(float x) {
		float y;

		if (fabs(x - xPrev) < 1e-5) {
			y = f(0.5 * (xPrev + x));
		}
		else {
			y = (F(x) - F(xPrev)) / (x - xPrev);
		}
		xPrev = x;
		return y;
	}

	static float f(float x) {
		if (-(x + 2) > c) {
			return c;
		}
		else if (x < -1) {
			return -(x + 2);
		}
		else if (x < 1) {
			return x;
		}
		else if (-x + 2 > -c) {
			return -x + 2;
		}
		else {
			return -c;
		}
	}

	static float F(float x) {
		if (x < 0) {
			return F(-x);
		}
		else if (x < 1) {
			return x * x * 0.5;
		}
		else if (x < 2 + c) {
			return 2 * x * (1.f - x * 0.25f) - 1.f;
		}
		else {
			return 2 * (2 + c) * (1 - (2 + c) * 0.25f) - 1.f - c * (x - 2 - c);
		}
	}

private:
	float xPrev = 0.f;
	static constexpr float c = 0.1;
};


struct PonyVCO : Module {
	enum ParamId {
		FREQ_PARAM,
		RANGE_PARAM,
		TIMBRE_PARAM,
		OCT_PARAM,
		WAVE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		TZFM_INPUT,
		TIMBRE_INPUT,
		VOCT_INPUT,
		SYNC_INPUT,
		VCA_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUT_OUTPUT,
		OUT2_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};
	enum Waveform {
		WAVE_SIN,
		WAVE_TRI,
		WAVE_SAW,
		WAVE_PULSE
	};

	float range[4] = {8.f, 1.f, 1.f / 12.f, 10.f};
	chowdsp::VariableOversampling<> oversampler;
	int oversamplingIndex = 1; 	// default is 2^oversamplingIndex == x2 oversampling

	DCBlocker blockOutputDCFilter;
	bool blockOutputDC = false;

	DCBlocker blockTZFMDCFilter;
	bool blockTZFMDC = true;

	// hardware doesn't limit PW but some user might want to (to 5%->95%)
	bool limitPW = true;

	dsp::SchmittTrigger syncTrigger;

	bool naiveImplementation = false;
	FoldStage1 stage1;
	FoldStage2 stage2;

	PonyVCO() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(FREQ_PARAM, -0.5f, 0.5f, 0.0f, "Frequency");
		configSwitch(RANGE_PARAM, 0.f, 3.f, 0.f, "Range", {"VCO: Full", "VCO: Octave", "VCO: Semitone", "LFO"});
		configParam(TIMBRE_PARAM, 0.f, 1.f, 0.f, "Timbre");
		configSwitch(OCT_PARAM, 0.f, 6.f, 4.f, "Octave", {"C1", "C2", "C3", "C4", "C5", "C6", "C7"});
		configSwitch(WAVE_PARAM, 0.f, 3.f, 0.f, "Wave", {"Sin", "Triangle", "Sawtooth", "Pulse"});
		configInput(TZFM_INPUT, "Through-zero FM");
		configInput(TIMBRE_INPUT, "Timber (wavefolder/PWM)");
		configInput(VOCT_INPUT, "Volt per octave");
		configInput(SYNC_INPUT, "Hard sync");
		configInput(VCA_INPUT, "VCA");
		configOutput(OUT_OUTPUT, "Waveform");

		// calculate up/downsampling rates
		onSampleRateChange();
	}

	void onSampleRateChange() override {
		float sampleRate = APP->engine->getSampleRate();
		blockTZFMDCFilter.setFrequency(5. / sampleRate);
		blockOutputDCFilter.setFrequency(11.025 / sampleRate);
		oversampler.setOversamplingIndex(oversamplingIndex);
		oversampler.reset(sampleRate);
	}

	// implementation taken from "Alias-Suppressed Oscillators Based on Differentiated Polynomial Waveforms",
	// also the notes from Surge Synthesier repo:
	// https://github.com/surge-synthesizer/surge/blob/09f1ec8e103265bef6fc0d8a0fc188238197bf8c/src/common/dsp/oscillators/ModernOscillator.cpp#L19
	// Calculation is performed at double precision, as the differencing equations appeared to work poorly with only float.

	double phase = 0.0; 	// phase at current (sub)sample
	double phases[3] = {}; 	// phase as extrapolated to the current and two previous samples
	double sawBuffer[3] = {}, sawOffsetBuff[3] = {}, triBuffer[3] = {}; 	// buffers for storing the terms in the difference equation

	void process(const ProcessArgs& args) override {

		const int rangeIndex = params[RANGE_PARAM].getValue();
		const bool lfoMode = rangeIndex == 3;

		const Waveform waveform = (Waveform) params[WAVE_PARAM].getValue();
		const float mult = lfoMode ? 1.0 : dsp::FREQ_C4;
		const float baseFreq = std::pow(2, (int)(params[OCT_PARAM].getValue() - 3)) * mult;
		const int oversamplingRatio = lfoMode ? 1 : oversampler.getOversamplingRatio();
		const float timbre = clamp(params[TIMBRE_PARAM].getValue() + inputs[TIMBRE_INPUT].getVoltage() / 10.f, 0.f, 1.f);

		float tzfmVoltage = inputs[TZFM_INPUT].getVoltage();
		if (blockTZFMDC) {
			tzfmVoltage = blockTZFMDCFilter.process(tzfmVoltage);
		}

		const double pitch = inputs[VOCT_INPUT].getVoltage() + params[FREQ_PARAM].getValue() * range[rangeIndex];
		const double freq = baseFreq * simd::pow(2.f, pitch);
		const double deltaBasePhase = clamp(freq * args.sampleTime / oversamplingRatio, -0.5f, 0.5f);
		// denominator for the second-order FD
		const double denominator = 0.25 / (deltaBasePhase * deltaBasePhase);
		// not clamped, but _total_ phase treated later with floor/ceil
		const double deltaFMPhase = freq * tzfmVoltage * args.sampleTime / oversamplingRatio;

		float pw = timbre;
		if (limitPW) {
			pw = clamp(pw, 0.05, 0.95);
		}

		// hard sync
		if (syncTrigger.process(inputs[SYNC_INPUT].getVoltage())) {
			// hardware waveform is actually cos, so pi/2 phase offset is required
			// - variable phase is defined on [0, 1] rather than [0, 2pi] so pi/2 -> 0.25
			phase = (waveform == WAVE_SIN) ? 0.25f : 0.f;
		}

		float* osBuffer = oversampler.getOSBuffer();
		for (int i = 0; i < oversamplingRatio; ++i) {

			phase += deltaBasePhase + deltaFMPhase;
			if (phase > 1.f) {
				phase -= floor(phase);
			}
			else if (phase < 0.f) {
				phase += -ceil(phase) + 1;
			}

			// sin is simple
			if (waveform == WAVE_SIN) {
				osBuffer[i] = sin2pi_pade_05_5_4(phase);
			}
			else {

				phases[0] = phase - 2 * deltaBasePhase + (phase < 2 * deltaBasePhase);
				phases[1] = phase - deltaBasePhase + (phase < deltaBasePhase);
				phases[2] = phase;

				switch (waveform) {
					case WAVE_TRI: {

						if (naiveImplementation) {
							osBuffer[i] = 4.f * std::fabs(phase - std::round(phase)) - 1.f;
						}
						else {
							osBuffer[i] = aliasSuppressedTri() * denominator;
						}
						break;
					}
					case WAVE_SAW: {
						if (naiveImplementation) {
							osBuffer[i] = -1.f + 2.f * phase;
						}
						else {
							osBuffer[i] = aliasSuppressedSaw() * denominator;
						}
						break;
					}
					case WAVE_PULSE: {

						if (naiveImplementation) {
							osBuffer[i] = (phase < pw) ?  -1.f : +1.f;
						}
						else {
							double saw = aliasSuppressedSaw();
							double sawOffset = aliasSuppressedOffsetSaw(pw);

							osBuffer[i] = (sawOffset - saw) * denominator;
						}
						break;
					}
					default: break;
				}
			}

			if (waveform != WAVE_PULSE) {
				osBuffer[i] = wavefolder(osBuffer[i], (1 - 0.85 * timbre));
			}
		}

		// downsample, optionally remove DC
		float out = (oversamplingRatio > 1) ? oversampler.downsample() : osBuffer[0];
		if (blockOutputDC) {
			out = blockOutputDCFilter.process(out);
		}

		// end of chain VCA
		const float gain = std::max(0.f, inputs[VCA_INPUT].getNormalVoltage(10.f) / 10.f);
		outputs[OUT_OUTPUT].setVoltage(5.f * out * gain);
	}

	double aliasSuppressedTri() {
		for (int i = 0; i < 3; ++i) {
			double p = 2 * phases[i] - 1.0; 				// range -1.0 to +1.0
			double s = 0.5 - std::abs(p); 					// eq 30
			triBuffer[i] = (s * s * s - 0.75 * s) / 3.0; 	// eq 29
		}
		return (triBuffer[0] - 2.0 * triBuffer[1] + triBuffer[2]);
	}

	double aliasSuppressedSaw() {
		for (int i = 0; i < 3; ++i) {
			double p = 2 * phases[i] - 1.0; 		// range -1 to +1
			sawBuffer[i] = (p * p * p - p) / 6.0;	// eq 11
		}

		return (sawBuffer[0] - 2.0 * sawBuffer[1] + sawBuffer[2]);
	}

	double aliasSuppressedOffsetSaw(double pw) {
		for (int i = 0; i < 3; ++i) {
			double p = 2 * phases[i] - 1.0; 	// range -1 to +1
			double pwp = p + 2 * pw;			// phase after pw (pw in [0, 1])
			pwp += (pwp > 1) * -2;     			// modulo on [-1, +1]
			sawOffsetBuff[i] = (pwp * pwp * pwp - pwp) / 6.0;	// eq 11
		}
		return (sawOffsetBuff[0] - 2.0 * sawOffsetBuff[1] + sawOffsetBuff[2]);
	}

	float wavefolder(float x, float xt) {
		if (naiveImplementation) {
			return FoldStage2::f(FoldStage1::f(x, xt));
		}
		else {
			return stage2.process(stage1.process(x, xt));
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "blockTZFMDC", json_boolean(blockTZFMDC));
		json_object_set_new(rootJ, "blockOutputDC", json_boolean(blockOutputDC));
		json_object_set_new(rootJ, "oversamplingIndex", json_integer(oversampler.getOversamplingIndex()));
		json_object_set_new(rootJ, "naiveImplementation", json_boolean(naiveImplementation));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* blockOutputDCJ = json_object_get(rootJ, "blockOutputDC");
		if (blockOutputDCJ) {
			blockOutputDC = json_boolean_value(blockOutputDCJ);
		}

		json_t* blockTZFMDCJ = json_object_get(rootJ, "blockTZFMDC");
		if (blockTZFMDCJ) {
			blockTZFMDC = json_boolean_value(blockTZFMDCJ);
		}

		json_t* oversamplingIndexJ = json_object_get(rootJ, "oversamplingIndex");
		if (oversamplingIndexJ) {
			oversamplingIndex = json_integer_value(oversamplingIndexJ);
			onSampleRateChange();
		}

		json_t* naiveJ = json_object_get(rootJ, "naiveImplementation");
		if (naiveJ) {
			naiveImplementation = json_boolean_value(naiveJ);
		}
	}
};


struct PonyVCOWidget : ModuleWidget {
	PonyVCOWidget(PonyVCO* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/PonyVCO.svg")));

		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Davies1900hDarkGreyKnob>(mm2px(Vec(10.0, 14.999)), module, PonyVCO::FREQ_PARAM));
		addParam(createParam<CKSSHoriz4>(mm2px(Vec(5.498, 27.414)), module, PonyVCO::RANGE_PARAM));
		addParam(createParam<BefacoSlidePotSmall>(mm2px(Vec(12.65, 37.0)), module, PonyVCO::TIMBRE_PARAM));
		addParam(createParam<CKSSVert7>(mm2px(Vec(3.8, 41.304)), module, PonyVCO::OCT_PARAM));
		addParam(createParam<CKSSHoriz4>(mm2px(Vec(5.681, 74.436)), module, PonyVCO::WAVE_PARAM));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(5.014, 87.455)), module, PonyVCO::TZFM_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(14.978, 87.455)), module, PonyVCO::TIMBRE_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(5.014, 100.413)), module, PonyVCO::VOCT_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(14.978, 100.413)), module, PonyVCO::SYNC_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(5.014, 113.409)), module, PonyVCO::VCA_INPUT));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(15.0, 113.363)), module, PonyVCO::OUT_OUTPUT));
	}


	void appendContextMenu(Menu* menu) override {
		PonyVCO* module = dynamic_cast<PonyVCO*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator());
		menu->addChild(createBoolPtrMenuItem("Block TZFM DC", "", &module->blockTZFMDC));
		menu->addChild(createBoolPtrMenuItem("Block Output DC", "", &module->blockOutputDC));
		menu->addChild(createBoolPtrMenuItem("Limit square PW (5\%->95\%)", "", &module->limitPW));
		menu->addChild(createBoolPtrMenuItem("Naive implementation", "", &module->naiveImplementation));

		menu->addChild(createIndexSubmenuItem("Oversampling",
		{"Off", "x2", "x4", "x8", "x16"},
		[ = ]() {
			return module->oversamplingIndex;
		},
		[ = ](int mode) {
			module->oversamplingIndex = mode;
			module->onSampleRateChange();
		}
		                                     ));

	}
};


Model* modelPonyVCO = createModel<PonyVCO, PonyVCOWidget>("PonyVCO");