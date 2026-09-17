#include "plugin.hpp"
#include "ChowDSP.hpp"

using namespace simd;

struct Octaves : Module {
	enum ParamId {
		PWM_CV_PARAM,
		OCTAVE_PARAM,
		TUNE_PARAM,
		PWM_PARAM,
		RANGE_PARAM,
		GAIN_01F_PARAM,
		GAIN_02F_PARAM,
		GAIN_04F_PARAM,
		GAIN_08F_PARAM,
		GAIN_16F_PARAM,
		GAIN_32F_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		VOCT1_INPUT,
		VOCT2_INPUT,
		SYNC_INPUT,
		PWM_INPUT,
		GAIN_01F_INPUT,
		GAIN_02F_INPUT,
		GAIN_04F_INPUT,
		GAIN_08F_INPUT,
		GAIN_16F_INPUT,
		GAIN_32F_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUT_01F_OUTPUT,
		OUT_02F_OUTPUT,
		OUT_04F_OUTPUT,
		OUT_08F_OUTPUT,
		OUT_16F_OUTPUT,
		OUT_32F_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	bool limitPW = true;
	bool removePulseDC = false;
	bool useTriangleCore = false;
	static const int NUM_OUTPUTS = 6;
	const float ranges[3] = {4.f, 1.f, 1.f / 12.f}; 	// full, octave, semitone

	float_4 phase[4] = {};		// phase for core waveform, in [0, 1]
	chowdsp::VariableOversampling<6, float_4> oversampler[NUM_OUTPUTS][4]; 	// uses a 2*6=12th order Butterworth filter
	int oversamplingIndex = 2; 	// default is 2^oversamplingIndex == x4 oversampling

	DCBlockerT<2, float_4> blockDCFilter[NUM_OUTPUTS][4];			// optionally block DC with RC filter @ ~22 Hz
	dsp::TSchmittTrigger<float_4> syncTrigger[4]; 	// for hard sync

	Octaves() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(PWM_CV_PARAM, 0.f, 1.f, 1.f, "PWM CV attenuater");

		auto octParam = configSwitch(OCTAVE_PARAM, 0.f, 6.f, 1.f, "Octave", {"C1", "C2", "C3", "C4", "C5", "C6", "C7"});
		octParam->snapEnabled = true;

		configParam(TUNE_PARAM, -1.f, 1.f, 0.f, "Tune");
		configParam(PWM_PARAM, 0.5f, 0.f, 0.5f, "PWM");
		auto rangeParam = configSwitch(RANGE_PARAM, 0.f, 2.f, 1.f, "Range", {"VCO: Full", "VCO: Octave", "VCO: Semitone"});
		rangeParam->snapEnabled = true;

		configParam(GAIN_01F_PARAM, 0.f, 1.f, 1.00f, "Gain Fundamental");
		configParam(GAIN_02F_PARAM, 0.f, 1.f, 0.75f, "Gain x2 Fundamental");
		configParam(GAIN_04F_PARAM, 0.f, 1.f, 0.50f, "Gain x4 Fundamental");
		configParam(GAIN_08F_PARAM, 0.f, 1.f, 0.25f, "Gain x8 Fundamental");
		configParam(GAIN_16F_PARAM, 0.f, 1.f, 0.f, "Gain x16 Fundamental");
		configParam(GAIN_32F_PARAM, 0.f, 1.f, 0.f, "Gain x32 Fundamental");

		configInput(VOCT1_INPUT, "V/Octave 1");
		configInput(VOCT2_INPUT, "V/Octave 2");
		configInput(SYNC_INPUT, "Sync");
		configInput(PWM_INPUT, "PWM");
		configInput(GAIN_01F_INPUT, "Gain Fundamental CV");
		configInput(GAIN_02F_INPUT, "Gain x2F CV");
		configInput(GAIN_04F_INPUT, "Gain x4F CV");
		configInput(GAIN_08F_INPUT, "Gain x8F CV");
		configInput(GAIN_16F_INPUT, "Gain x16F CV");
		configInput(GAIN_32F_INPUT, "Gain x32F CV");

		configOutput(OUT_01F_OUTPUT, "x1F");
		configOutput(OUT_02F_OUTPUT, "x2F");
		configOutput(OUT_04F_OUTPUT, "x4F");
		configOutput(OUT_08F_OUTPUT, "x8F");
		configOutput(OUT_16F_OUTPUT, "x16F");
		configOutput(OUT_32F_OUTPUT, "x32F");

		// calculate up/downsampling rates
		onSampleRateChange();
	}

	void onSampleRateChange() override {
		float sampleRate = APP->engine->getSampleRate();
		for (int c = 0; c < NUM_OUTPUTS; c++) {
			for (int i = 0; i < 4; i++) {
				oversampler[c][i].setOversamplingIndex(oversamplingIndex);
				oversampler[c][i].reset(sampleRate);
				blockDCFilter[c][i].setFrequency(22.05 / sampleRate);
			}
		}
	}


