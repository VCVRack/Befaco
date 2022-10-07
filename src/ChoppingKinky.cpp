#include "plugin.hpp"
#include "ChowDSP.hpp"


struct ChoppingKinky : Module {
	enum ParamIds {
		FOLD_A_PARAM,
		FOLD_B_PARAM,
		CV_A_PARAM,
		CV_B_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		IN_A_INPUT,
		IN_B_INPUT,
		IN_GATE_INPUT,
		CV_A_INPUT,
		VCA_CV_A_INPUT,
		CV_B_INPUT,
		VCA_CV_B_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_CHOPP_OUTPUT,
		OUT_A_OUTPUT,
		OUT_B_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		LED_A_LIGHT,
		LED_B_LIGHT,
		NUM_LIGHTS
	};
	enum {
		CHANNEL_A,
		CHANNEL_B,
		CHANNEL_CHOPP,
		NUM_CHANNELS
	};

	static const int WAVESHAPE_CACHE_SIZE = 256;
	float waveshapeA[WAVESHAPE_CACHE_SIZE + 1] = {};
	float waveshapeBPositive[WAVESHAPE_CACHE_SIZE + 1] = {};
	float waveshapeBNegative[WAVESHAPE_CACHE_SIZE + 1] = {};

	dsp::SchmittTrigger trigger;
	bool outputAToChopp = false;
	float previousA = 0.0;

	chowdsp::VariableOversampling<6> oversampler[NUM_CHANNELS]; 	// uses a 2*6=12th order Butterworth filter
	int oversamplingIndex = 2; 	// default is 2^oversamplingIndex == x4 oversampling

	DCBlocker blockDCFilter;
	bool blockDC = false;

	ChoppingKinky() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(FOLD_A_PARAM, 0.f, 2.f, 0.f, "Gain/shape control for channel A");
		configParam(FOLD_B_PARAM, 0.f, 2.f, 0.f, "Gain/shape control for channel B");
		configParam(CV_A_PARAM, -1.f, 1.f, 0.f, "Channel A CV control attenuverter");
		configParam(CV_B_PARAM, -1.f, 1.f, 0.f, "Channel A CV control attenuverter");

		configInput(IN_A_INPUT, "A");
		configInput(IN_B_INPUT, "B");
		configInput(IN_GATE_INPUT, "Chopp");
		configInput(CV_A_INPUT, "CV A (with attenuator)");
		configInput(VCA_CV_A_INPUT, "CV A");
		configInput(CV_B_INPUT, "CV B (with attenuator)");
		configInput(VCA_CV_B_INPUT, "CV B");

		getInputInfo(CV_B_INPUT)->description = "Normalled to CV A (with attenuator) Input";

		configOutput(OUT_CHOPP_OUTPUT, "Chopp");
		configOutput(OUT_A_OUTPUT, "A");
		configOutput(OUT_B_OUTPUT, "B");

		cacheWaveshaperResponses();

