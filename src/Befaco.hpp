#include "rack.hpp"
#include "componentlibrary.hpp"


using namespace rack;


extern Plugin *plugin;

extern Model *modelEvenVCO;
extern Model *modelRampage;
extern Model *modelABC;
extern Model *modelSpringReverb;
extern Model *modelMixer;
extern Model *modelSlewLimiter;
extern Model *modelDualAtenuverter;


struct Knurlie : SVGScrew {
	Knurlie() {
		sw->svg = SVG::load(asset::plugin(plugin, "res/Knurlie.svg"));
		sw->wrap();
		box.size = sw->box.size;
	}
};
