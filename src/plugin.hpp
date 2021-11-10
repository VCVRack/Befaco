#pragma once
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
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/Knurlie.svg")));
	}
};

struct BefacoTinyKnobWhite : BefacoTinyKnob {
	BefacoTinyKnobWhite() {}
};

struct BefacoTinyKnobRed : BefacoTinyKnob {
	BefacoTinyKnobRed() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/BefacoTinyPointWhite.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/BefacoTinyKnobRed_bg.svg")));
	}
};

struct BefacoTinyKnobDarkGrey : BefacoTinyKnob {
	BefacoTinyKnobDarkGrey() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/BefacoTinyPointWhite.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/BefacoTinyKnobDarkGrey_bg.svg")));
	}
};

struct BefacoTinyKnobLightGrey : BefacoTinyKnob {
	BefacoTinyKnobLightGrey() {
		bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/BefacoTinyKnobLightGrey_bg.svg")));
	}
};

struct BefacoTinyKnobBlack : BefacoTinyKnob {
	BefacoTinyKnobBlack() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/BefacoTinyPointWhite.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/BefacoTinyKnobBlack_bg.svg")));
	}
};

struct Davies1900hLargeGreyKnob : Davies1900hKnob {
	Davies1900hLargeGreyKnob() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/Davies1900hLargeGrey.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/Davies1900hLargeGrey_bg.svg")));
	}
};

struct Davies1900hLightGreyKnob : Davies1900hKnob {
	Davies1900hLightGreyKnob() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/Davies1900hLightGrey.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/Davies1900hLightGrey_bg.svg")));
	}
};

struct Davies1900hDarkGreyKnob : Davies1900hKnob {
	Davies1900hDarkGreyKnob() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/Davies1900hDarkGrey.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/Davies1900hDarkGrey_bg.svg")));
	}
};

/** Deprecated alias */
using Davies1900hDarkBlackAlt = Davies1900hBlackKnob;

struct BananutRed : app::SvgPort {
	BananutRed() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/BananutRed.svg")));
	}
};
/** Deprecated alias */
using BefacoOutputPort = BananutRed;

struct BananutBlack : app::SvgPort {
	BananutBlack() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/BananutBlack.svg")));
	}
};
/** Deprecated alias */
using BefacoInputPort = BananutBlack;

struct CKSSNarrow : app::SvgSwitch {
	CKSSNarrow() {
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/components/SwitchNarrow_0.svg")));
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/components/SwitchNarrow_1.svg")));
	}
};

struct Crossfader : app::SvgSlider {
	Crossfader() {
		setBackgroundSvg(Svg::load(asset::plugin(pluginInstance, "res/components/CrossfaderBackground.svg")));
		setHandleSvg(Svg::load(asset::plugin(pluginInstance, "res/components/CrossfaderHandle.svg")));
		minHandlePos = mm2px(Vec(4.5f, -0.8f));
		maxHandlePos = mm2px(Vec(34.5, -0.8f));
		horizontal = true;
		math::Vec margin = math::Vec(15, 5);
		background->box.pos = margin;
		box.size = background->box.size.plus(margin.mult(2));
	}
};

struct BefacoSwitchHorizontal : app::SvgSwitch {
	BefacoSwitchHorizontal() {
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/components/BefacoSwitchHoriz_0.svg")));
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/components/BefacoSwitchHoriz_1.svg")));
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/components/BefacoSwitchHoriz_2.svg")));
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