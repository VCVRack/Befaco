#include "plugin.hpp"
#include "VariableOversampling.hpp"

static const size_t BUF_LEN = 32;

template <typename T>
T sin2pi_pade_05_5_4(T x) {
	x -= 0.5f;
	return (T(-6.283185307) * x + T(33.19863968) * simd::pow(x, 3) - T(32.44191367) * simd::pow(x, 5))
	       / (1 + T(1.296008659) * simd::pow(x, 2) + T(0.7028072946) * simd::pow(x, 4));
}

template <typename T>
T tanh_pade(T x) {
	T x2 = x * x;
	T q = 12.f + x2;
	return 12.f * x * q / (36.f * x2 + q * q);
}

static float foldResponse(float inputGain, float G) {
	return std::tanh(inputGain) + G * std::sin(M_PI * inputGain);
	//return tanh_pade(inputGain) + G * sin2pi_pade_05_5_4(inputGain / 2);
}

static float foldResponseSin(float inputGain) {
	return std::sin(2 * M_PI * inputGain / 2.5);
	//return sin2pi_pade_05_5_4(inputGain / 2.5);
	//return (std::abs(inputGain) < 1.0) ? inputGain : 1.0f;
}


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

	dsp::SchmittTrigger trigger;
	bool output_a_to_chopp;

	float previous_a = 0.0;


	VariableOversampling<> oversampler[NUM_CHANNELS];
	int oversamplingIndex = 2;

	dsp::RCFilter choppDC;
	bool blockDC = false;

	unsigned int currentFrame = 0;


	ChoppingKinky() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(FOLD_A_PARAM, 0.f, 2.f, 0.f, "");
		configParam(FOLD_B_PARAM, 0.f, 2.f, 0.f, "");
		configParam(CV_A_PARAM, -1.f, 1.f, 0.f, "");
		configParam(CV_B_PARAM, -1.f, 1.f, 0.f, "");

		// calculate up/downsampling rates
		onSampleRateChange();

	}

	void onSampleRateChange() override {
		// SampleRateConverter needs integer value
		int sampleRate = APP->engine->getSampleRate();

		choppDC.setCutoff(10.3f / sampleRate);

		for (int channel_idx = 0; channel_idx < NUM_CHANNELS; channel_idx++) {			
			oversampler[channel_idx].setOversamplingIndex(oversamplingIndex); 
			oversampler[channel_idx].reset(sampleRate);
		}
	}


	void process(const ProcessArgs& args) override {

		float gain_a = params[FOLD_A_PARAM].getValue();
		if (inputs[CV_A_INPUT].isConnected()) {
			gain_a += params[CV_A_PARAM].getValue() * inputs[CV_A_INPUT].getVoltage() / 10.f;
		}
		if (inputs[VCA_CV_A_INPUT].isConnected()) {
			gain_a += inputs[VCA_CV_A_INPUT].getVoltage() / 10.f;
		}

		float gain_b = params[FOLD_B_PARAM].getValue();
		if (inputs[CV_B_INPUT].isConnected()) {
			gain_b += params[CV_B_PARAM].getValue() * inputs[CV_B_INPUT].getVoltage() / 10.f;
		}
		if (inputs[VCA_CV_B_INPUT].isConnected()) {
			gain_b += inputs[VCA_CV_B_INPUT].getVoltage() / 10.f;
		}

		float in_a = 0;
		float in_b = 0;
		if (inputs[IN_A_INPUT].isConnected()) {
			in_a = inputs[IN_A_INPUT].getVoltage() / 5.0f;
		}
		if (inputs[IN_B_INPUT].isConnected()) {
			in_b = inputs[IN_B_INPUT].getVoltage() / 5.0f;
		}
		else if (inputs[IN_A_INPUT].isConnected()) {
			in_b = in_a;
		}

		// if the CHOPP gate is wired in, do chop logic
		if (inputs[IN_GATE_INPUT].isConnected()) {
			// TODO: check rescale?
			trigger.process(rescale(inputs[IN_GATE_INPUT].getVoltage(), 0.1f, 2.f, 0.f, 1.f));
			output_a_to_chopp = trigger.isHigh();
		}
		else {
			if (previous_a > 0 && in_a < 0) {
				output_a_to_chopp = false;
			}
			else if (previous_a < 0 && in_a > 0) {
				output_a_to_chopp = true;
			}
		}

		in_a = in_a * gain_a;
		in_b = in_b * gain_b;
		float in_chopp = output_a_to_chopp ? 1.f : 0.f;

		oversampler[CHANNEL_A].upsample(in_a);
		oversampler[CHANNEL_B].upsample(in_b);
		oversampler[CHANNEL_CHOPP].upsample(in_chopp);

		float* osBufferA = oversampler[CHANNEL_A].getOSBuffer();
		float* osBufferB = oversampler[CHANNEL_B].getOSBuffer();
		float* osBufferChopp = oversampler[CHANNEL_CHOPP].getOSBuffer();

		for (int i = 0; i < oversampler[0].getOversamplingRatio(); i++) {
			// TOO SLOW
			//break;
			osBufferA[i] = foldResponse(4.0f * osBufferA[i], -0.2);
			osBufferB[i] = foldResponseSin(4.0f * osBufferB[i]);
			osBufferChopp[i] = osBufferChopp[i] * osBufferA[i] + (1.f - osBufferChopp[i]) * osBufferB[i];
		}

		float out_a = oversampler[CHANNEL_A].downsample();
		float out_b = oversampler[CHANNEL_B].downsample();
		float out_chopp = oversampler[CHANNEL_CHOPP].downsample();

		if (blockDC) {
			choppDC.process(out_chopp);
			out_chopp -= choppDC.lowpass();
		}
		previous_a = in_a;

		outputs[OUT_A_OUTPUT].setVoltage(out_a * 5.0f);
		outputs[OUT_B_OUTPUT].setVoltage(out_b * 5.0f);
		outputs[OUT_CHOPP_OUTPUT].setVoltage(out_chopp * 5.0f);

		if (inputs[IN_GATE_INPUT].isConnected()) {
			lights[LED_A_LIGHT].setSmoothBrightness((float) output_a_to_chopp, args.sampleTime);
			lights[LED_B_LIGHT].setSmoothBrightness((float)(!output_a_to_chopp), args.sampleTime);
		}
		else {
			lights[LED_A_LIGHT].setBrightness(0.f);
			lights[LED_B_LIGHT].setBrightness(0.f);
		}
	}


	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "filterDC", json_boolean(blockDC));
		json_object_set_new(rootJ, "oversamplingIndex", json_integer(oversampler[0].getOversamplingIndex()));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* modeJ = json_object_get(rootJ, "filterDC");
		if (modeJ) {
			blockDC = json_boolean_value(modeJ);
		}

		json_t* modeJOS = json_object_get(rootJ, "oversamplingIndex");
		if (modeJOS) {
			oversamplingIndex = json_integer_value(modeJOS);
			onSampleRateChange();
		}
	}
};


