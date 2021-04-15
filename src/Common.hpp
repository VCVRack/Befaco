#pragma once

#include <rack.hpp>
#include <dsp/common.hpp>

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

struct BefacoTinyKnobGrey : app::SvgKnob {
	BefacoTinyKnobGrey() {
		minAngle = -0.8 * M_PI;
		maxAngle = 0.8 * M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BefacoTinyKnobGrey.svg")));
	}
};

struct Davies1900hLargeGreyKnob : Davies1900hKnob {
	Davies1900hLargeGreyKnob() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Davies1900hLargeGrey.svg")));
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