		// calculate up/downsampling rates
		onSampleRateChange();
	}

	void onSampleRateChange() override {
		float sampleRate = APP->engine->getSampleRate();

		blockDCFilter.setFrequency(22.05 / sampleRate);

		for (int channel_idx = 0; channel_idx < NUM_CHANNELS; channel_idx++) {
			oversampler[channel_idx].setOversamplingIndex(oversamplingIndex);
			oversampler[channel_idx].reset(sampleRate);
		}
	}

	void process(const ProcessArgs& args) override {

		float gainA = params[FOLD_A_PARAM].getValue();
		gainA += params[CV_A_PARAM].getValue() * inputs[CV_A_INPUT].getVoltage() / 10.f;
		gainA += inputs[VCA_CV_A_INPUT].getVoltage() / 10.f;
		gainA = std::max(gainA, 0.f);

		// CV_B_INPUT is normalled to CV_A_INPUT (input with attenuverter)
		float gainB = params[FOLD_B_PARAM].getValue();
		gainB += params[CV_B_PARAM].getValue() * inputs[CV_B_INPUT].getNormalVoltage(inputs[CV_A_INPUT].getVoltage()) / 10.f;
		gainB += inputs[VCA_CV_B_INPUT].getVoltage() / 10.f;
		gainB = std::max(gainB, 0.f);

		const float inA = inputs[IN_A_INPUT].getVoltageSum();
		const float inB = inputs[IN_B_INPUT].getNormalVoltage(inputs[IN_A_INPUT].getVoltageSum());

		// if the CHOPP gate is wired in, do chop logic
		if (inputs[IN_GATE_INPUT].isConnected()) {
			// TODO: check rescale?
			trigger.process(rescale(inputs[IN_GATE_INPUT].getVoltageSum(), 0.1f, 2.f, 0.f, 1.f));
			outputAToChopp = trigger.isHigh();
		}
		// else zero-crossing detector on input A switches between A and B
		else {
			if (previousA > 0 && inA < 0) {
				outputAToChopp = false;
			}
			else if (previousA < 0 && inA > 0) {
				outputAToChopp = true;
			}
		}
		previousA = inA;

		const bool choppIsRequired = outputs[OUT_CHOPP_OUTPUT].isConnected();
		const bool aIsRequired = outputs[OUT_A_OUTPUT].isConnected() || choppIsRequired;
		const bool bIsRequired = outputs[OUT_B_OUTPUT].isConnected() || choppIsRequired;

		if (aIsRequired) {
			oversampler[CHANNEL_A].upsample(inA * gainA);
		}
		if (bIsRequired) {
			oversampler[CHANNEL_B].upsample(inB * gainB);
		}
		if (choppIsRequired) {
			oversampler[CHANNEL_CHOPP].upsample(outputAToChopp ? 1.f : 0.f);
		}

		float* osBufferA = oversampler[CHANNEL_A].getOSBuffer();
		float* osBufferB = oversampler[CHANNEL_B].getOSBuffer();
		float* osBufferChopp = oversampler[CHANNEL_CHOPP].getOSBuffer();

		for (int i = 0; i < oversampler[0].getOversamplingRatio(); i++) {
			if (aIsRequired) {
				//osBufferA[i] = wavefolderAResponse(osBufferA[i]);
				osBufferA[i] = wavefolderAResponseCached(osBufferA[i]);
			}
			if (bIsRequired) {
				//osBufferB[i] = wavefolderBResponse(osBufferB[i]);
				osBufferB[i] = wavefolderBResponseCached(osBufferB[i]);
			}
			if (choppIsRequired) {
				osBufferChopp[i] = osBufferChopp[i] * osBufferA[i] + (1.f - osBufferChopp[i]) * osBufferB[i];
			}
		}

		float outA = aIsRequired ? oversampler[CHANNEL_A].downsample() : 0.f;
		float outB = bIsRequired ? oversampler[CHANNEL_B].downsample() : 0.f;
		float outChopp = choppIsRequired ? oversampler[CHANNEL_CHOPP].downsample() : 0.f;

		if (blockDC) {
			outChopp = blockDCFilter.process(outChopp);
		}

		outputs[OUT_A_OUTPUT].setVoltage(outA);
		outputs[OUT_B_OUTPUT].setVoltage(outB);
		outputs[OUT_CHOPP_OUTPUT].setVoltage(outChopp);

		if (inputs[IN_GATE_INPUT].isConnected()) {
			lights[LED_A_LIGHT].setSmoothBrightness((float) outputAToChopp, args.sampleTime);
			lights[LED_B_LIGHT].setSmoothBrightness((float)(!outputAToChopp), args.sampleTime);
		}
		else {
			lights[LED_A_LIGHT].setBrightness(0.f);
			lights[LED_B_LIGHT].setBrightness(0.f);
		}
	}

	float wavefolderAResponseCached(float x) {
		if (x >= 0) {
			float j = rescale(clamp(x, 0.f, 10.f), 0.f, 10.f, 0, WAVESHAPE_CACHE_SIZE - 1);
			return interpolateLinear(waveshapeA, j);
		}
		else {
			return -wavefolderAResponseCached(-x);
		}
	}

	float wavefolderBResponseCached(float x) {
		if (x >= 0) {
			float j = rescale(clamp(x, 0.f, 10.f), 0.f, 10.f, 0, WAVESHAPE_CACHE_SIZE - 1);
			return interpolateLinear(waveshapeBPositive, j);
		}
		else {
			float j = rescale(clamp(-x, 0.f, 10.f), 0.f, 10.f, 0, WAVESHAPE_CACHE_SIZE - 1);
			return interpolateLinear(waveshapeBNegative, j);
		}
	}

	static float wavefolderAResponse(float x) {
		if (x < 0) {
			return -wavefolderAResponse(-x);
		}

		float xScaleFactor = 1.f / 20.f;
		float yScaleFactor = 12.5f;
		x = x * xScaleFactor;

		float piecewiseX1 = 0.087;
		float piecewiseX2 = 0.245;
		float piecewiseX3 = 0.3252;

		if (x < piecewiseX1) {
			float x_ = x / piecewiseX1;
			return -0.38 * yScaleFactor * (std::sin(M_PI * std::pow(x_, 0.8)) + 1.0 / (3 * 1.6) * std::sin(3 * M_PI * std::pow(x_, 0.8)));
		}
		else if (x < piecewiseX2) {
			float x_ = x - piecewiseX1;
			return -yScaleFactor * (-0.2 * std::sin(0.5 * M_PI * 12.69 * x_) - 0.24 * std::sin(1.5 * M_PI * 12.69 * x_));
		}
		else if (x < piecewiseX3) {
			float x_ = 9.8 * (x - piecewiseX2);
			return -0.33 * yScaleFactor * std::sin(x_ / 0.165) * (1 + 0.9 * std::pow(x_, 3) / (1.0 + 2.0 * std::pow(x_, 6)));
		}
		else {
			float x_ = (x - piecewiseX3) / 0.05;
			return yScaleFactor * ((0.4274 - 0.031) * std::exp(-std::pow(x_, 2.0)) + 0.031);
		}
	}

	static float wavefolderBResponse(float x) {
		float xScaleFactor = 1.f / 20.f;
		float yScaleFactor = 12.5f;
		x = x * xScaleFactor;

		// assymetric response
		if (x > 0) {
			float piecewiseX1 = 0.117;
			float piecewiseX2 = 0.2837;

			if (x <  piecewiseX1) {
				float x_ = x / piecewiseX1;
				return -0.3 * yScaleFactor * (std::sin(M_PI * std::pow(x_, 0.67)) + 1.0 / (3 * 0.8) * std::sin(3 * M_PI * std::pow(x_, 0.67)));
			}
			else if (x < piecewiseX2) {
				float x_ = x - piecewiseX1;
				return 0.35 * yScaleFactor * std::sin(12. * M_PI * x_);
			}
			else {
				float x_ = (x - piecewiseX2);
				return 0.57 * yScaleFactor * std::tanh(x_ / 0.03);
			}
		}
		else {
			float piecewiseX1 = -0.105;
			float piecewiseX2 = -0.20722;

			if (x > piecewiseX1) {
				float x_ = x / piecewiseX1;
				return 0.37 * yScaleFactor * (std::sin(M_PI * std::pow(x_, 0.65)) + 1.0 / (3 * 1.2) * std::sin(3 * M_PI * std::pow(x_, 0.65)));
			}
			else if (x > piecewiseX2) {
				float x_ = x - piecewiseX1;
				return 0.2 * yScaleFactor * std::sin(15 * M_PI * x_) * (1.0 - 10.f * x_);
			}
			else {
				float x_ = (x - piecewiseX2) / 0.07;
				return yScaleFactor * ((0.4022 - 0.065) * std::exp(-std::pow(x_, 2)) + 0.065);
			}
		}
	}

	// functional form for waveshapers uses a lot of transcendental functions, so we cache
	// the response in a LUT
	void cacheWaveshaperResponses() {
		for (int i = 0; i < WAVESHAPE_CACHE_SIZE; ++i) {
			float x = rescale(i, 0, WAVESHAPE_CACHE_SIZE - 1, 0.0, 10.f);
			waveshapeA[i] = wavefolderAResponse(x);
			waveshapeBPositive[i] = wavefolderBResponse(+x);
			waveshapeBNegative[i] = wavefolderBResponse(-x);
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "filterDC", json_boolean(blockDC));
		json_object_set_new(rootJ, "oversamplingIndex", json_integer(oversampler[0].getOversamplingIndex()));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* filterDCJ = json_object_get(rootJ, "filterDC");
		if (filterDCJ) {
			blockDC = json_boolean_value(filterDCJ);
		}

		json_t* oversamplingIndexJ = json_object_get(rootJ, "oversamplingIndex");
		if (oversamplingIndexJ) {
			oversamplingIndex = json_integer_value(oversamplingIndexJ);
			onSampleRateChange();
		}
	}
};