struct ChoppingKinkyWidget : ModuleWidget {
	ChoppingKinkyWidget(ChoppingKinky* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ChoppingKinky.svg")));

		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<Knurlie>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Davies1900hLargeWhiteKnob>(mm2px(Vec(26.106, 22.108)), module, ChoppingKinky::FOLD_A_PARAM));
		addParam(createParamCentered<Davies1900hLargeWhiteKnob>(mm2px(Vec(26.106, 63.118)), module, ChoppingKinky::FOLD_B_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(10.515, 83.321)), module, ChoppingKinky::CV_A_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(30.583, 83.321)), module, ChoppingKinky::CV_B_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.35, 28.391)), module, ChoppingKinky::IN_A_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.35, 56.551)), module, ChoppingKinky::IN_B_INPUT));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(26.106, 42.613)), module, ChoppingKinky::IN_GATE_INPUT));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.189, 98.957)), module, ChoppingKinky::CV_A_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.19, 98.957)), module, ChoppingKinky::VCA_CV_A_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25.19, 98.957)), module, ChoppingKinky::CV_B_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(35.19, 98.957)), module, ChoppingKinky::VCA_CV_B_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(9.982, 111.076)), module, ChoppingKinky::OUT_A_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(31.558, 111.076)), module, ChoppingKinky::OUT_B_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.638, 110.15)), module, ChoppingKinky::OUT_CHOPP_OUTPUT));

		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(26.106, 33.342)), module, ChoppingKinky::LED_A_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(26.106, 51.717)), module, ChoppingKinky::LED_B_LIGHT));
	}

	struct DCMenuItem : MenuItem {
		ChoppingKinky* module;
		void onAction(const event::Action& e) override {
			module->blockDC ^= true;
		}
	};

	struct ModeItem : MenuItem {
		ChoppingKinky* module;
		int mode;
		void onAction(const event::Action& e) override {
			module->oversamplingIndex = mode;
			module->onSampleRateChange();
		}
	};

	void appendContextMenu(Menu* menu) override {
		ChoppingKinky* module = dynamic_cast<ChoppingKinky*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator());

		DCMenuItem* dcItem = createMenuItem<DCMenuItem>("Block DC on Chopp", CHECKMARK(module->blockDC));
		dcItem->module = module;
		menu->addChild(dcItem);

		menu->addChild(createMenuLabel("Oversampling mode"));

		for (int i = 0; i < 5; i++) {
			ModeItem* modeItem = createMenuItem<ModeItem>(std::to_string(int (1 << i)) + "x");
			modeItem->rightText = CHECKMARK(module->oversamplingIndex == i);
			modeItem->module = module;
			modeItem->mode = i;
			menu->addChild(modeItem);
		}
	}
};


Model* modelChoppingKinky = createModel<ChoppingKinky, ChoppingKinkyWidget>("ChoppingKinky");