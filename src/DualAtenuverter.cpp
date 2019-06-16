#include "plugin.hpp"
#include "simd_mask.hpp"


struct DualAtenuverter : Module {
	enum ParamIds {
		ATEN1_PARAM,
		OFFSET1_PARAM,
		ATEN2_PARAM,
		OFFSET2_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		IN1_INPUT,
		IN2_INPUT,
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

	DualAtenuverter() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(ATEN1_PARAM, -1.0, 1.0, 0.0, "Ch 1 gain");
		configParam(OFFSET1_PARAM, -10.0, 10.0, 0.0, "Ch 1 offset", " V");
		configParam(ATEN2_PARAM, -1.0, 1.0, 0.0, "Ch 2 gain");
		configParam(OFFSET2_PARAM, -10.0, 10.0, 0.0, "Ch 2 offset", " V");
	}

	void process(const ProcessArgs &args) override {

		simd::float_4 out1[4];
		simd::float_4 out2[4];

		int channels1 = inputs[IN1_INPUT].getChannels(); channels1 = channels1>0?channels1:1;
		int channels2 = inputs[IN2_INPUT].getChannels(); channels2 = channels2>0?channels2:1;

		simd::float_4 att1 = simd::float_4(params[ATEN1_PARAM].getValue());
		simd::float_4 att2 = simd::float_4(params[ATEN2_PARAM].getValue());

		simd::float_4 offset1 = simd::float_4(params[OFFSET1_PARAM].getValue());
		simd::float_4 offset2 = simd::float_4(params[OFFSET2_PARAM].getValue());

		for (int c = 0; c < channels1; c += 4) out1[c / 4] = clamp(simd::float_4::load(inputs[IN1_INPUT].getVoltages(c)) * att1 + offset1, -10.f, 10.f);
		for (int c = 0; c < channels2; c += 4) out2[c / 4] = clamp(simd::float_4::load(inputs[IN2_INPUT].getVoltages(c)) * att2 + offset2, -10.f, 10.f);
		
		outputs[OUT1_OUTPUT].setChannels(channels1);
		outputs[OUT2_OUTPUT].setChannels(channels2);
	
		for (int c = 0; c < channels1; c += 4)  out1[c / 4].store(outputs[OUT1_OUTPUT].getVoltages(c));
		for (int c = 0; c < channels2; c += 4)  out2[c / 4].store(outputs[OUT2_OUTPUT].getVoltages(c));
		
		float light1 = outputs[OUT1_OUTPUT].getVoltageSum()/channels1;
		float light2 = outputs[OUT2_OUTPUT].getVoltageSum()/channels2;

		if(channels1==1) {
			lights[OUT1_LIGHT  ].setSmoothBrightness(light1 / 5.f, args.sampleTime);
			lights[OUT1_LIGHT+1].setSmoothBrightness(-light1 / 5.f, args.sampleTime);
			lights[OUT1_LIGHT+2].setBrightness(0.0f);
		} else {
			lights[OUT1_LIGHT  ].setBrightness(0.0f);
			lights[OUT1_LIGHT+1].setBrightness(0.0f);
			lights[OUT1_LIGHT+2].setBrightness(10.0f);
		}

		if(channels2==1) {
			lights[OUT2_LIGHT  ].setSmoothBrightness(light2 / 5.f, args.sampleTime);
			lights[OUT2_LIGHT+1].setSmoothBrightness(-light2 / 5.f, args.sampleTime);
			lights[OUT2_LIGHT+2].setBrightness(0.0f);
		} else {
			lights[OUT2_LIGHT  ].setBrightness(0.0f);
			lights[OUT2_LIGHT+1].setBrightness(0.0f);
			lights[OUT2_LIGHT+2].setBrightness(10.0f);
		}



	}
};


struct DualAtenuverterWidget : ModuleWidget {
	DualAtenuverterWidget(DualAtenuverter *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/DualAtenuverter.svg")));

		addChild(createWidget<Knurlie>(Vec(15, 0)));
		addChild(createWidget<Knurlie>(Vec(15, 365)));

		addParam(createParam<Davies1900hWhiteKnob>(Vec(20, 33), module, DualAtenuverter::ATEN1_PARAM));
		addParam(createParam<Davies1900hRedKnob>(Vec(20, 91), module, DualAtenuverter::OFFSET1_PARAM));
		addParam(createParam<Davies1900hWhiteKnob>(Vec(20, 201), module, DualAtenuverter::ATEN2_PARAM));
		addParam(createParam<Davies1900hRedKnob>(Vec(20, 260), module, DualAtenuverter::OFFSET2_PARAM));

		addInput(createInput<PJ301MPort>(Vec(7, 152), module, DualAtenuverter::IN1_INPUT));
		addOutput(createOutput<PJ301MPort>(Vec(43, 152), module, DualAtenuverter::OUT1_OUTPUT));

		addInput(createInput<PJ301MPort>(Vec(7, 319), module, DualAtenuverter::IN2_INPUT));
		addOutput(createOutput<PJ301MPort>(Vec(43, 319), module, DualAtenuverter::OUT2_OUTPUT));

		addChild(createLight<MediumLight<RedGreenBlueLight>>(Vec(33, 143), module, DualAtenuverter::OUT1_LIGHT));
		addChild(createLight<MediumLight<RedGreenBlueLight>>(Vec(33, 311), module, DualAtenuverter::OUT2_LIGHT));
	}
};


Model *modelDualAtenuverter = createModel<DualAtenuverter, DualAtenuverterWidget>("DualAtenuverter");
