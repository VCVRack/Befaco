#include "Befaco.hpp"


Plugin *plugin;

void init(rack::Plugin *p) {
	plugin = p;
	p->slug = "Befaco";
#ifdef VERSION
	p->version = TOSTRING(VERSION);
#endif
	p->addModel(createModel<EvenVCOWidget>("Befaco", "Befaco", "EvenVCO", "EvenVCO"));
	p->addModel(createModel<RampageWidget>("Befaco", "Befaco", "Rampage", "Rampage"));
	p->addModel(createModel<ABCWidget>("Befaco", "Befaco", "ABC", "A*B+C"));
	p->addModel(createModel<SpringReverbWidget>("Befaco", "Befaco", "SpringReverb", "Spring Reverb"));
	p->addModel(createModel<MixerWidget>("Befaco", "Befaco", "Mixer", "Mixer"));
	p->addModel(createModel<SlewLimiterWidget>("Befaco", "Befaco", "SlewLimiter", "Slew Limiter"));
	p->addModel(createModel<DualAtenuverterWidget>("Befaco", "Befaco", "DualAtenuverter", "Dual Atenuverter"));

	springReverbInit();
}
