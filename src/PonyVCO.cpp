#include "plugin.hpp"
#include "ChowDSP.hpp"

using simd::float_4;

// references:
// * "REDUCING THE ALIASING OF NONLINEAR WAVESHAPING USING CONTINUOUS-TIME CONVOLUTION" (https://www.dafx.de/paper-archive/2016/dafxpapers/20-DAFx-16_paper_41-PN.pdf)
// * "Antiderivative Antialiasing for Memoryless Nonlinearities" https://acris.aalto.fi/ws/portalfiles/portal/27135145/ELEC_bilbao_et_al_antiderivative_antialiasing_IEEESPL.pdf
// * https://ccrma.stanford.edu/~jatin/Notebooks/adaa.html
// * Pony waveshape  https://www.desmos.com/calculator/1kvahyl4ti

template<typename T>
class FoldStage1 {
public:

	T process(T x, T xt) {
		T y = simd::ifelse(simd::abs(x - xPrev) < 1e-5,
		                   f(0.5 * (xPrev + x), xt),
		                   (F(x, xt) - F(xPrev, xt)) / (x - xPrev));

		xPrev = x;
		return y;
	}

	// xt - threshold x
	static T f(T x, T xt) {
		return simd::ifelse(x > xt, +5 * xt - 4 * x, simd::ifelse(x < -xt, -5 * xt - 4 * x, x));
	}

	static T F(T x, T xt) {
		return simd::ifelse(x > xt,  5 * xt * x - 2 * x * x - 2.5 * xt * xt,
		                    simd::ifelse(x < -xt, -5 * xt * x - 2 * x * x - 2.5 * xt * xt, x * x / 2.f));
	}

	void reset() {
		xPrev = 0.f;
	}

private:
	T xPrev = 0.f;
};

template<typename T>
class FoldStage2 {
public:
	T process(T x) {
		const T y = simd::ifelse(simd::abs(x - xPrev) < 1e-5, f(0.5 * (xPrev + x)), (F(x) - F(xPrev)) / (x - xPrev));
		xPrev = x;
		return y;
	}

	static T f(T x) {
		return simd::ifelse(-(x + 2) > c, c, simd::ifelse(x < -1, -(x + 2), simd::ifelse(x < 1, x, simd::ifelse(-x + 2 > -c, -x + 2, -c))));
	}

	static T F(T x) {
		return simd::ifelse(x > 0, F_signed(x), F_signed(-x));
	}

	static T F_signed(T x) {
		return simd::ifelse(x < 1, x * x * 0.5, simd::ifelse(x < 2.f + c, 2.f * x * (1.f - x * 0.25f) - 1.f,
		                    2.f * (2.f + c) * (1.f - (2.f + c) * 0.25f) - 1.f - c * (x - 2.f - c)));
	}

	void reset() {
		xPrev = 0.f;
	}

private:
	T xPrev = 0.f;
	static constexpr float c = 0.1f;
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
	chowdsp::VariableOversampling<6, float_4> oversampler[4]; 	// uses a 2*6=12th order Butterworth filter
	int oversamplingIndex = 1; 	// default is 2^oversamplingIndex == x2 oversampling

	dsp::TRCFilter<float_4> blockTZFMDCFilter[4];
	bool blockTZFMDC = true;

	// hardware doesn't limit PW but some user might want to (to 5%->95%)
	bool limitPW = true;

	// hardware has DC for non-50% duty cycle, optionally add/remove it
	bool removePulseDC = true;

	dsp::TSchmittTrigger<float_4> syncTrigger[4];

	FoldStage1<float_4> stage1[4];
	FoldStage2<float_4> stage2[4];

	PonyVCO() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(FREQ_PARAM, -0.5f, 0.5f, 0.0f, "Frequency");
		auto rangeParam = configSwitch(RANGE_PARAM, 0.f, 3.f, 0.f, "Range", {"VCO: Full", "VCO: Octave", "VCO: Semitone", "LFO"});
		rangeParam->snapEnabled = true;

