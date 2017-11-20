#include "Befaco.hpp"


Plugin *plugin;

void init(rack::Plugin *p) {
	plugin = p;
	p->slug = "Befaco";
#ifdef VERSION
	p->version = TOSTRING(VERSION);
#endif

	p->addModel(createModel<EvenVCOWidget>("Befaco", "EvenVCO", "EvenVCO", OSCILLATOR_TAG));
	p->addModel(createModel<RampageWidget>("Befaco", "Rampage", "Rampage", FUNCTION_GENERATOR_TAG, LOGIC_TAG, SLEW_LIMITER_TAG, ENVELOPE_FOLLOWER_TAG, DUAL_TAG));
	p->addModel(createModel<ABCWidget>("Befaco", "ABC", "A*B+C", RING_MODULATOR_TAG, ATTENUATOR_TAG, DUAL_TAG));
	p->addModel(createModel<SpringReverbWidget>("Befaco", "SpringReverb", "Spring Reverb", REVERB_TAG, DUAL_TAG));
	p->addModel(createModel<MixerWidget>("Befaco", "Mixer", "Mixer", MIXER_TAG));
	p->addModel(createModel<SlewLimiterWidget>("Befaco", "SlewLimiter", "Slew Limiter", SLEW_LIMITER_TAG, ENVELOPE_FOLLOWER_TAG));
	p->addModel(createModel<DualAtenuverterWidget>("Befaco", "DualAtenuverter", "Dual Atenuverter", ATTENUATOR_TAG, DUAL_TAG));

	springReverbInit();
}
