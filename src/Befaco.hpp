#include "rack.hpp"
#include "component.hpp"


using namespace rack;


extern Plugin *pluginInstance;

extern Model *modelEvenVCO;
extern Model *modelRampage;
extern Model *modelABC;
extern Model *modelSpringReverb;
extern Model *modelMixer;
extern Model *modelSlewLimiter;
extern Model *modelDualAtenuverter;


struct Knurlie : SVGScrew {
	Knurlie() {
		sw->svg = SVG::load(asset::plugin(pluginInstance, "res/Knurlie.svg"));
		sw->wrap();
		box.size = sw->box.size;
	}
};
