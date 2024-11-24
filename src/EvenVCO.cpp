#include "plugin.hpp"
#include "ChowDSP.hpp"

using simd::float_4;

struct EvenVCO : Module {
	enum ParamIds {
		OCTAVE_PARAM,
		TUNE_PARAM,
		PWM_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		PITCH1_INPUT,
		PITCH2_INPUT,
		FM_INPUT,
		SYNC_INPUT,
		PWM_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		TRI_OUTPUT,
		SINE_OUTPUT,
		EVEN_OUTPUT,
		SAW_OUTPUT,
		SQUARE_OUTPUT,
		NUM_OUTPUTS
	};


	float_4 phase[4] = {};
	dsp::TSchmittTrigger<float_4> syncTrigger[4];
	bool removePulseDC = true;
	bool limitPW = true;

	EvenVCO() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);
		configParam(OCTAVE_PARAM, -5.0, 4.0, 0.0, "Octave", "'", 0.5);
		getParamQuantity(OCTAVE_PARAM)->snapEnabled = true;
		configParam(TUNE_PARAM, -7.0, 7.0, 0.0, "Tune", " semitones");
		configParam(PWM_PARAM, -1.0, 1.0, 0.0, "Pulse width");

		configInput(PITCH1_INPUT, "Pitch 1");
		configInput(PITCH2_INPUT, "Pitch 2");
		configInput(FM_INPUT, "FM");
		configInput(SYNC_INPUT, "Hard Sync");
		configInput(PWM_INPUT, "Pulse Width Modulation");

		configOutput(TRI_OUTPUT, "Triangle");
		configOutput(SINE_OUTPUT, "Sine");
		configOutput(EVEN_OUTPUT, "Even");
		configOutput(SAW_OUTPUT, "Sawtooth");
		configOutput(SQUARE_OUTPUT, "Square");

	}

	void onSampleRateChange() override {
		float sampleRate = APP->engine->getSampleRate();
		for (int i = 0; i < NUM_OUTPUTS; ++i) {
			for (int c = 0; c < 4; c++) {
				oversampler[i][c].setOversamplingIndex(oversamplingIndex);
				oversampler[i][c].reset(sampleRate);
			}
		}

		for (int c = 0; c < 4; c++) {
			for (int i = 0; i < NUM_UPSAMPLED_INPUTS; i++) {
				oversamplerInputs[i][c].setOversamplingIndex(oversamplingIndex);
				oversamplerInputs[i][c].reset(sampleRate);
			}
		}

		const float lowFreqRegime = oversampler[0][0].getOversamplingRatio() * 1e-3 * sampleRate;
		DEBUG("Low freq regime: %g", lowFreqRegime);
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

	float_4 aliasSuppressedDoubleSaw(float_4* phases) {
		float_4 sawBuffer[3];
		for (int i = 0; i < 3; ++i) {
			float_4 p = 4.0 * simd::ifelse(phases[i] < 0.5,  phases[i], phases[i] - 0.5) - 1.0;
			sawBuffer[i] = (p * p * p - p) / 24.0;	// eq 11 (modified for doubled freq)
		}

		return (sawBuffer[0] - 2.0 * sawBuffer[1] + sawBuffer[2]);
	}

	float_4 aliasSuppressedOffsetSaw(float_4* phases, float_4 pw) {
		float_4 sawOffsetBuff[3];

		for (int i = 0; i < 3; ++i) {
			float_4 p = 2 * phases[i] - 1.0; 	// range -1 to +1
			float_4 pwp = p + 2 * pw;			// phase after pw (pw in [0, 1])
			pwp += simd::ifelse(pwp > 1, -2, 0);     			// modulo on [-1, +1]
			sawOffsetBuff[i] = (pwp * pwp * pwp - pwp) / 6.0;	// eq 11
		}
		return (sawOffsetBuff[0] - 2.0 * sawOffsetBuff[1] + sawOffsetBuff[2]);
	}

	enum UpsampledInputs {
		FM_INPUT_UP,
		SYNC_INPUT_UP,
		NUM_UPSAMPLED_INPUTS
	};
	chowdsp::VariableOversampling<6, float_4> oversamplerInputs[NUM_UPSAMPLED_INPUTS][4]; 	// uses a 2*6=12th order Butterworth filter
	chowdsp::VariableOversampling<6, float_4> oversampler[NUM_OUTPUTS][4]; 	// uses a 2*6=12th order Butterworth filter
	int oversamplingIndex = 2; 	// default is 2^oversamplingIndex == x4 oversampling

	void process(const ProcessArgs& args) override {

		// pitch inputs determine number of polyphony engines
		const int channels = std::max({1, inputs[PITCH1_INPUT].getChannels(), inputs[PITCH2_INPUT].getChannels()});

		const float pitchKnobs = 1.f + std::round(params[OCTAVE_PARAM].getValue()) + params[TUNE_PARAM].getValue() / 12.f;
		const int oversamplingRatio = oversampler[0][0].getOversamplingRatio();

		for (int c = 0; c < channels; c += 4) {
			float_4 pw = simd::clamp(params[PWM_PARAM].getValue() + inputs[PWM_INPUT].getPolyVoltageSimd<float_4>(c) / 5.f, -1.f, 1.f);
			if (limitPW) {
				pw = simd::rescale(pw, -1, +1, 0.05f, 0.95f);
			}
			else {
				pw = simd::rescale(pw, -1.f, +1.f, 0.f, 1.f);
			}

			const float_4 pitch = inputs[PITCH1_INPUT].getPolyVoltageSimd<float_4>(c) + inputs[PITCH2_INPUT].getPolyVoltageSimd<float_4>(c);

			// pulsewave waveform doesn't have DC even for non 50% duty cycles, but Befaco team would like the option
			// for it to be added back in for hardware compatibility reasons
			const float_4 pulseDCOffset = (!removePulseDC) * 2.f * (0.5f - pw);

			// input oversampling buffers
			float_4* osBufferSync = oversamplerInputs[SYNC_INPUT_UP][c / 4].getOSBuffer();
			float_4* osBufferFM = oversamplerInputs[FM_INPUT_UP][c / 4].getOSBuffer();

			// upsample hard sync input (if connected)
			if (inputs[SYNC_INPUT].isConnected()) {
				oversamplerInputs[SYNC_INPUT_UP][c].upsample(inputs[SYNC_INPUT].getPolyVoltageSimd<float_4>(c));
			}
			else {
				std::fill(osBufferSync, &osBufferSync[oversamplingRatio], float_4::zero());
			}
			// upsample FM input (if connected)
			if (inputs[FM_INPUT].isConnected()) {
				oversamplerInputs[FM_INPUT_UP][c].upsample(inputs[FM_INPUT].getPolyVoltageSimd<float_4>(c));
			}
			else {
				std::fill(osBufferFM, &osBufferFM[oversamplingRatio], float_4::zero());
			}

			float_4* osBufferTri = oversampler[TRI_OUTPUT][c / 4].getOSBuffer();
			float_4* osBufferSaw = oversampler[SAW_OUTPUT][c / 4].getOSBuffer();
			float_4* osBufferSin = oversampler[SINE_OUTPUT][c / 4].getOSBuffer();
			float_4* osBufferSquare = oversampler[SQUARE_OUTPUT][c / 4].getOSBuffer();
			float_4* osBufferEven = oversampler[EVEN_OUTPUT][c / 4].getOSBuffer();
			for (int i = 0; i < oversamplingRatio; ++i) {
				// use upsampled FM input
				const float_4 fmVoltage = osBufferFM[i] * 0.25f;
				const float_4 freq = dsp::FREQ_C4 * simd::pow(2.f, pitchKnobs + pitch + fmVoltage);
				const float_4 deltaBasePhase = simd::clamp(freq * args.sampleTime / oversamplingRatio, 1e-6, 0.5f);
				// floating point arithmetic doesn't work well at low frequencies, specifically because the finite difference denominator
				// becomes tiny - we check for that scenario and use naive / 1st order waveforms in that frequency regime (as aliasing isn't
				// a problem there). With no oversampling, at 44100Hz, the threshold frequency is 44.1Hz.
				const float_4 lowFreqRegime = simd::abs(deltaBasePhase) < 1e-3;
				// 1 / denominator for the second-order FD
				const float_4 denominatorInv = 0.25 / (deltaBasePhase * deltaBasePhase);

				phase[c / 4] += deltaBasePhase;
				// ensure within [0, 1]
				phase[c / 4] -= simd::floor(phase[c / 4]);

				const float_4 syncMask = syncTrigger[c / 4].process(osBufferSync[i]);
				phase[c / 4] = simd::ifelse(syncMask, 0.5f, phase[c / 4]);

				float_4 phases[3]; // phase as extrapolated to the current and two previous samples

				phases[0] = phase[c / 4] - 2 * deltaBasePhase + simd::ifelse(phase[c / 4] < 2 * deltaBasePhase, 1.f, 0.f);
				phases[1] = phase[c / 4] - deltaBasePhase + simd::ifelse(phase[c / 4] < deltaBasePhase, 1.f, 0.f);
				phases[2] = phase[c / 4];

				if (outputs[SINE_OUTPUT].isConnected() || outputs[EVEN_OUTPUT].isConnected()) {
					// sin doesn't need PDW
					osBufferSin[i] = -simd::cos(M_PI + 2.0 * M_PI * phase[c / 4]);
				}

				if (outputs[TRI_OUTPUT].isConnected()) {
					const float_4 dpwOrder1 = 1.0 - 2.0 * simd::abs(2 * phase[c / 4] - 1.0);
					const float_4 dpwOrder3 = aliasSuppressedTri(phases) * denominatorInv;

					osBufferTri[i] = -simd::ifelse(lowFreqRegime, dpwOrder1, dpwOrder3);
				}

				if (outputs[SAW_OUTPUT].isConnected()) {
					const float_4 dpwOrder1 = 2 * phase[c / 4] - 1.0;
					const float_4 dpwOrder3 = aliasSuppressedSaw(phases) * denominatorInv;

					osBufferSaw[i] = simd::ifelse(lowFreqRegime, dpwOrder1, dpwOrder3);
				}

				if (outputs[SQUARE_OUTPUT].isConnected()) {

					float_4 dpwOrder1 = simd::ifelse(phase[c / 4] < pw, +1.0, -1.0);
					dpwOrder1 += removePulseDC ? 2.f * (0.5f - pw) : 0.f;

					float_4 saw = aliasSuppressedSaw(phases);
					float_4 sawOffset = aliasSuppressedOffsetSaw(phases, pw);
					float_4 dpwOrder3 = (saw - sawOffset) * denominatorInv - pulseDCOffset;

					osBufferSquare[i] = simd::ifelse(lowFreqRegime, dpwOrder1, dpwOrder3);
				}

				if (outputs[EVEN_OUTPUT].isConnected()) {

					float_4 dpwOrder1 = 4.0 * simd::ifelse(phase[c / 4] < 0.5, phase[c / 4], phase[c / 4] - 0.5) - 1.0;
					float_4 dpwOrder3 = aliasSuppressedDoubleSaw(phases) * denominatorInv;
					float_4 doubleSaw = simd::ifelse(lowFreqRegime, dpwOrder1, dpwOrder3);
					osBufferEven[i] = 0.55 * (doubleSaw + 1.27 * osBufferSin[i]);
				}


			} 	// end of oversampling loop

			// downsample (if required)
			if (outputs[SINE_OUTPUT].isConnected()) {
				const float_4 outSin = (oversamplingRatio > 1) ? oversampler[SINE_OUTPUT][c / 4].downsample() : osBufferSin[0];
				outputs[SINE_OUTPUT].setVoltageSimd(5.f * outSin, c);
			}

			if (outputs[TRI_OUTPUT].isConnected()) {
				const float_4 outTri = (oversamplingRatio > 1) ? oversampler[TRI_OUTPUT][c / 4].downsample() : osBufferTri[0];
				outputs[TRI_OUTPUT].setVoltageSimd(5.f * outTri, c);
			}

			if (outputs[SAW_OUTPUT].isConnected()) {
				const float_4 outSaw = (oversamplingRatio > 1) ? oversampler[SAW_OUTPUT][c / 4].downsample() : osBufferSaw[0];
				outputs[SAW_OUTPUT].setVoltageSimd(5.f * outSaw, c);
			}

			if (outputs[SQUARE_OUTPUT].isConnected()) {
				const float_4 outSquare = (oversamplingRatio > 1) ? oversampler[SQUARE_OUTPUT][c / 4].downsample() : osBufferSquare[0];
				outputs[SQUARE_OUTPUT].setVoltageSimd(5.f * outSquare, c);
			}

			if (outputs[EVEN_OUTPUT].isConnected()) {
				const float_4 outEven = (oversamplingRatio > 1) ? oversampler[EVEN_OUTPUT][c / 4].downsample() : osBufferEven[0];
				outputs[EVEN_OUTPUT].setVoltageSimd(5.f * outEven, c);
			}

		} 	// end of channels loop

		// Outputs
		outputs[TRI_OUTPUT].setChannels(channels);
		outputs[SINE_OUTPUT].setChannels(channels);
		outputs[EVEN_OUTPUT].setChannels(channels);
		outputs[SAW_OUTPUT].setChannels(channels);
		outputs[SQUARE_OUTPUT].setChannels(channels);
	}


	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "removePulseDC", json_boolean(removePulseDC));
		json_object_set_new(rootJ, "limitPW", json_boolean(limitPW));
		json_object_set_new(rootJ, "oversamplingIndex", json_integer(oversampler[0][0].getOversamplingIndex()));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* pulseDCJ = json_object_get(rootJ, "removePulseDC");
		if (pulseDCJ) {
			removePulseDC = json_boolean_value(pulseDCJ);
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


struct EvenVCOWidget : ModuleWidget {
	EvenVCOWidget(EvenVCO* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/EvenVCO.svg")));

		addChild(createWidget<Knurlie>(Vec(15, 0)));
		addChild(createWidget<Knurlie>(Vec(15, 365)));
		addChild(createWidget<Knurlie>(Vec(15 * 6, 0)));
		addChild(createWidget<Knurlie>(Vec(15 * 6, 365)));

		addParam(createParam<BefacoBigKnob>(Vec(22, 32), module, EvenVCO::OCTAVE_PARAM));
		addParam(createParam<BefacoTinyKnob>(Vec(73, 131), module, EvenVCO::TUNE_PARAM));
		addParam(createParam<Davies1900hRedKnob>(Vec(16, 230), module, EvenVCO::PWM_PARAM));

		addInput(createInput<BefacoInputPort>(Vec(8, 120), module, EvenVCO::PITCH1_INPUT));
		addInput(createInput<BefacoInputPort>(Vec(19, 157), module, EvenVCO::PITCH2_INPUT));
		addInput(createInput<BefacoInputPort>(Vec(48, 183), module, EvenVCO::FM_INPUT));
		addInput(createInput<BefacoInputPort>(Vec(86, 189), module, EvenVCO::SYNC_INPUT));

		addInput(createInput<BefacoInputPort>(Vec(72, 236), module, EvenVCO::PWM_INPUT));

		addOutput(createOutput<BefacoOutputPort>(Vec(10, 283), module, EvenVCO::TRI_OUTPUT));
		addOutput(createOutput<BefacoOutputPort>(Vec(87, 283), module, EvenVCO::SINE_OUTPUT));
		addOutput(createOutput<BefacoOutputPort>(Vec(48, 306), module, EvenVCO::EVEN_OUTPUT));
		addOutput(createOutput<BefacoOutputPort>(Vec(10, 327), module, EvenVCO::SAW_OUTPUT));
		addOutput(createOutput<BefacoOutputPort>(Vec(87, 327), module, EvenVCO::SQUARE_OUTPUT));
	}

	void appendContextMenu(Menu* menu) override {
		EvenVCO* module = dynamic_cast<EvenVCO*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator());
		menu->addChild(createSubmenuItem("Hardware compatibility", "",
		[ = ](Menu * menu) {
			menu->addChild(createBoolPtrMenuItem("Remove DC from pulse", "", &module->removePulseDC));
			menu->addChild(createBoolPtrMenuItem("Limit pulsewidth (5\%-95\%)", "", &module->limitPW));
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


Model* modelEvenVCO = createModel<EvenVCO, EvenVCOWidget>("EvenVCO");