		configParam(TIMBRE_PARAM, 0.f, 1.f, 0.f, "Timbre");
		auto octParam = configSwitch(OCT_PARAM, 0.f, 6.f, 4.f, "Octave", {"C1", "C2", "C3", "C4", "C5", "C6", "C7"});
		octParam->snapEnabled = true;

		auto waveParam = configSwitch(WAVE_PARAM, 0.f, 3.f, 0.f, "Wave", {"Sin", "Triangle", "Sawtooth", "Pulse"});
		waveParam->snapEnabled = true;

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
		for (int c = 0; c < 4; c++) {
			blockTZFMDCFilter[c].setCutoffFreq(5.0 / sampleRate);
			oversampler[c].setOversamplingIndex(oversamplingIndex);
			oversampler[c].reset(sampleRate);

			stage1[c].reset();
			stage2[c].reset();
		}
	}

	// implementation taken from "Alias-Suppressed Oscillators Based on Differentiated Polynomial Waveforms",
	// also the notes from Surge Synthesier repo:
	// https://github.com/surge-synthesizer/surge/blob/09f1ec8e103265bef6fc0d8a0fc188238197bf8c/src/common/dsp/oscillators/ModernOscillator.cpp#L19

	float_4 phase[4] = {}; 	// phase at current (sub)sample

	void process(const ProcessArgs& args) override {

		const int rangeIndex = params[RANGE_PARAM].getValue();
		const bool lfoMode = rangeIndex == 3;

		const Waveform waveform = (Waveform) params[WAVE_PARAM].getValue();
		const float mult = lfoMode ? 1.0 : dsp::FREQ_C4;
		const float baseFreq = std::pow(2, (int)(params[OCT_PARAM].getValue() - 3)) * mult;
		const int oversamplingRatio = lfoMode ? 1 : oversampler[0].getOversamplingRatio();

		// number of active polyphony engines (must be at least 1)
		const int channels = std::max({inputs[TZFM_INPUT].getChannels(), inputs[VOCT_INPUT].getChannels(), inputs[TIMBRE_INPUT].getChannels(), 1});

		for (int c = 0; c < channels; c += 4) {
			const float_4 timbre = simd::clamp(params[TIMBRE_PARAM].getValue() + inputs[TIMBRE_INPUT].getPolyVoltageSimd<float_4>(c) / 10.f, 0.f, 1.f);

			float_4 tzfmVoltage = inputs[TZFM_INPUT].getPolyVoltageSimd<float_4>(c);
			if (blockTZFMDC) {
				blockTZFMDCFilter[c / 4].process(tzfmVoltage);
				tzfmVoltage = blockTZFMDCFilter[c / 4].highpass();
			}

			const float_4 pitch = inputs[VOCT_INPUT].getPolyVoltageSimd<float_4>(c) + params[FREQ_PARAM].getValue() * range[rangeIndex];
			const float_4 freq = baseFreq * simd::pow(2.f, pitch);
			const float_4 deltaBasePhase = simd::clamp(freq * args.sampleTime / oversamplingRatio, -0.5f, 0.5f);
			// floating point arithmetic doesn't work well at low frequencies, specifically because the finite difference denominator
			// becomes tiny - we check for that scenario and use naive / 1st order waveforms in that frequency regime (as aliasing isn't
			// a problem there). With no oversampling, at 44100Hz, the threshold frequency is 44.1Hz.
			const float_4 lowFreqRegime = simd::abs(deltaBasePhase) < 1e-3;

			// 1 / denominator for the second-order FD
			const float_4 denominatorInv = 0.25 / (deltaBasePhase * deltaBasePhase);
			// not clamped, but _total_ phase treated later with floor/ceil
			const float_4 deltaFMPhase = freq * tzfmVoltage * args.sampleTime / oversamplingRatio;

			float_4 pw = timbre;
			if (limitPW) {
				pw = clamp(pw, 0.05, 0.95);
			}
			// pulsewave waveform doesn't have DC even for non 50% duty cycles, but Befaco team would like the option
			// for it to be added back in for hardware compatibility reasons
			const float_4 pulseDCOffset = (!removePulseDC) * 2.f * (0.5f - pw);

			// hard sync
			const float_4 syncMask = syncTrigger[c / 4].process(inputs[SYNC_INPUT].getPolyVoltageSimd<float_4>(c));
			if (waveform == WAVE_SIN) {
				// hardware waveform is actually cos, so pi/2 phase offset is required
				// - variable phase is defined on [0, 1] rather than [0, 2pi] so pi/2 -> 0.25
				phase[c / 4] = simd::ifelse(syncMask, 0.25f, phase[c / 4]);
			}
			else {
				phase[c / 4] = simd::ifelse(syncMask, 0.f, phase[c / 4]);
			}

			float_4* osBuffer = oversampler[c / 4].getOSBuffer();
			for (int i = 0; i < oversamplingRatio; ++i) {

				phase[c / 4] += deltaBasePhase + deltaFMPhase;
				// ensure within [0, 1]
				phase[c / 4] -= simd::floor(phase[c / 4]);

				// sin is simple
				if (waveform == WAVE_SIN) {
					osBuffer[i] = sin2pi_pade_05_5_4(phase[c / 4]);
				}
				else {
					float_4 phases[3]; // phase as extrapolated to the current and two previous samples

					phases[0] = phase[c / 4] - 2 * deltaBasePhase + simd::ifelse(phase[c / 4] < 2 * deltaBasePhase, 1.f, 0.f);
					phases[1] = phase[c / 4] - deltaBasePhase + simd::ifelse(phase[c / 4] < deltaBasePhase, 1.f, 0.f);
					phases[2] = phase[c / 4];

					switch (waveform) {
						case WAVE_TRI: {
							const float_4 dpwOrder1 = 1.0 - 2.0 * simd::abs(2 * phase[c / 4] - 1.0);
							const float_4 dpwOrder3 = aliasSuppressedTri(phases) * denominatorInv;

							osBuffer[i] = simd::ifelse(lowFreqRegime, dpwOrder1, dpwOrder3);
							break;
						}
						case WAVE_SAW: {
							const float_4 dpwOrder1 = 2 * phase[c / 4] - 1.0;
							const float_4 dpwOrder3 = aliasSuppressedSaw(phases) * denominatorInv;

							osBuffer[i] = simd::ifelse(lowFreqRegime, dpwOrder1, dpwOrder3);
							break;
						}
						case WAVE_PULSE: {
							float_4 dpwOrder1 = simd::ifelse(phase[c / 4] < 1. - pw, +1.0, -1.0);
							dpwOrder1 -= removePulseDC ? 2.f * (0.5f - pw) : 0.f;

							float_4 saw = aliasSuppressedSaw(phases);
							float_4 sawOffset = aliasSuppressedOffsetSaw(phases, pw);
							float_4 dpwOrder3 = (sawOffset - saw) * denominatorInv + pulseDCOffset;

							osBuffer[i] = simd::ifelse(lowFreqRegime, dpwOrder1, dpwOrder3);
							break;
						}
						default: break;
					}
				}

				if (waveform != WAVE_PULSE) {
					osBuffer[i] = wavefolder(osBuffer[i], (1 - 0.85 * timbre), c);
				}

			} 	// end of oversampling loop

			// downsample (if required)
			const float_4 out = (oversamplingRatio > 1) ? oversampler[c / 4].downsample() : osBuffer[0];

			// end of chain VCA
			const float_4 gain = simd::clamp(inputs[VCA_INPUT].getNormalPolyVoltageSimd<float_4>(10.f, c) / 10.f, 0.f, 1.f);
			outputs[OUT_OUTPUT].setVoltageSimd(5.f * out * gain, c);

		} 	// end of channels loop

		outputs[OUT_OUTPUT].setChannels(channels);
	}

	float_4 aliasSuppressedTri(float_4* phases) {
		float_4 triBuffer[3];
		for (int i = 0; i < 3; ++i) {
			float_4 p = 2 * phases[i] - 1.0; 				// range -1.0 to +1.0
			float_4 s = 0.5 - simd::abs(p); 				// eq 30
			triBuffer[i] = (s * s * s - 0.75 * s) / 3.0; 	// eq 29
		}
		return (triBuffer[0] - 2.0 * triBuffer[1] + triBuffer[2]);
	}

	float_4 aliasSuppressedSaw(float_4* phases) {
		float_4 sawBuffer[3];
		for (int i = 0; i < 3; ++i) {
			float_4 p = 2 * phases[i] - 1.0; 		// range -1 to +1
			sawBuffer[i] = (p * p * p - p) / 6.0;	// eq 11
		}

		return (sawBuffer[0] - 2.0 * sawBuffer[1] + sawBuffer[2]);
	}

	float_4 aliasSuppressedOffsetSaw(float_4* phases, float_4 pw) {
		float_4 sawOffsetBuff[3];

		for (int i = 0; i < 3; ++i) {
			float_4 p = 2 * phases[i] - 1.0; 		// range -1 to +1
			float_4 pwp = p + 2 * pw;				// phase after pw (pw in [0, 1])
			pwp += simd::ifelse(pwp > 1, -2, 0);     			// modulo on [-1, +1]
			sawOffsetBuff[i] = (pwp * pwp * pwp - pwp) / 6.0;	// eq 11
		}
		return (sawOffsetBuff[0] - 2.0 * sawOffsetBuff[1] + sawOffsetBuff[2]);
	}

	float_4 wavefolder(float_4 x, float_4 xt, int c) {
		return stage2[c / 4].process(stage1[c / 4].process(x, xt));
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "blockTZFMDC", json_boolean(blockTZFMDC));
		json_object_set_new(rootJ, "removePulseDC", json_boolean(removePulseDC));
		json_object_set_new(rootJ, "limitPW", json_boolean(limitPW));
		json_object_set_new(rootJ, "oversamplingIndex", json_integer(oversampler[0].getOversamplingIndex()));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {

		json_t* blockTZFMDCJ = json_object_get(rootJ, "blockTZFMDC");
		if (blockTZFMDCJ) {
			blockTZFMDC = json_boolean_value(blockTZFMDCJ);
		}

		json_t* removePulseDCJ = json_object_get(rootJ, "removePulseDC");
		if (removePulseDCJ) {
			removePulseDC = json_boolean_value(removePulseDCJ);
		}

		json_t* limitPWJ = json_object_get(rootJ, "limitPW");
		if (limitPWJ) {
			limitPW = json_boolean_value(limitPWJ);
		}

		json_t* oversamplingIndexJ = json_object_get(rootJ, "oversamplingIndex");
		if (oversamplingIndexJ) {
			oversamplingIndex = json_integer_value(oversamplingIndexJ);
			onSampleRateChange();
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
		addParam(createParam<CKSSVert7>(mm2px(Vec(3.8, 40.54)), module, PonyVCO::OCT_PARAM));
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
		menu->addChild(createSubmenuItem("Hardware compatibility", "",
		[ = ](Menu * menu) {
			menu->addChild(createBoolPtrMenuItem("Filter TZFM DC", "", &module->blockTZFMDC));
			menu->addChild(createBoolPtrMenuItem("Limit pulsewidth (5\%-95\%)", "", &module->limitPW));
			menu->addChild(createBoolPtrMenuItem("Remove pulse DC", "", &module->removePulseDC));
		}
		                                ));

		menu->addChild(createIndexSubmenuItem("Oversampling",
		{"Off", "x2", "x4", "x8"},
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