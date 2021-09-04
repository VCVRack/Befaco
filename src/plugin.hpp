#include <rack.hpp>


using namespace rack;


extern Plugin* pluginInstance;

extern Model* modelEvenVCO;
extern Model* modelRampage;
extern Model* modelABC;
extern Model* modelSpringReverb;
extern Model* modelMixer;
extern Model* modelSlewLimiter;
extern Model* modelDualAtenuverter;
extern Model* modelPercall;
extern Model* modelHexmixVCA;
extern Model* modelChoppingKinky;
extern Model* modelKickall;
extern Model* modelSamplingModulator;
extern Model* modelMorphader;
extern Model* modelADSR;
extern Model* modelSTMix;
extern Model* modelMuxlicer;
extern Model* modelMex;

struct Knurlie : SvgScrew {
	Knurlie() {
		sw->svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/Knurlie.svg"));
		sw->wrap();
		box.size = sw->box.size;
	}
};

struct BefacoTinyKnobRed : app::SvgKnob {
	BefacoTinyKnobRed() {
		minAngle = -0.8 * M_PI;
		maxAngle = 0.8 * M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BefacoTinyKnobRed.svg")));
	}
};

struct BefacoTinyKnobWhite : app::SvgKnob {
	BefacoTinyKnobWhite() {
		minAngle = -0.8 * M_PI;
		maxAngle = 0.8 * M_PI;
		setSvg(APP->window->loadSvg(asset::system("res/ComponentLibrary/BefacoTinyKnob.svg")));
	}
};

struct BefacoTinyKnobDarkGrey : app::SvgKnob {
	BefacoTinyKnobDarkGrey() {
		minAngle = -0.8 * M_PI;
		maxAngle = 0.8 * M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BefacoTinyKnobDarkGrey.svg")));
	}
};

struct BefacoTinyKnobLightGrey : app::SvgKnob {
	BefacoTinyKnobLightGrey() {
		minAngle = -0.8 * M_PI;
		maxAngle = 0.8 * M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BefacoTinyKnobLightGrey.svg")));
	}
};

struct BefacoTinyKnobBlack : app::SvgKnob {
	BefacoTinyKnobBlack() {
		minAngle = -0.8 * M_PI;
		maxAngle = 0.8 * M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BefacoTinyKnobBlack.svg")));
	}
};

struct Davies1900hLargeGreyKnob : Davies1900hKnob {
	Davies1900hLargeGreyKnob() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Davies1900hLargeGrey.svg")));
	}
};

struct Davies1900hLightGreyKnob : Davies1900hWhiteKnob {
	Davies1900hLightGreyKnob() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Davies1900hLightGrey.svg")));
	}
};

struct Davies1900hDarkGreyKnob : Davies1900hWhiteKnob {
	Davies1900hDarkGreyKnob() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Davies1900hDarkGrey.svg")));
	}
};

// library black Davies1900h doesn't work well on black backgrounds
struct Davies1900hDarkBlackAlt : Davies1900hWhiteKnob {
	Davies1900hDarkBlackAlt() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Davies1900hBlack.svg")));
	}
};

struct BefacoOutputPort : app::SvgPort {
	BefacoOutputPort() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BefacoOutputPort.svg")));
	}
};

struct BefacoInputPort : app::SvgPort {
	BefacoInputPort() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BefacoInputPort.svg")));
	}
};

struct CKSSNarrow : app::SvgSwitch {
	CKSSNarrow() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SwitchNarrow_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SwitchNarrow_1.svg")));
	}
};

struct Crossfader : app::SvgSlider {
	Crossfader() {
		setBackgroundSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/CrossfaderBackground.svg")));
		setHandleSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/CrossfaderHandle.svg")));
		minHandlePos = mm2px(Vec(4.5f, -0.8f));
		maxHandlePos = mm2px(Vec(34.5, -0.8f));
		horizontal = true;
		math::Vec margin = math::Vec(15, 5);
		background->box.pos = margin;
		box.size = background->box.size.plus(margin.mult(2));
	}
};

struct BefacoButton : app::SvgSwitch {
	BefacoButton() {
		momentary = true;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BefacoButton_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BefacoButton_1.svg")));
	}
};

struct BefacoSwitchHorizontal : app::SvgSwitch {
	BefacoSwitchHorizontal() {		
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BefacoSwitchHoriz_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BefacoSwitchHoriz_1.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BefacoSwitchHoriz_2.svg")));
	}
};

struct BefacoTinyKnobSnap : BefacoTinyKnob {
	BefacoTinyKnobSnap() {
		snap = true;
		minAngle = -0.80 * M_PI;
		maxAngle = 0.80 * M_PI;
	}
};

template <typename T>
T sin2pi_pade_05_5_4(T x) {
	x -= 0.5f;
	return (T(-6.283185307) * x + T(33.19863968) * simd::pow(x, 3) - T(32.44191367) * simd::pow(x, 5))
	       / (1 + T(1.296008659) * simd::pow(x, 2) + T(0.7028072946) * simd::pow(x, 4));
}

template <typename T>
T tanh_pade(T x) {
	T x2 = x * x;
	T q = 12.f + x2;
	return 12.f * x * q / (36.f * x2 + q * q);
}

template <typename T>
T exponentialBipolar80Pade_5_4(T x) {
	return (T(0.109568) * x + T(0.281588) * simd::pow(x, 3) + T(0.133841) * simd::pow(x, 5))
	       / (T(1.) - T(0.630374) * simd::pow(x, 2) + T(0.166271) * simd::pow(x, 4));
}

struct ADEnvelope {
	enum Stage {
		STAGE_OFF,
		STAGE_ATTACK,
		STAGE_DECAY
	};

	Stage stage = STAGE_OFF;
	float env = 0.f;
	float attackTime = 0.1, decayTime = 0.1;
	float attackShape = 1.0, decayShape = 1.0;

	ADEnvelope() { };

	void process(const float& sampleTime) {

		if (stage == STAGE_OFF) {
			env = envLinear = 0.0f;
		}
		else if (stage == STAGE_ATTACK) {
			envLinear += sampleTime / attackTime;
			env = std::pow(envLinear, attackShape);
		}
		else if (stage == STAGE_DECAY) {
			envLinear -= sampleTime / decayTime;
			env = std::pow(envLinear, decayShape);
		}

		if (envLinear >= 1.0f) {
			stage = STAGE_DECAY;
			env = envLinear = 1.0f;
		}
		else if (envLinear <= 0.0f) {
			stage = STAGE_OFF;
			env = envLinear = 0.0f;
		}
	}

	void trigger() {
		stage = ADEnvelope::STAGE_ATTACK;
		// non-linear envelopes won't retrigger at the correct starting point if
		// attackShape != decayShape, so we advance the linear envelope
		envLinear = std::pow(env, 1.0f / attackShape);
	}

private:
	float envLinear = 0.f;
};

struct MexMessage { 
	int addressIndex = 0;
	bool isPlaying = false;
	float allGates = 0.f;
	float outputClock = 0.f;
};

