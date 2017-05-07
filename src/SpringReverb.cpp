#include <string.h>
#include "Befaco.hpp"
#include "dsp.hpp"
#include "pffft.h"


float *springReverbIR;
int springReverbIRLen;

void springReverbInit() {
	FILE *f = fopen("plugins/Befaco/res/SpringReverbIR.pcm", "rb");
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


struct RealTimeConvolver {
	// `kernelBlocks` number of contiguous FFT blocks of size `blockSize`
	// indexed by [i * blockSize*2 + j]
	float *kernelFfts = NULL;
	float *inputFfts = NULL;
	float *outputTail = NULL;
	float *tmpBlock = NULL;
	size_t blockSize;
	size_t kernelBlocks = 0;
	size_t inputPos = 0;
	// kiss_fftr_cfg fft_cfg;
	// kiss_fftr_cfg ifft_cfg;
	PFFFT_Setup *pffft;

	/** blocksize should be >=32 and a power of 2 */
	RealTimeConvolver(size_t blockSize) {
		this->blockSize = blockSize;
		pffft = pffft_new_setup(blockSize*2, PFFFT_REAL);
		outputTail = new float[blockSize]();
		tmpBlock = new float[blockSize*2]();
	}

	~RealTimeConvolver() {
		clear();
		delete[] outputTail;
		delete[] tmpBlock;
		pffft_destroy_setup(pffft);
	}

	void clear() {
		if (kernelFfts) {
			pffft_aligned_free(kernelFfts);
			kernelFfts = NULL;
		}
		if (inputFfts) {
			pffft_aligned_free(inputFfts);
			inputFfts = NULL;
		}
		kernelBlocks = 0;
		inputPos = 0;
	}

	void setKernel(const float *kernel, size_t length) {
		clear();

		if (!kernel || length == 0)
			return;

		// Round up to the nearest factor blockSize
		kernelBlocks = (length - 1) / blockSize + 1;

		// Allocate blocks
		kernelFfts = (float*) pffft_aligned_malloc(sizeof(float) * blockSize*2 * kernelBlocks);
		inputFfts = (float*) pffft_aligned_malloc(sizeof(float) * blockSize*2 * kernelBlocks);
		memset(inputFfts, 0, sizeof(float) * blockSize*2 * kernelBlocks);

		for (size_t i = 0; i < kernelBlocks; i++) {
			// Pad each block with zeros
			memset(tmpBlock, 0, sizeof(float) * blockSize*2);
			size_t len = mini(blockSize, length - i*blockSize);
			memcpy(tmpBlock, &kernel[i*blockSize], sizeof(float)*len);
			// Compute fft
			pffft_transform(pffft, tmpBlock, &kernelFfts[blockSize*2 * i], NULL, PFFFT_FORWARD);
		}
	}

	/** Applies reverb to input
	input and output must be size blockSize
	*/
	void processBlock(const float *input, float *output) {
		if (kernelBlocks == 0) {
			memset(output, 0, sizeof(float) * blockSize);
			return;
		}

		// Step input position
		inputPos = (inputPos + 1) % kernelBlocks;
		// Pad block with zeros
		memset(tmpBlock, 0, sizeof(float) * blockSize*2);
		memcpy(tmpBlock, input, sizeof(float) * blockSize);
		// Compute input fft
		pffft_transform(pffft, tmpBlock, &inputFfts[blockSize*2 * inputPos], NULL, PFFFT_FORWARD);
		// Create output fft
		memset(tmpBlock, 0, sizeof(float) * blockSize*2);
		// convolve input fft by kernel fft
		// Note: This is the CPU bottleneck loop
		for (size_t i = 0; i < kernelBlocks; i++) {
			size_t pos = (inputPos - i + kernelBlocks) % kernelBlocks;
			pffft_zconvolve_accumulate(pffft, &kernelFfts[blockSize*2 * i], &inputFfts[blockSize*2 * pos], tmpBlock, 1.0);
		}
		// Compute output
		pffft_transform(pffft, tmpBlock, tmpBlock, NULL, PFFFT_BACKWARD);
		// Add block tail from last output block
		for (size_t i = 0; i < blockSize; i++) {
			tmpBlock[i] += outputTail[i];
		}
		// Copy output block to output
		for (size_t i = 0; i < blockSize; i++) {
			// Scale based on FFT
			output[i] = tmpBlock[i] / blockSize;
		}
		// Set tail
		for (size_t i = 0; i < blockSize; i++) {
			outputTail[i] = tmpBlock[i + blockSize];
		}
	}
};


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

	RealTimeConvolver *convolver = NULL;
	SampleRateConverter<1> inputSrc;
	SampleRateConverter<1> outputSrc;
	DoubleRingBuffer<Frame<1>, 16*BLOCKSIZE> inputBuffer;
	DoubleRingBuffer<Frame<1>, 16*BLOCKSIZE> outputBuffer;

	RCFilter dryFilter;
	PeakFilter vuFilter;
	PeakFilter lightFilter;
	float vuLights[7] = {};
	float lights[1] = {};

	SpringReverb();
	~SpringReverb();
	void step();
};


SpringReverb::SpringReverb() {
	params.resize(NUM_PARAMS);
	inputs.resize(NUM_INPUTS);
	outputs.resize(NUM_OUTPUTS);

	convolver = new RealTimeConvolver(BLOCKSIZE);
	convolver->setKernel(springReverbIR, springReverbIRLen);
}