	void process(const ProcessArgs& args) override {

		const int numActivePolyphonyEngines = getNumActivePolyphonyEngines();

		// work out active outputs
		const int highestOutput = getMaxConnectedOutput();
		if (highestOutput == -1) {
			return;
		}

		for (int c = 0; c < numActivePolyphonyEngines; c += 4) {

			const int rangeIndex = params[RANGE_PARAM].getValue();
			float_4 pitch = ranges[rangeIndex] * params[TUNE_PARAM].getValue() + inputs[VOCT1_INPUT].getPolyVoltageSimd<float_4>(c) + inputs[VOCT2_INPUT].getPolyVoltageSimd<float_4>(c);
			pitch += params[OCTAVE_PARAM].getValue() - 3;
			const float_4 freq = dsp::FREQ_C4 * dsp::exp2_taylor5(pitch);
			// -1 to +1
			const float_4 pwmCV = params[PWM_CV_PARAM].getValue() * clamp(inputs[PWM_INPUT].getPolyVoltageSimd<float_4>(c) / 10.f, -1.f, 1.f);
			const float_4 pulseWidthLimit = limitPW ? 0.05f : 0.0f;

			// pwm in [-0.25 : +0.25]
			const float_4 pwm = 2 * clamp(0.5 - params[PWM_PARAM].getValue() + 0.5 * pwmCV, -0.5f + pulseWidthLimit, 0.5f - pulseWidthLimit);

			const int oversamplingRatio = oversampler[0][0].getOversamplingRatio();

			const float_4 deltaPhase = freq * args.sampleTime / oversamplingRatio;

			//  process sync
			float_4 sync = syncTrigger[c / 4].process(inputs[SYNC_INPUT].getPolyVoltageSimd<float_4>(c));
			phase[c / 4] = simd::ifelse(sync, 0.5f, phase[c / 4]);


			for (int i = 0; i < oversamplingRatio; i++) {

				phase[c / 4] += deltaPhase;
				phase[c / 4] -= simd::floor(phase[c / 4]);

				float_4 sum = {};
				for (int oct = 0; oct <= highestOutput; oct++) {

					const float_4 gainCV = simd::clamp(inputs[GAIN_01F_INPUT + oct].getNormalPolyVoltageSimd<float_4>(10.f, c) / 10.f, 0.f, 1.0f);
					const float_4 gain = params[GAIN_01F_PARAM + oct].getValue() * gainCV;

					// don't bother processing if gain is zero and no output is connected
					const bool isGainZero = simd::movemask(gain != 0.f) == 0; 
					if (isGainZero && !outputs[OUT_01F_OUTPUT + oct].isConnected()) {
						continue;
					}

					// derive phases for higher octaves from base phase (this keeps things in sync!)
					const float_4 n = (float)(1 << oct);
					// this is on [0, 1]
					const float_4 effectivePhase = n * simd::fmod(phase[c / 4], 1 / n);
					const float_4 waveTri = 1.0 - 2.0 * simd::abs(2.f * effectivePhase - 1.0);
					// build square from triangle + comparator
					const float_4 waveSquare = simd::ifelse(waveTri > pwm, +1.f, -1.f);

					sum += (useTriangleCore ? waveTri : waveSquare) * gain;
					sum = clamp(sum, -1.f, 1.f);

					if (outputs[OUT_01F_OUTPUT + oct].isConnected()) {
						oversampler[oct][c/4].getOSBuffer()[i] = sum;
						sum = 0.f;

						// DEBUG("here %f %f %f %f %f", phase[c/4][0], waveTri[0], sum[0], gain[0], gainCV[0]);
					}

					

				}

			} // end of oversampling loop

			// only downsample required channels
			for (int oct = 0; oct <= highestOutput; oct++) {
				if (outputs[OUT_01F_OUTPUT + oct].isConnected()) {

					// downsample (if required)
					float_4 out = (oversamplingRatio > 1) ? oversampler[oct][c/4].downsample() : oversampler[oct][c/4].getOSBuffer()[0];
					if (removePulseDC) {
						out = blockDCFilter[oct][c/4].process(out);
					}

					outputs[OUT_01F_OUTPUT + oct].setVoltageSimd(5.f * out, c);					
				}
			}
		}	// end of polyphony loop

		for (int c = 0; c < NUM_OUTPUTS; c++) {
			if (outputs[OUT_01F_OUTPUT + c].isConnected()) {
				outputs[OUT_01F_OUTPUT + c].setChannels(numActivePolyphonyEngines);
			}
		}
	}

