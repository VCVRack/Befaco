#include <string.h>
#include "Befaco.hpp"
#include "dsp/functions.hpp"
#include "dsp/samplerate.hpp"
#include "dsp/ringbuffer.hpp"
#include "dsp/filter.hpp"
#include "dsp/fir.hpp"
#include "pffft.h"


float *springReverbIR;
int springReverbIRLen;

void springReverbInit() {
	std::string irFilename = assetPlugin(plugin, "res/SpringReverbIR.pcm");
	FILE *f = fopen(irFilename.c_str(), "rb");
	assert(f);
	fseek(f, 0, SEEK_END);
	int size = ftell(f);
	fseek(f, 0, SEEK_SET);

	springReverbIRLen = size / sizeof(float);
	springReverbIR = new float[springReverbIRLen];
	fread(springReverbIR, sizeof(float), springReverbIRLen, f);
	fclose(f);

	// TODO Add springReverbDestroy() function once plugins have destroy() callbacks
}


#define BLOCKSIZE 1024

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
		VU1_LIGHT,
		NUM_LIGHTS = VU1_LIGHT + 7
	};

	RealTimeConvolver *convolver = NULL;
	SampleRateConverter<1> inputSrc;
	SampleRateConverter<1> outputSrc;
	DoubleRingBuffer<Frame<1>, 16*BLOCKSIZE> inputBuffer;
	DoubleRingBuffer<Frame<1>, 16*BLOCKSIZE> outputBuffer;

	RCFilter dryFilter;
	PeakFilter vuFilter;
	PeakFilter lightFilter;

	SpringReverb();
	~SpringReverb();
	void step() override;
};


SpringReverb::SpringReverb() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
	convolver = new RealTimeConvolver(BLOCKSIZE);
	convolver->setKernel(springReverbIR, springReverbIRLen);
}

SpringReverb::~SpringReverb() {
	delete convolver;
}

void SpringReverb::step() {
	float in1 = inputs[IN1_INPUT].value;
	float in2 = inputs[IN2_INPUT].value;
	const float levelScale = 0.030;
	const float levelBase = 25.0;
	float level1 = levelScale * exponentialBipolar(levelBase, params[LEVEL1_PARAM].value) * inputs[CV1_INPUT].normalize(10.0) / 10.0;
	float level2 = levelScale * exponentialBipolar(levelBase, params[LEVEL2_PARAM].value) * inputs[CV2_INPUT].normalize(10.0) / 10.0;
	float dry = in1 * level1 + in2 * level2;

	// HPF on dry
	float dryCutoff = 200.0 * powf(20.0, params[HPF_PARAM].value) * engineGetSampleTime();
	dryFilter.setCutoff(dryCutoff);
	dryFilter.process(dry);

	// Add dry to input buffer
	if (!inputBuffer.full()) {
		Frame<1> inputFrame;
		inputFrame.samples[0] = dryFilter.highpass();
		inputBuffer.push(inputFrame);
	}


	if (outputBuffer.empty()) {
		float input[BLOCKSIZE] = {};
		float output[BLOCKSIZE];
		// Convert input buffer
		{
			inputSrc.setRates(engineGetSampleRate(), 48000);
			int inLen = inputBuffer.size();
			int outLen = BLOCKSIZE;
			inputSrc.process(inputBuffer.startData(), &inLen, (Frame<1>*) input, &outLen);
			inputBuffer.startIncr(inLen);
		}

		// Convolve block
		convolver->processBlock(input, output);

		// Convert output buffer
		{
			outputSrc.setRates(48000, engineGetSampleRate());
			int inLen = BLOCKSIZE;
			int outLen = outputBuffer.capacity();
			outputSrc.process((Frame<1>*) output, &inLen, outputBuffer.endData(), &outLen);
			outputBuffer.endIncr(outLen);
		}
	}

	// Set output
	if (outputBuffer.empty())
		return;
	float wet = outputBuffer.shift().samples[0];
	float balance = clamp(params[WET_PARAM].value + inputs[MIX_CV_INPUT].value / 10.0f, 0.0f, 1.0f);
	float mix = crossfade(in1, wet, balance);

	outputs[WET_OUTPUT].value = clamp(wet, -10.0f, 10.0f);
	outputs[MIX_OUTPUT].value = clamp(mix, -10.0f, 10.0f);

	// Set lights
	float lightRate = 5.0 * engineGetSampleTime();
	vuFilter.setRate(lightRate);
	vuFilter.process(fabsf(wet));
	lightFilter.setRate(lightRate);
	lightFilter.process(fabsf(dry*50.0));

	float vuValue = vuFilter.peak();
	for (int i = 0; i < 7; i++) {
		float light = powf(1.413, i) * vuValue / 10.0 - 1.0;
		lights[VU1_LIGHT + i].value = clamp(light, 0.0f, 1.0f);
	}
	lights[PEAK_LIGHT].value = lightFilter.peak();
}


