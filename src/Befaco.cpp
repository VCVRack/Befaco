#include "Befaco.hpp"


Plugin *plugin;

void init(rack::Plugin *p) {
	plugin = p;
	p->slug = "Befaco";
#ifdef VERSION
	p->version = TOSTRING(VERSION);
#endif

	p->addModel(createModel<EvenVCOWidget>("Befaco", "EvenVCO", "EvenVCO"));
	p->addModel(createModel<RampageWidget>("Befaco", "Rampage", "Rampage"));
	p->addModel(createModel<ABCWidget>("Befaco", "ABC", "A*B+C"));
	p->addModel(createModel<SpringReverbWidget>("Befaco", "SpringReverb", "Spring Reverb"));
	p->addModel(createModel<MixerWidget>("Befaco", "Mixer", "Mixer"));
	p->addModel(createModel<SlewLimiterWidget>("Befaco", "SlewLimiter", "Slew Limiter"));
	p->addModel(createModel<DualAtenuverterWidget>("Befaco", "DualAtenuverter", "Dual Atenuverter"));

	springReverbInit();
}
