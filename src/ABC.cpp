#include "plugin.hpp"
#include "simd_mask.hpp"

#define MAX(a,b) (a>b)?a:b

/* 
static float clip(float x) {
	// Pade approximant of x/(1 + x^12)^(1/12)
	const float limit = 1.16691853009184;
	x = clamp(x, -limit, limit);
	return (x + 1.45833*std::pow(x, 13) + 0.559028*std::pow(x, 25) + 0.0427035*std::pow(x, 37))
		/ (1. + 1.54167*std::pow(x, 12) + 0.642361*std::pow(x, 24) + 0.0579909*std::pow(x, 36));
}
*/

static simd::float_4 clip4(simd::float_4 x) {
	// Pade approximant of x/(1 + x^12)^(1/12)
	const simd::float_4 limit = simd::float_4(1.16691853009184);
	const simd::float_4 cnst_10 = simd::float_4(10.0);
	const simd::float_4 cnst_1  = simd::float_4(1.0);
	const simd::float_4 cnst_01 = simd::float_4(0.1);

	const simd::float_4 coeff_1 = simd::float_4(1.45833);
	const simd::float_4 coeff_2 = simd::float_4(0.559028);
	const simd::float_4 coeff_3 = simd::float_4(0.0427035);
	const simd::float_4 coeff_4 = simd::float_4(1.54167);
	const simd::float_4 coeff_5 = simd::float_4(0.642361);
	const simd::float_4 coeff_6 = simd::float_4(0.0579909);

	x = clamp(x*cnst_01, -limit, limit);
	return cnst_10 * (x + coeff_1*simd::pow(x, 13) + coeff_2*simd::pow(x, 25) + coeff_3*simd::pow(x, 37))
		/ (cnst_1 + coeff_4*simd::pow(x, 12) + coeff_5*simd::pow(x, 24) + coeff_6*simd::pow(x, 36));
}


static float exponentialBipolar80Pade_5_4(float x) {
	return (0.109568*x + 0.281588*std::pow(x, 3) + 0.133841*std::pow(x, 5))
		/ (1. - 0.630374*std::pow(x, 2) + 0.166271*std::pow(x, 4));
}


struct ABC : Module {
	enum ParamIds {
		B1_LEVEL_PARAM,
		C1_LEVEL_PARAM,
		B2_LEVEL_PARAM,
		C2_LEVEL_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		A1_INPUT,
		B1_INPUT,
		C1_INPUT,
		A2_INPUT,
		B2_INPUT,
		C2_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT1_OUTPUT,
		OUT2_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(OUT1_LIGHT, 3),
		ENUMS(OUT2_LIGHT, 3),
		NUM_LIGHTS
	};

	ChannelMask channelMask;