	// polyphony is defined by the largest number of active channels on voct, pwm or gain inputs
	int getNumActivePolyphonyEngines() {
		int activePolyphonyEngines = 1;
		for (int c = 0; c < NUM_OUTPUTS; c++) {
			if (inputs[GAIN_01F_INPUT + c].isConnected()) {
				activePolyphonyEngines = std::max(activePolyphonyEngines, inputs[GAIN_01F_INPUT + c].getChannels());
			}
		}
		activePolyphonyEngines = std::max({activePolyphonyEngines, inputs[VOCT1_INPUT].getChannels(), inputs[VOCT2_INPUT].getChannels()});
		activePolyphonyEngines = std::max(activePolyphonyEngines, inputs[PWM_INPUT].getChannels());
		activePolyphonyEngines = std::max(activePolyphonyEngines, inputs[SYNC_INPUT].getChannels());

		return activePolyphonyEngines;
	}

	int getMaxConnectedOutput() {
		int maxChans = -1;
		for (int c = 0; c < NUM_OUTPUTS; c++) {
			if (outputs[OUT_01F_OUTPUT + c].isConnected()) {
				maxChans = c;
			}
		}
		return maxChans;
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "removePulseDC", json_boolean(removePulseDC));
		json_object_set_new(rootJ, "limitPW", json_boolean(limitPW));
		json_object_set_new(rootJ, "oversamplingIndex", json_integer(oversampler[0][0].getOversamplingIndex()));
		json_object_set_new(rootJ, "useTriangleCore", json_boolean(useTriangleCore));

		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {

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

		json_t* useTriangleCoreJ = json_object_get(rootJ, "useTriangleCore");
		if (useTriangleCoreJ) {
			useTriangleCore = json_boolean_value(useTriangleCoreJ);
		}
	}
};

struct OctavesWidget : ModuleWidget {
	OctavesWidget(Octaves* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Octaves.svg")));

		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<Knurlie>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<BefacoTinyKnobLightGrey>(mm2px(Vec(52.138, 15.037)), module, Octaves::PWM_CV_PARAM));
		addParam(createParam<CKSSVert7>(mm2px(Vec(22.171, 30.214)), module, Octaves::OCTAVE_PARAM));
		addParam(createParamCentered<BefacoTinyKnobLightGrey>(mm2px(Vec(10.264, 33.007)), module, Octaves::TUNE_PARAM));
		addParam(createParamCentered<Davies1900hLargeGreyKnob>(mm2px(Vec(45.384, 40.528)), module, Octaves::PWM_PARAM));
		addParam(createParam<CKSSThreeHorizontal>(mm2px(Vec(6.023, 48.937)), module, Octaves::RANGE_PARAM));
		addParam(createParam<BefacoSlidePotSmall>(mm2px(Vec(2.9830, 60.342)), module, Octaves::GAIN_01F_PARAM));
		addParam(createParam<BefacoSlidePotSmall>(mm2px(Vec(12.967, 60.342)), module, Octaves::GAIN_02F_PARAM));
		addParam(createParam<BefacoSlidePotSmall>(mm2px(Vec(22.951, 60.342)), module, Octaves::GAIN_04F_PARAM));
		addParam(createParam<BefacoSlidePotSmall>(mm2px(Vec(32.936, 60.342)), module, Octaves::GAIN_08F_PARAM));
		addParam(createParam<BefacoSlidePotSmall>(mm2px(Vec(42.920, 60.342)), module, Octaves::GAIN_16F_PARAM));
		addParam(createParam<BefacoSlidePotSmall>(mm2px(Vec(52.905, 60.342)), module, Octaves::GAIN_32F_PARAM));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(5.247, 15.181)), module, Octaves::VOCT1_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(15.282, 15.181)), module, Octaves::VOCT2_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(25.316, 15.181)), module, Octaves::SYNC_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(37.092, 15.135)), module, Octaves::PWM_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(5.247, 100.492)), module, Octaves::GAIN_01F_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(15.282, 100.492)), module, Octaves::GAIN_02F_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(25.316, 100.492)), module, Octaves::GAIN_04F_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(35.35, 100.492)), module, Octaves::GAIN_08F_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(45.384, 100.492)), module, Octaves::GAIN_16F_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(55.418, 100.492)), module, Octaves::GAIN_32F_INPUT));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(5.247, 113.508)), module, Octaves::OUT_01F_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(15.282, 113.508)), module, Octaves::OUT_02F_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(25.316, 113.508)), module, Octaves::OUT_04F_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(35.35, 113.508)), module, Octaves::OUT_08F_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(45.384, 113.508)), module, Octaves::OUT_16F_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(55.418, 113.508)), module, Octaves::OUT_32F_OUTPUT));

	}

	void appendContextMenu(Menu* menu) override {
		Octaves* module = dynamic_cast<Octaves*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator());
		menu->addChild(createSubmenuItem("Hardware compatibility", "",
		[ = ](Menu * menu) {
			menu->addChild(createBoolPtrMenuItem("Limit pulsewidth (5\%-95\%)", "", &module->limitPW));
			menu->addChild(createBoolPtrMenuItem("Remove pulse DC", "", &module->removePulseDC));
			menu->addChild(createBoolPtrMenuItem("Use triangle core", "", &module->useTriangleCore));
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

Model* modelOctaves = createModel<Octaves, OctavesWidget>("Octaves");
