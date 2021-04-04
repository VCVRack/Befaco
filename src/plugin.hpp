#include "rack.hpp"


using namespace rack;


extern Plugin *pluginInstance;

extern Model *modelEvenVCO;
extern Model *modelRampage;
extern Model *modelABC;
extern Model *modelSpringReverb;
extern Model *modelMixer;
extern Model *modelSlewLimiter;
extern Model *modelDualAtenuverter;
extern Model *modelPercall;
extern Model *modelHexmixVCA;

struct Knurlie : SvgScrew {
	Knurlie() {
		sw->svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/Knurlie.svg"));
		sw->wrap();
		box.size = sw->box.size;
	}
};