	ABC() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(B1_LEVEL_PARAM, -1.0, 1.0, 0.0, "B1 Level");
		configParam(C1_LEVEL_PARAM, -1.0, 1.0, 0.0, "C1 Level");
		configParam(B2_LEVEL_PARAM, -1.0, 1.0, 0.0, "B2 Level");
		configParam(C2_LEVEL_PARAM, -1.0, 1.0, 0.0, "C2 Level");
	}




	void process(const ProcessArgs &args) override {

		simd::float_4 a1[4];
		simd::float_4 b1[4];
		simd::float_4 c1[4];
		simd::float_4 out1[4];

		simd::float_4 a2[4];
		simd::float_4 b2[4];
		simd::float_4 c2[4];
		simd::float_4 out2[4];

		int channels_1 = 1;
		int channels_2 = 1;



		// process upper section

		if(outputs[OUT1_OUTPUT].isConnected() || outputs[OUT2_OUTPUT].isConnected() ) {

			int channels_A1 = inputs[A1_INPUT].getChannels();
			int channels_B1 = inputs[B1_INPUT].getChannels();
			int channels_C1 = inputs[C1_INPUT].getChannels();

			channels_1 = MAX(channels_1, channels_A1);
			channels_1 = MAX(channels_1, channels_B1);
			channels_1 = MAX(channels_1, channels_C1);

			float mult_B1 = (2.f/5.f)*exponentialBipolar80Pade_5_4(params[B1_LEVEL_PARAM].getValue());
			float mult_C1 = exponentialBipolar80Pade_5_4(params[C1_LEVEL_PARAM].getValue());

			load_input(inputs[A1_INPUT], a1, channels_A1);
			channelMask.apply(a1, channels_1);

			if(inputs[B1_INPUT].isConnected()) {
				load_input(inputs[B1_INPUT], b1, channels_B1);
				for(int c=0; c<channels_1; c+=4) b1[c/4] *= simd::float_4(mult_B1);
			} else {
				for(int c=0; c<channels_1; c+=4) b1[c/4] = simd::float_4(5.f*mult_B1);
			}
			channelMask.apply(b1, channels_1);

			if(inputs[C1_INPUT].isConnected()) {
				load_input(inputs[C1_INPUT], c1, channels_C1);
					for(int c=0; c<channels_1; c+=4) c1[c/4] *= simd::float_4(mult_C1);
			} else {
				for(int c=0; c<channels_1; c+=4) c1[c/4] = simd::float_4(10.f*mult_C1);
			}
			channelMask.apply(c1, channels_1);

			for(int c=0; c<channels_1; c+=4) out1[c/4] = clip4(a1[c/4] * b1[c/4] + c1[c/4]);
		}

		// process lower section

		if(outputs[OUT2_OUTPUT].isConnected()) {

			int channels_A2 = inputs[A2_INPUT].getChannels();
			int channels_B2 = inputs[B2_INPUT].getChannels();
			int channels_C2 = inputs[C2_INPUT].getChannels();

			channels_2 = MAX(channels_2, channels_A2);
			channels_2 = MAX(channels_2, channels_B2);
			channels_2 = MAX(channels_2, channels_C2);

			float mult_B2 = (2.f/5.f)*exponentialBipolar80Pade_5_4(params[B2_LEVEL_PARAM].getValue());
			float mult_C2 = exponentialBipolar80Pade_5_4(params[C2_LEVEL_PARAM].getValue());

			load_input(inputs[A2_INPUT], a2, channels_A2);
			channelMask.apply(a2, channels_2);

			if(inputs[B2_INPUT].isConnected()) {
				load_input(inputs[B2_INPUT], b2, channels_B2);
				for(int c=0; c<channels_2; c+=4) b2[c/4] *= simd::float_4(mult_B2);
			} else {
				for(int c=0; c<channels_2; c+=4) b2[c/4] = simd::float_4(5.f*mult_B2);
			}
			channelMask.apply(b2, channels_2);

			if(inputs[C2_INPUT].isConnected()) {
				load_input(inputs[C2_INPUT], c2, channels_C2);
				for(int c=0; c<channels_2; c+=4) c2[c/4] *= simd::float_4(mult_C2);
			} else {
				for(int c=0; c<channels_2; c+=4) c2[c/4] = simd::float_4(10.f*mult_C2);
			}
			channelMask.apply(c2, channels_2);

			for(int c=0; c<channels_2; c+=4) out2[c/4] = clip4(a2[c/4] * b2[c/4] + c2[c/4]);
		};


		// Set outputs
		if (outputs[OUT1_OUTPUT].isConnected()) {
			outputs[OUT1_OUTPUT].setChannels(channels_1);
			for(int c=0; c<channels_1; c+=4) out1[c/4].store(outputs[OUT1_OUTPUT].getVoltages(c));
		}
		else {
			for(int c=0; c<channels_1; c+=4) out2[c/4] += out1[c/4];
			channels_2 = MAX(channels_1, channels_2);
		}

		if (outputs[OUT2_OUTPUT].isConnected()) {
			outputs[OUT2_OUTPUT].setChannels(channels_2);
			for(int c=0; c<channels_2; c+=4) out2[c/4].store(outputs[OUT2_OUTPUT].getVoltages(c));
		}

		// Lights

		float light_1;
		float light_2;

		if(channels_1==1) {
			light_1 = out1[0].s[0];
			lights[OUT1_LIGHT + 0].setSmoothBrightness(light_1 / 5.f, args.sampleTime);
			lights[OUT1_LIGHT + 1].setSmoothBrightness(-light_1 / 5.f, args.sampleTime);
			lights[OUT1_LIGHT + 2].setBrightness(0.f);
		} else {
			light_1 = 10.f;
			lights[OUT1_LIGHT + 0].setBrightness(0.0f);
			lights[OUT1_LIGHT + 1].setBrightness(0.0f);
			lights[OUT1_LIGHT + 2].setBrightness(light_1);
		}

		if(channels_2==1) {
			light_2 = out2[0].s[0];
			lights[OUT2_LIGHT + 0].setSmoothBrightness(light_2 / 5.f, args.sampleTime);
			lights[OUT2_LIGHT + 1].setSmoothBrightness(-light_2 / 5.f, args.sampleTime);
			lights[OUT2_LIGHT + 2].setBrightness(0.f);
		} else {
			light_2 = 10.f;
			lights[OUT2_LIGHT + 0].setBrightness(0.0f);
			lights[OUT2_LIGHT + 1].setBrightness(0.0f);
			lights[OUT2_LIGHT + 2].setBrightness(light_2);
		}	

				
	}
};


struct ABCWidget : ModuleWidget {
	ABCWidget(ABC *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ABC.svg")));

		addChild(createWidget<Knurlie>(Vec(15, 0)));
		addChild(createWidget<Knurlie>(Vec(15, 365)));

		addParam(createParam<Davies1900hRedKnob>(Vec(45, 37), module, ABC::B1_LEVEL_PARAM));
		addParam(createParam<Davies1900hWhiteKnob>(Vec(45, 107), module, ABC::C1_LEVEL_PARAM));
		addParam(createParam<Davies1900hRedKnob>(Vec(45, 204), module, ABC::B2_LEVEL_PARAM));
		addParam(createParam<Davies1900hWhiteKnob>(Vec(45, 274), module, ABC::C2_LEVEL_PARAM));

		addInput(createInput<PJ301MPort>(Vec(7, 28), module, ABC::A1_INPUT));
		addInput(createInput<PJ301MPort>(Vec(7, 70), module, ABC::B1_INPUT));
		addInput(createInput<PJ301MPort>(Vec(7, 112), module, ABC::C1_INPUT));
		addOutput(createOutput<PJ301MPort>(Vec(7, 154), module, ABC::OUT1_OUTPUT));
		addInput(createInput<PJ301MPort>(Vec(7, 195), module, ABC::A2_INPUT));
		addInput(createInput<PJ301MPort>(Vec(7, 237), module, ABC::B2_INPUT));
		addInput(createInput<PJ301MPort>(Vec(7, 279), module, ABC::C2_INPUT));
		addOutput(createOutput<PJ301MPort>(Vec(7, 321), module, ABC::OUT2_OUTPUT));

		addChild(createLight<MediumLight<RedGreenBlueLight>>(Vec(37, 162), module, ABC::OUT1_LIGHT));
		addChild(createLight<MediumLight<RedGreenBlueLight>>(Vec(37, 329), module, ABC::OUT2_LIGHT));
	}
};


Model *modelABC = createModel<ABC, ABCWidget>("ABC");
