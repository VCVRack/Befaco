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
	p->addModel(modelMorphader);
	p->addModel(modelADSR);
	p->addModel(modelSTMix);
	p->addModel(modelMuxlicer);
	p->addModel(modelMex);
	p->addModel(modelNoisePlethora);
	p->addModel(modelChannelStrip);
	p->addModel(modelPonyVCO);
	p->addModel(modelMotionMTR);
	p->addModel(modelBurst);
	p->addModel(modelVoltio);
}
