#include "plugin.hpp"


Plugin *pluginInstance;

void init(rack::Plugin *p) {
	pluginInstance = p;

	p->addModel(modelEvenVCO);
	p->addModel(modelRampage);
	p->addModel(modelABC);
	p->addModel(modelSpringReverb);
	p->addModel(modelMixer);
	p->addModel(modelSlewLimiter);
	p->addModel(modelDualAtenuverter);
	p->addModel(modelPercall);
	p->addModel(modelHexmixVCA);
	p->addModel(modelChoppingKinky);
	p->addModel(modelKickall);
	p->addModel(modelSamplingModulator);
}
