#include "plugin.hpp"

using simd::float_4;

// equal sum crossfade, -1 <= p <= 1
template <typename T>
inline T equalSumCrossfade(T a, T b, const float p) {
	return a * (0.5f * (1.f - p)) + b * (0.5f * (1.f + p));
}

// equal power crossfade, -1 <= p <= 1
template <typename T>
inline T equalPowerCrossfade(T a, T b, const float p) {
	//return std::min(std::exp(4.f * p), 1.f) * b + std::min(std::exp(4.f * -p), 1.f) * a;
	return std::min(exponentialBipolar80Pade_5_4(p + 1), 1.f) * b + std::min(exponentialBipolar80Pade_5_4(1 - p), 1.f) * a;
}

// TExponentialSlewLimiter doesn't appear to work as is required for this application.
// I think it is due to the absence of the logic that stops the output rising / falling too quickly,
// i.e. faster than the original signal? For now, we use this implementation (essentialy the same as
// SlewLimiter.cpp)
struct ExpLogSlewLimiter {

	float out = 0.f;
	float slew = 0.f;

	void reset() {
		out = 0.f;
	}

	void setSlew(float slew) {
		this->slew = slew;
	}
	float process(float deltaTime, float in) {
		if (in > out) {
			out += slew * (in - out) * deltaTime;
			if (out > in) {
				out = in;
			}
		}
		else if (in < out) {
			out += slew * (in - out) * deltaTime;
			if (out < in) {
				out = in;
			}
		}
		return out;
	}
};


struct Morphader : Module {
	enum ParamIds {
		CV_PARAM,
		ENUMS(A_LEVEL, 4),
		ENUMS(B_LEVEL, 4),
		ENUMS(MODE, 4),
		FADER_LAG_PARAM,
		FADER_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(CV_INPUT, 4),
		ENUMS(A_INPUT, 4),
		ENUMS(B_INPUT, 4),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(OUT, 4),
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(A_LED, 4),
		ENUMS(B_LED, 4),
		NUM_LIGHTS
	};
	enum CrossfadeMode {
		AUDIO_MODE,
		CV_MODE
	};

	static const int NUM_MIXER_CHANNELS = 4;
	const float_4 normal10VSimd = {10.f};
	ExpLogSlewLimiter slewLimiter;

	// minimum and maximum slopes in volts per second, they specify the time to get
	// from A (-1) to B (+1)
	constexpr static float slewMin = 2.0 / 15.f;
	constexpr static float slewMax = 2.0 / 0.01f;

	Morphader() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configParam(CV_PARAM, 0.f, 1.f, 1.f, "CV attenuator");

		for (int i = 0; i < NUM_MIXER_CHANNELS; i++) {
			configParam(A_LEVEL + i, 0.f, 1.f, 0.f, string::f("A level %d", i + 1));
			configInput(A_INPUT + i, string::f("A%d", i + 1));
		}
		for (int i = 0; i < NUM_MIXER_CHANNELS; i++) {
			configParam(B_LEVEL + i, 0.f, 1.f, 0.f, string::f("B level %d", i + 1));
			configInput(B_INPUT + i, string::f("B%d", i + 1));
		}
		for (int i = 0; i < NUM_MIXER_CHANNELS; i++) {
			configSwitch(MODE + i, AUDIO_MODE, CV_MODE, AUDIO_MODE, string::f("Mode %d", i + 1), {"Audio", "CV"});
			configInput(CV_INPUT + i, string::f("CV channel %d", i + 1));
		}