struct ChoppingKinkyWidget : ModuleWidget {
	ChoppingKinkyWidget(ChoppingKinky* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/ChoppingKinky.svg")));

		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<Knurlie>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Davies1900hLargeWhiteKnob>(mm2px(Vec(26.051, 21.999)), module, ChoppingKinky::FOLD_A_PARAM));
		addParam(createParamCentered<Davies1900hLargeWhiteKnob>(mm2px(Vec(26.051, 62.768)), module, ChoppingKinky::FOLD_B_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(10.266, 83.297)), module, ChoppingKinky::CV_A_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(30.277, 83.297)), module, ChoppingKinky::CV_B_PARAM));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.127, 27.843)), module, ChoppingKinky::IN_A_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(26.057, 42.228)), module, ChoppingKinky::IN_GATE_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(6.104, 56.382)), module, ChoppingKinky::IN_B_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(5.209, 98.499)), module, ChoppingKinky::CV_A_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(15.259, 98.499)), module, ChoppingKinky::VCA_CV_A_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(25.308, 98.499)), module, ChoppingKinky::CV_B_INPUT));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(35.358, 98.499)), module, ChoppingKinky::VCA_CV_B_INPUT));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(20.23, 109.669)), module, ChoppingKinky::OUT_CHOPP_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(31.091, 110.747)), module, ChoppingKinky::OUT_B_OUTPUT));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(9.589, 110.777)), module, ChoppingKinky::OUT_A_OUTPUT));

		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(26.057, 33.307)), module, ChoppingKinky::LED_A_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(26.057, 51.53)), module, ChoppingKinky::LED_B_LIGHT));
	}

	void appendContextMenu(Menu* menu) override {
		ChoppingKinky* module = dynamic_cast<ChoppingKinky*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator());
		menu->addChild(createBoolPtrMenuItem("Block DC on Chopp", "", &module->blockDC));

		menu->addChild(createMenuLabel("Oversampling mode"));

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


Model* modelChoppingKinky = createModel<ChoppingKinky, ChoppingKinkyWidget>("ChoppingKinky");