SpringReverb::~SpringReverb() {
	delete convolver;
}

void SpringReverb::step() {
	float in1 = getf(inputs[IN1_INPUT]);
	float in2 = getf(inputs[IN2_INPUT]);
	const float levelScale = 0.030;
	const float levelBase = 25.0;
	float level1 = levelScale * exponentialBipolar(levelBase, params[LEVEL1_PARAM]) * getf(inputs[CV1_INPUT], 10.0) / 10.0;
	float level2 = levelScale * exponentialBipolar(levelBase, params[LEVEL2_PARAM]) * getf(inputs[CV2_INPUT], 10.0) / 10.0;
	float dry = in1 * level1 + in2 * level2;

	// HPF on dry
	float dryCutoff = 200.0 * powf(20.0, params[HPF_PARAM]) / gSampleRate;
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
			inputSrc.setRatio(48000.0 / gSampleRate);
			int inLen = inputBuffer.size();
			int outLen = BLOCKSIZE;
			inputSrc.process(inputBuffer.startData(), &inLen, (Frame<1>*) input, &outLen);
			inputBuffer.startIncr(inLen);
		}

		// Convolve block
		convolver->processBlock(input, output);

		// Convert output buffer
		{
			outputSrc.setRatio(gSampleRate / 48000.0);
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
	float crossfade = clampf(params[WET_PARAM] + getf(inputs[MIX_CV_INPUT]) / 10.0, 0.0, 1.0);
	float mix = crossf(in1, wet, crossfade);

	setf(outputs[WET_OUTPUT], clampf(wet, -10.0, 10.0));
	setf(outputs[MIX_OUTPUT], clampf(mix, -10.0, 10.0));

	// Set lights
	float lightRate = 5.0 / gSampleRate;
	vuFilter.setRate(lightRate);
	vuFilter.process(fabsf(wet));
	lightFilter.setRate(lightRate);
	lightFilter.process(fabsf(dry*50.0));

	float vuValue = vuFilter.peak();
	for (int i = 0; i < 7; i++) {
		float light = powf(1.413, i) * vuValue / 10.0 - 1.0;
		vuLights[i] = clampf(light, 0.0, 1.0);
	}
	lights[0] = lightFilter.peak();
}


SpringReverbWidget::SpringReverbWidget() {
	SpringReverb *module = new SpringReverb();
	setModule(module);
	box.size = Vec(15*8, 380);

	{
		Panel *panel = new DarkPanel();
		panel->box.size = box.size;
		panel->backgroundImage = Image::load("plugins/Befaco/res/SpringReverb.png");
		addChild(panel);
	}

	addChild(createScrew<ScrewBlack>(Vec(15, 0)));
	addChild(createScrew<ScrewBlack>(Vec(15, 365)));
	addChild(createScrew<ScrewBlack>(Vec(15*6, 0)));
	addChild(createScrew<ScrewBlack>(Vec(15*6, 365)));

	addParam(createParam<BefacoBigKnob>(Vec(22, 29), module, SpringReverb::WET_PARAM, 0.0, 1.0, 0.5));

	addParam(createParam<BefacoSlidePot>(Vec(12, 116), module, SpringReverb::LEVEL1_PARAM, 0.0, 1.0, 0.0));
	addParam(createParam<BefacoSlidePot>(Vec(92, 116), module, SpringReverb::LEVEL2_PARAM, 0.0, 1.0, 0.0));

	addParam(createParam<Davies1900hWhiteKnob>(Vec(41, 209), module, SpringReverb::HPF_PARAM, 0.0, 1.0, 0.5));

	addInput(createInput<PJ3410Port>(Vec(6-3, 243-3), module, SpringReverb::CV1_INPUT));
	addInput(createInput<PJ3410Port>(Vec(86-3, 243-3), module, SpringReverb::CV2_INPUT));
	addInput(createInput<PJ3410Port>(Vec(26-3, 281-3), module, SpringReverb::IN1_INPUT));
	addInput(createInput<PJ3410Port>(Vec(65-3, 281-3), module, SpringReverb::IN2_INPUT));

	addOutput(createOutput<PJ3410Port>(Vec(6-3, 317-3), module, SpringReverb::MIX_OUTPUT));
	addInput(createInput<PJ3410Port>(Vec(46-3, 324-3), module, SpringReverb::MIX_CV_INPUT));
	addOutput(createOutput<PJ3410Port>(Vec(87-3, 317-3), module, SpringReverb::WET_OUTPUT));

	addChild(createValueLight<SmallLight<RedValueLight>>(Vec(55, 114), &module->vuLights[0]));
	addChild(createValueLight<SmallLight<YellowValueLight>>(Vec(55, 127), &module->vuLights[1]));
	addChild(createValueLight<SmallLight<YellowValueLight>>(Vec(55, 139), &module->vuLights[2]));
	addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(55, 151), &module->vuLights[3]));
	addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(55, 164), &module->vuLights[4]));
	addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(55, 176), &module->vuLights[5]));
	addChild(createValueLight<SmallLight<GreenValueLight>>(Vec(55, 189), &module->vuLights[6]));

	addChild(createValueLight<SmallLight<GreenRedPolarityLight>>(Vec(55, 270), &module->lights[0]));
}