		configParam(FADER_LAG_PARAM, 2.0f / slewMax, 2.0f / slewMin, 2.0f / slewMax, "Fader lag", "s");
		configParam(FADER_PARAM, -1.f, 1.f, 0.f, "Fader");
	}

	// determine the cross-fade between -1 (A) and +1 (B) for each of the 4 channels
	float_4 determineChannelCrossfades(const float deltaTime) {

		float_4 channelCrossfades = {};
		const float slewLambda = 2.0f / params[FADER_LAG_PARAM].getValue();
		slewLimiter.setSlew(slewLambda);
		const float masterCrossfadeValue = slewLimiter.process(deltaTime, params[FADER_PARAM].getValue());

		for (int i = 0; i < NUM_MIXER_CHANNELS; i++) {

			if (i == 0) {
				// CV will be added to master for channel 1, and if not connected, the normalled value of 5.0V will correspond to the midpoint
				const float crossfadeCV = clamp(inputs[CV_INPUT + i].getVoltage(), 0.f, 10.f);
				channelCrossfades[i] = params[CV_PARAM].getValue() * rescale(crossfadeCV, 0.f, 10.f, 0.f, +2.f) + masterCrossfadeValue;
			}
			else {
				// if present for the current channel, CV has total control (crossfader is ignored)
				if (inputs[CV_INPUT + i].isConnected()) {
					const float crossfadeCV = clamp(inputs[CV_INPUT + i].getVoltage(), 0.f, 10.f);
					channelCrossfades[i] = rescale(crossfadeCV, 0.f, 10.f, -1.f, +1.f);
				}
				// if channel 1 is plugged in, but this channel isn't, channel 1 is normalled - in
				// this scenario, however the CV is summed with the crossfader
				else if (inputs[CV_INPUT + 0].isConnected()) {
					const float crossfadeCV = clamp(inputs[CV_INPUT + 0].getVoltage(), 0.f, 10.f);
					channelCrossfades[i] = params[CV_PARAM].getValue() * rescale(crossfadeCV, 0.f, 10.f, 0.f, +2.f) + masterCrossfadeValue;
				}
				else {
					channelCrossfades[i] = masterCrossfadeValue;
				}
			}

			channelCrossfades[i] = clamp(channelCrossfades[i], -1.f, +1.f);
		}

		return channelCrossfades;
	}

	void process(const ProcessArgs& args) override {

		int maxChannels = 1;
		float_4 mix[4] = {};
		const float_4 channelCrossfades = determineChannelCrossfades(args.sampleTime);

		for (int i = 0; i < NUM_MIXER_CHANNELS; i++) {

			const int channels = std::max(std::max(inputs[A_INPUT + i].getChannels(), inputs[B_INPUT + i].getChannels()), 1);
			// keep track of the max number of channels for the mix output, noting that if channels are taken out of the mix
			// (i.e. they're connected) they shouldn't contribute to the mix polyphony calculation
			if (!outputs[OUT + i].isConnected()) {
				maxChannels = std::max(maxChannels, channels);
			}

			float_4 out[4] = {};
			for (int c = 0; c < channels; c += 4) {
				float_4 inA = inputs[A_INPUT + i].getNormalVoltageSimd(normal10VSimd, c) * params[A_LEVEL + i].getValue();
				float_4 inB = inputs[B_INPUT + i].getNormalVoltageSimd(normal10VSimd, c) * params[B_LEVEL + i].getValue();

				switch (static_cast<CrossfadeMode>(params[MODE + i].getValue())) {
					case CV_MODE: {
						out[c / 4] = equalSumCrossfade(inA, inB, channelCrossfades[i]);
						break;
					}
					case AUDIO_MODE: {
						// in audio mode, close to the centre point it is possible to get large voltages
						// (e.g. if A and B are both 10V const). however according to the standard, it is
						// better not to clip this https://vcvrack.com/manual/VoltageStandards#Output-Saturation
						out[c / 4] = equalPowerCrossfade(inA, inB, channelCrossfades[i]);
						break;
					}
					default: {
						out[c / 4] = 0.f;
					}
				}
			}

			// if output is patched, the channel is taken out of the mix
			if (outputs[OUT + i].isConnected() && i != NUM_MIXER_CHANNELS - 1) {
				outputs[OUT + i].setChannels(channels);
				for (int c = 0; c < channels; c += 4) {
					outputs[OUT + i].setVoltageSimd(out[c / 4], c);
				}
			}
			else {
				for (int c = 0; c < channels; c += 4) {
					mix[c / 4] += out[c / 4];
				}
			}

			if (i == NUM_MIXER_CHANNELS - 1) {
				outputs[OUT + i].setChannels(maxChannels);

				for (int c = 0; c < maxChannels; c += 4) {
					outputs[OUT + i].setVoltageSimd(mix[c / 4], c);
				}
			}

			switch (static_cast<CrossfadeMode>(params[MODE + i].getValue())) {
				case AUDIO_MODE: {
					lights[A_LED + i].setBrightness(equalPowerCrossfade(1.f, 0.f, channelCrossfades[i]));
					lights[B_LED + i].setBrightness(equalPowerCrossfade(0.f, 1.f, channelCrossfades[i]));
					break;
				}
				case CV_MODE: {
					lights[A_LED + i].setBrightness(equalSumCrossfade(1.f, 0.f, channelCrossfades[i]));
					lights[B_LED + i].setBrightness(equalSumCrossfade(0.f, 1.f, channelCrossfades[i]));
					break;
				}
				default: {
					lights[A_LED + i].setBrightness(0.f);
					lights[B_LED + i].setBrightness(0.f);
					break;
				}
			}
		} // end loop over mixer channels
	}
};


