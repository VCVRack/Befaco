#include "Befaco.hpp"


struct BefacoPlugin : Plugin {
	BefacoPlugin() {
		slug = "Befaco";
		name = "Befaco";
		createModel<EvenVCOWidget>(this, "EvenVCO", "EvenVCO");
		createModel<RampageWidget>(this, "Rampage", "Rampage");
		createModel<ABCWidget>(this, "ABC", "A*B+C");
		createModel<SpringReverbWidget>(this, "SpringReverb", "Spring Reverb");
		createModel<MixerWidget>(this, "Mixer", "Mixer");
		createModel<SlewLimiterWidget>(this, "SlewLimiter", "Slew Limiter");
		createModel<DualAtenuverterWidget>(this, "DualAtenuverter", "Dual Atenuverter");
	}
};


Plugin *init() {
	springReverbInit();
	return new BefacoPlugin();
}
