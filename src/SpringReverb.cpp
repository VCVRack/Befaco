#include "plugin.hpp"
#include <pffft.h>

static std::vector<uint8_t> ir;

static void initIR() {
	if (!ir.empty())
		return;

	try {
		ir = system::readFile(asset::plugin(pluginInstance, "res/SpringReverbIR.f32"));
	}
	catch (std::exception& e) {
		WARN("Cannot load IR: %s", e.what());
	}
}

static const size_t BLOCK_SIZE = 1024;


struct SpringReverb : Module {
	enum ParamIds {
		WET_PARAM,
		LEVEL1_PARAM,
		LEVEL2_PARAM,
		HPF_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CV1_INPUT,
		CV2_INPUT,
		IN1_INPUT,
		IN2_INPUT,
		MIX_CV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		MIX_OUTPUT,
		WET_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		PEAK_LIGHT,
		ENUMS(VU1_LIGHTS, 7),
		NUM_LIGHTS
	};

	dsp::RealTimeConvolver* convolver = NULL;
	dsp::SampleRateConverter<1> inputSrc;
	dsp::SampleRateConverter<1> outputSrc;
	dsp::DoubleRingBuffer<dsp::Frame<1>, 16 * BLOCK_SIZE> inputBuffer;
	dsp::DoubleRingBuffer<dsp::Frame<1>, 16 * BLOCK_SIZE> outputBuffer;

	dsp::RCFilter dryFilter;

	dsp::VuMeter2 vuFilter;
	dsp::VuMeter2 lightFilter;
	dsp::ClockDivider lightRefreshClock;

	const float brightnessIntervals[8] = {17.f, 14.f, 12.f, 9.f, 6.f, 0.f, -6.f, -12.f};

	SpringReverb() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(WET_PARAM, 0.0, 1.0, 0.5, "Dry/wet", "%", 0, 100);
		configParam(LEVEL1_PARAM, 0.0, 1.0, 0.0, "In 1 level", "%", 0, 100);
		configParam(LEVEL2_PARAM, 0.0, 1.0, 0.0, "In 2 level", "%", 0, 100);
		configParam(HPF_PARAM, 0.0, 1.0, 0.5, "High pass filter cutoff");

		initIR();

		convolver = new dsp::RealTimeConvolver(BLOCK_SIZE);

		const float* kernel = (const float*) ir.data();
		size_t kernelLen = ir.size() / sizeof(float);
		convolver->setKernel(kernel, kernelLen);

		vuFilter.mode = dsp::VuMeter2::PEAK;
		lightFilter.mode = dsp::VuMeter2::PEAK;

		lightRefreshClock.setDivision(32);
	}

	~SpringReverb() {
		delete convolver;
	}

	void processBypass(const ProcessArgs& args) override {
		float in1 = inputs[IN1_INPUT].getVoltageSum();
		float in2 = inputs[IN2_INPUT].getVoltageSum();

		float dry = clamp(in1 + in2, -10.0f, 10.0f);

		outputs[WET_OUTPUT].setVoltage(dry);
		outputs[MIX_OUTPUT].setVoltage(dry);
	}

	void process(const ProcessArgs& args) override {
		float in1 = inputs[IN1_INPUT].getVoltageSum();
		float in2 = inputs[IN2_INPUT].getVoltageSum();
		const float levelScale = 0.030;
		const float levelBase = 25.0;
		float level1 = levelScale * dsp::exponentialBipolar(levelBase, params[LEVEL1_PARAM].getValue()) * inputs[CV1_INPUT].getNormalVoltage(10.0) / 10.0;
		float level2 = levelScale * dsp::exponentialBipolar(levelBase, params[LEVEL2_PARAM].getValue()) * inputs[CV2_INPUT].getNormalVoltage(10.0) / 10.0;
		float dry = in1 * level1 + in2 * level2;

		// HPF on dry
		float dryCutoff = 200.0 * std::pow(20.0, params[HPF_PARAM].getValue()) * args.sampleTime;
		dryFilter.setCutoff(dryCutoff);
		dryFilter.process(dry);

		// Add dry to input buffer
		if (!inputBuffer.full()) {
			dsp::Frame<1> inputFrame;
			inputFrame.samples[0] = dryFilter.highpass();
			inputBuffer.push(inputFrame);
		}


		if (outputBuffer.empty()) {
			float input[BLOCK_SIZE] = {};
			float output[BLOCK_SIZE];
			// Convert input buffer
			{
				inputSrc.setRates(args.sampleRate, 48000);
				int inLen = inputBuffer.size();
				int outLen = BLOCK_SIZE;
				inputSrc.process(inputBuffer.startData(), &inLen, (dsp::Frame<1>*) input, &outLen);
				inputBuffer.startIncr(inLen);
			}

			// Convolve block
			convolver->processBlock(input, output);

			// Convert output buffer
			{
				outputSrc.setRates(48000, args.sampleRate);
				int inLen = BLOCK_SIZE;
				int outLen = outputBuffer.capacity();
				outputSrc.process((dsp::Frame<1>*) output, &inLen, outputBuffer.endData(), &outLen);
				outputBuffer.endIncr(outLen);
			}
		}

		// Set output
		if (outputBuffer.empty())
			return;

		float wet = outputBuffer.shift().samples[0];
		float balance = clamp(params[WET_PARAM].getValue() + inputs[MIX_CV_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);
		float mix = crossfade(in1, wet, balance);

		outputs[WET_OUTPUT].setVoltage(clamp(wet, -10.0f, 10.0f));
		outputs[MIX_OUTPUT].setVoltage(clamp(mix, -10.0f, 10.0f));

		// process VU lights
		vuFilter.process(args.sampleTime, wet);
		// process peak light
		lightFilter.process(args.sampleTime, dry * 50.0);

		if (lightRefreshClock.process()) {

			for (int i = 0; i < 7; i++) {
				float brightness = vuFilter.getBrightness(brightnessIntervals[i + 1], brightnessIntervals[i]);
				lights[VU1_LIGHTS + i].setBrightness(brightness);
			}

			lights[PEAK_LIGHT].value = lightFilter.v;
		}
	}
};