struct MorphaderWidget : ModuleWidget {
	MorphaderWidget(Morphader* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/Morphader.svg")));

		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<Knurlie>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(10.817, 15.075)), module, Morphader::CV_PARAM));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(30.243, 30.537)), module, Morphader::A_LEVEL + 0));
		addParam(createParamCentered<BefacoTinyKnobLightGrey>(mm2px(Vec(30.243, 48.017)), module, Morphader::A_LEVEL + 1));
		addParam(createParamCentered<BefacoTinyKnobDarkGrey>(mm2px(Vec(30.243, 65.523)), module, Morphader::A_LEVEL + 2));
		addParam(createParamCentered<BefacoTinyKnobBlack>(mm2px(Vec(30.243, 83.051)), module, Morphader::A_LEVEL + 3));
		addParam(createParamCentered<BefacoTinyKnob>(mm2px(Vec(52.696, 30.537)), module, Morphader::B_LEVEL + 0));
		addParam(createParamCentered<BefacoTinyKnobLightGrey>(mm2px(Vec(52.696, 48.017)), module, Morphader::B_LEVEL + 1));
		addParam(createParamCentered<BefacoTinyKnobDarkGrey>(mm2px(Vec(52.696, 65.523)), module, Morphader::B_LEVEL + 2));
		addParam(createParamCentered<BefacoTinyKnobBlack>(mm2px(Vec(52.696, 83.051)), module, Morphader::B_LEVEL + 3));
		addParam(createParam<CKSSNarrow>(mm2px(Vec(39.775, 28.107)), module, Morphader::MODE + 0));
		addParam(createParam<CKSSNarrow>(mm2px(Vec(39.775, 45.627)), module, Morphader::MODE + 1));
		addParam(createParam<CKSSNarrow>(mm2px(Vec(39.775, 63.146)), module, Morphader::MODE + 2));
		addParam(createParam<CKSSNarrow>(mm2px(Vec(39.775, 80.666)), module, Morphader::MODE + 3));
		addParam(createParamCentered<BefacoTinyKnobRed>(mm2px(Vec(10.817, 99.242)), module, Morphader::FADER_LAG_PARAM));
		addParam(createParamCentered<Crossfader>(mm2px(Vec(30., 114.25)), module, Morphader::FADER_PARAM));

		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(25.214, 14.746)), module, Morphader::CV_INPUT + 0));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(35.213, 14.746)), module, Morphader::CV_INPUT + 1));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(45.236, 14.746)), module, Morphader::CV_INPUT + 2));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(55.212, 14.746)), module, Morphader::CV_INPUT + 3));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(5.812, 32.497)), module, Morphader::A_INPUT + 0));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(5.812, 48.017)), module, Morphader::A_INPUT + 1));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(5.812, 65.523)), module, Morphader::A_INPUT + 2));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(5.812, 81.185)), module, Morphader::A_INPUT + 3));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(15.791, 32.497)), module, Morphader::B_INPUT + 0));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(15.791, 48.017)), module, Morphader::B_INPUT + 1));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(15.791, 65.523)), module, Morphader::B_INPUT + 2));
		addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(15.791, 81.185)), module, Morphader::B_INPUT + 3));

		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(25.177, 100.5)), module, Morphader::OUT + 0));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(35.177, 100.5)), module, Morphader::OUT + 1));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(45.177, 100.5)), module, Morphader::OUT + 2));
		addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(55.176, 100.5)), module, Morphader::OUT + 3));

		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(37.594, 24.378)), module, Morphader::A_LED + 0));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(37.594, 41.908)), module, Morphader::A_LED + 1));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(37.594, 59.488)), module, Morphader::A_LED + 2));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(37.594, 76.918)), module, Morphader::A_LED + 3));

		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(45.332, 24.378)), module, Morphader::B_LED + 0));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(45.332, 41.908)), module, Morphader::B_LED + 1));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(45.332, 59.488)), module, Morphader::B_LED + 2));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(45.332, 76.918)), module, Morphader::B_LED + 3));
	}
};


Model* modelMorphader = createModel<Morphader, MorphaderWidget>("Morphader");