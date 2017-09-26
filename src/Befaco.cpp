#include "Befaco.hpp"


Plugin *plugin;

void init(rack::Plugin *p) {
	plugin = p;
	plugin->slug = "Befaco";
	plugin->name = "Befaco";
	createModel<EvenVCOWidget>(plugin, "EvenVCO", "EvenVCO");
	// createModel<RampageWidget>(plugin, "Rampage", "Rampage");
	createModel<ABCWidget>(plugin, "ABC", "A*B+C");
	createModel<SpringReverbWidget>(plugin, "SpringReverb", "Spring Reverb");
	createModel<MixerWidget>(plugin, "Mixer", "Mixer");
	createModel<SlewLimiterWidget>(plugin, "SlewLimiter", "Slew Limiter");
	createModel<DualAtenuverterWidget>(plugin, "DualAtenuverter", "Dual Atenuverter");

	springReverbInit();
}
