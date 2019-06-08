#include "plugin.hpp"

#define MAX(a,b) (a>b)?a:b
#define MIN(a,b) (a<b)?a:b


struct Mixer : Module {
	enum ParamIds {
		CH1_PARAM,
		CH2_PARAM,
		CH3_PARAM,
		CH4_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		IN1_INPUT,
		IN2_INPUT,
		IN3_INPUT,
		IN4_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT1_OUTPUT,
		OUT2_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		OUT_POS_LIGHT,
		OUT_NEG_LIGHT,
		NUM_LIGHTS
	};

	simd::float_4 mask[4];
	simd::float_4 minus_one;

	Mixer() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(CH1_PARAM, 0.0, 1.0, 0.0, "Ch 1 level", "%", 0, 100);
		configParam(CH2_PARAM, 0.0, 1.0, 0.0, "Ch 2 level", "%", 0, 100);
		configParam(CH3_PARAM, 0.0, 1.0, 0.0, "Ch 3 level", "%", 0, 100);
		configParam(CH4_PARAM, 0.0, 1.0, 0.0, "Ch 4 level", "%", 0, 100);

		minus_one = simd::float_4(-1.0f);

		__m128i tmp =  _mm_cmpeq_epi16(_mm_set_epi32(0,0,0,0),_mm_set_epi32(0,0,0,0));

		for(int i=0; i<4; i++) {
			mask[3-i] = simd::float_4(_mm_castsi128_ps(tmp));
			tmp = _mm_srli_si128(tmp, 4);
		}

	}


	void process(const ProcessArgs &args) override {

		int i;

		int channels1 = inputs[IN1_INPUT].getChannels();
		int channels2 = inputs[IN2_INPUT].getChannels();
		int channels3 = inputs[IN3_INPUT].getChannels();
		int channels4 = inputs[IN4_INPUT].getChannels();

		int out_channels = 1;
		out_channels = MAX(out_channels, channels1);
		out_channels = MAX(out_channels, channels2);
		out_channels = MAX(out_channels, channels3);
		out_channels = MAX(out_channels, channels4);
		
		simd::float_4 mult1 = simd::float_4(params[CH1_PARAM].getValue());
		simd::float_4 mult2 = simd::float_4(params[CH2_PARAM].getValue());
		simd::float_4 mult3 = simd::float_4(params[CH3_PARAM].getValue());
		simd::float_4 mult4 = simd::float_4(params[CH4_PARAM].getValue());

		simd::float_4 in1[4]; 
		simd::float_4 in2[4]; 
		simd::float_4 in3[4]; 
		simd::float_4 in4[4]; 

		simd::float_4 out[4]; 

		out[0] = simd::float_4(0.f);
		out[1] = simd::float_4(0.f);
		out[2] = simd::float_4(0.f);
		out[3] = simd::float_4(0.f);


		if(inputs[IN1_INPUT].isConnected()) {
			// this also loads some spurious channels into in1[]
			for(int c=0; c<channels1; c+=4) in1[c/4] = simd::float_4::load(inputs[IN1_INPUT].getVoltages(c)) * mult1;

			for(i=0; i<channels1/4; i++) out[i] += in1[i];                          // add only "real" channels. 
			out[i] += simd::float_4(_mm_and_ps(in1[i].v, mask[channels1-4*i].v));   // make sure we zero out spurious channels
		}
	
		if(inputs[IN2_INPUT].isConnected()) {
			for(int c=0; c<channels2; c+=4) in2[c/4] = simd::float_4::load(inputs[IN2_INPUT].getVoltages(c)) * mult2;

			for(i=0; i<channels2/4; i++) out[i] += in2[i];
			out[i] += simd::float_4(_mm_and_ps(in2[i].v, mask[channels2-4*i].v));
		}

		if(inputs[IN3_INPUT].isConnected()) {
			for(int c=0; c<channels3; c+=4) in3[c/4] = simd::float_4::load(inputs[IN3_INPUT].getVoltages(c)) * mult3;

			for(i=0; i<channels3/4; i++) out[i] += in3[i];
			out[i] += simd::float_4(_mm_and_ps(in3[i].v, mask[channels3-4*i].v));
		}

		if(inputs[IN4_INPUT].isConnected()) {
			for(int c=0; c<channels4; c+=4) in4[c/4] = simd::float_4::load(inputs[IN4_INPUT].getVoltages(c)) * mult4;

			for(i=0; i<channels4/4; i++) out[i] += in4[i];
			out[i] += simd::float_4(_mm_and_ps(in4[i].v, mask[channels4-4*i].v));
		}


		outputs[OUT1_OUTPUT].setChannels(out_channels);
		outputs[OUT2_OUTPUT].setChannels(out_channels);

		for(int c=0; c<out_channels; c+=4) {
			out[c / 4].store(outputs[OUT1_OUTPUT].getVoltages(c));
			out[c / 4] *= minus_one;
			out[c / 4].store(outputs[OUT2_OUTPUT].getVoltages(c));
		}
		/*
		lights[OUT_POS_LIGHT].setSmoothBrightness(out / 5.f, args.sampleTime);
		lights[OUT_NEG_LIGHT].setSmoothBrightness(-out / 5.f, args.sampleTime);
		*/
	}
};




struct MixerWidget : ModuleWidget {
	MixerWidget(Mixer *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Mixer.svg")));

		addChild(createWidget<Knurlie>(Vec(15, 0)));
		addChild(createWidget<Knurlie>(Vec(15, 365)));

		addParam(createParam<Davies1900hWhiteKnob>(Vec(19, 32), module, Mixer::CH1_PARAM));
		addParam(createParam<Davies1900hWhiteKnob>(Vec(19, 85), module, Mixer::CH2_PARAM));
		addParam(createParam<Davies1900hWhiteKnob>(Vec(19, 137), module, Mixer::CH3_PARAM));
		addParam(createParam<Davies1900hWhiteKnob>(Vec(19, 190), module, Mixer::CH4_PARAM));

		addInput(createInput<PJ301MPort>(Vec(7, 242), module, Mixer::IN1_INPUT));
		addInput(createInput<PJ301MPort>(Vec(43, 242), module, Mixer::IN2_INPUT));

		addInput(createInput<PJ301MPort>(Vec(7, 281), module, Mixer::IN3_INPUT));
		addInput(createInput<PJ301MPort>(Vec(43, 281), module, Mixer::IN4_INPUT));

		addOutput(createOutput<PJ301MPort>(Vec(7, 324), module, Mixer::OUT1_OUTPUT));
		addOutput(createOutput<PJ301MPort>(Vec(43, 324), module, Mixer::OUT2_OUTPUT));

		addChild(createLight<MediumLight<GreenRedLight>>(Vec(32.7, 310), module, Mixer::OUT_POS_LIGHT));
	}
};


Model *modelMixer = createModel<Mixer, MixerWidget>("Mixer");
