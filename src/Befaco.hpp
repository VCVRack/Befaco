#include "rack.hpp"


using namespace rack;


extern Plugin *plugin;

void springReverbInit();

////////////////////
// module widgets
////////////////////

struct EvenVCOWidget : ModuleWidget {
	EvenVCOWidget();
};

struct RampageWidget : ModuleWidget {
	RampageWidget();
};

struct ABCWidget : ModuleWidget {
	ABCWidget();
};

struct SpringReverbWidget : ModuleWidget {
	SpringReverbWidget();
};

struct MixerWidget : ModuleWidget {
	MixerWidget();
};

struct SlewLimiterWidget : ModuleWidget {
	SlewLimiterWidget();
};

struct DualAtenuverterWidget : ModuleWidget {
	DualAtenuverterWidget();
};