struct SpringReverbWidget : ModuleWidget {
	SpringReverbWidget(SpringReverb* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/SpringReverb.svg")));

		addChild(createWidget<Knurlie>(Vec(15, 0)));
		addChild(createWidget<Knurlie>(Vec(15, 365)));
		addChild(createWidget<Knurlie>(Vec(15 * 6, 0)));
		addChild(createWidget<Knurlie>(Vec(15 * 6, 365)));

		addParam(createParam<BefacoBigKnob>(Vec(22, 29), module, SpringReverb::WET_PARAM));

		addParam(createParam<BefacoSlidePot>(Vec(12, 116), module, SpringReverb::LEVEL1_PARAM));
		addParam(createParam<BefacoSlidePot>(Vec(93, 116), module, SpringReverb::LEVEL2_PARAM));

		addParam(createParam<Davies1900hWhiteKnob>(Vec(42, 210), module, SpringReverb::HPF_PARAM));

		addInput(createInput<BefacoInputPort>(Vec(7, 243), module, SpringReverb::CV1_INPUT));
		addInput(createInput<BefacoInputPort>(Vec(88, 243), module, SpringReverb::CV2_INPUT));
		addInput(createInput<BefacoInputPort>(Vec(27, 281), module, SpringReverb::IN1_INPUT));
		addInput(createInput<BefacoInputPort>(Vec(67, 281), module, SpringReverb::IN2_INPUT));

		addOutput(createOutput<BefacoOutputPort>(Vec(7, 317), module, SpringReverb::MIX_OUTPUT));
		addInput(createInput<BefacoInputPort>(Vec(47, 324), module, SpringReverb::MIX_CV_INPUT));
		addOutput(createOutput<BefacoOutputPort>(Vec(88, 317), module, SpringReverb::WET_OUTPUT));

		addChild(createLight<MediumLight<GreenRedLight>>(Vec(55, 269), module, SpringReverb::PEAK_LIGHT));
		addChild(createLight<MediumLight<RedLight>>(Vec(55, 113), module, SpringReverb::VU1_LIGHTS + 0));
		addChild(createLight<MediumLight<YellowLight>>(Vec(55, 126), module, SpringReverb::VU1_LIGHTS + 1));
		addChild(createLight<MediumLight<YellowLight>>(Vec(55, 138), module, SpringReverb::VU1_LIGHTS + 2));
		addChild(createLight<MediumLight<GreenLight>>(Vec(55, 150), module, SpringReverb::VU1_LIGHTS + 3));
		addChild(createLight<MediumLight<GreenLight>>(Vec(55, 163), module, SpringReverb::VU1_LIGHTS + 4));
		addChild(createLight<MediumLight<GreenLight>>(Vec(55, 175), module, SpringReverb::VU1_LIGHTS + 5));
		addChild(createLight<MediumLight<GreenLight>>(Vec(55, 188), module, SpringReverb::VU1_LIGHTS + 6));
	}
};


Model* modelSpringReverb = createModel<SpringReverb, SpringReverbWidget>("SpringReverb");