struct SpringReverbWidget : ModuleWidget {
	SpringReverbWidget(SpringReverb *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/SpringReverb.svg")));

		addChild(Widget::create<Knurlie>(Vec(15, 0)));
		addChild(Widget::create<Knurlie>(Vec(15, 365)));
		addChild(Widget::create<Knurlie>(Vec(15*6, 0)));
		addChild(Widget::create<Knurlie>(Vec(15*6, 365)));

		addParam(ParamWidget::create<BefacoBigKnob>(Vec(22, 29), module, SpringReverb::WET_PARAM, 0.0, 1.0, 0.5));

		addParam(ParamWidget::create<BefacoSlidePot>(Vec(12, 116), module, SpringReverb::LEVEL1_PARAM, 0.0, 1.0, 0.0));
		addParam(ParamWidget::create<BefacoSlidePot>(Vec(93, 116), module, SpringReverb::LEVEL2_PARAM, 0.0, 1.0, 0.0));

		addParam(ParamWidget::create<Davies1900hWhiteKnob>(Vec(42, 210), module, SpringReverb::HPF_PARAM, 0.0, 1.0, 0.5));

		addInput(Port::create<PJ301MPort>(Vec(7, 243), Port::INPUT, module, SpringReverb::CV1_INPUT));
		addInput(Port::create<PJ301MPort>(Vec(88, 243), Port::INPUT, module, SpringReverb::CV2_INPUT));
		addInput(Port::create<PJ301MPort>(Vec(27, 281), Port::INPUT, module, SpringReverb::IN1_INPUT));
		addInput(Port::create<PJ301MPort>(Vec(67, 281), Port::INPUT, module, SpringReverb::IN2_INPUT));

		addOutput(Port::create<PJ301MPort>(Vec(7, 317), Port::OUTPUT, module, SpringReverb::MIX_OUTPUT));
		addInput(Port::create<PJ301MPort>(Vec(47, 324), Port::INPUT, module, SpringReverb::MIX_CV_INPUT));
		addOutput(Port::create<PJ301MPort>(Vec(88, 317), Port::OUTPUT, module, SpringReverb::WET_OUTPUT));

		addChild(ModuleLightWidget::create<MediumLight<GreenRedLight>>(Vec(55, 269), module, SpringReverb::PEAK_LIGHT));
		addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(55, 113), module, SpringReverb::VU1_LIGHT + 0));
		addChild(ModuleLightWidget::create<MediumLight<YellowLight>>(Vec(55, 126), module, SpringReverb::VU1_LIGHT + 1));
		addChild(ModuleLightWidget::create<MediumLight<YellowLight>>(Vec(55, 138), module, SpringReverb::VU1_LIGHT + 2));
		addChild(ModuleLightWidget::create<MediumLight<GreenLight>>(Vec(55, 150), module, SpringReverb::VU1_LIGHT + 3));
		addChild(ModuleLightWidget::create<MediumLight<GreenLight>>(Vec(55, 163), module, SpringReverb::VU1_LIGHT + 4));
		addChild(ModuleLightWidget::create<MediumLight<GreenLight>>(Vec(55, 175), module, SpringReverb::VU1_LIGHT + 5));
		addChild(ModuleLightWidget::create<MediumLight<GreenLight>>(Vec(55, 188), module, SpringReverb::VU1_LIGHT + 6));
	}
};


Model *modelSpringReverb = Model::create<SpringReverb, SpringReverbWidget>("Befaco", "SpringReverb", "Spring Reverb", REVERB_TAG, DUAL_TAG);
