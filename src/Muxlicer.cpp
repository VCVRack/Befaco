#include "plugin.hpp"


static const float dividerFreq = 1000.f;


struct Muxlicer : Module {
	enum ParamIds {
		PLAY_PARAM,
		ADDRESS_PARAM,
		GATE_MODE_PARAM,
		SPEED_PARAM,
		ENUMS(LEVEL_PARAMS, 8),
		NUM_PARAMS
	};
	enum InputIds {
		GATE_MODE_INPUT,
		ADDRESS_INPUT,
		CLOCK_INPUT,
		RESET_INPUT,
		COM_INPUT,
		ALL_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		CLOCK_OUTPUT,
		ALL_GATES_OUTPUT,
		EOC_OUTPUT,
		ENUMS(GATE_OUTPUTS, 8),
		ENUMS(MUX_OUTPUTS, 8),
		NUM_OUTPUTS
	};
	enum LightIds {
		CLOCK_LIGHT,
		ENUMS(GATE_LIGHTS, 8),
		NUM_LIGHTS
	};

	float dividerPhase = 0.f;
	uint32_t clockDivider;
	float clockDividerF;
	uint32_t clockTime;
	uint32_t runIndex;
	uint32_t addressIndex = 0;
	float lastSpeed = 0.f;
	bool tapped = false;
	uint32_t tapTime = 99999;
	dsp::SchmittTrigger clockTrigger;

	Muxlicer() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(Muxlicer::PLAY_PARAM, 0.0, 1.0, 0.0, "");
		configParam(Muxlicer::ADDRESS_PARAM, -1.0, 7.0, -1.0, "");
		configParam(Muxlicer::GATE_MODE_PARAM, 0.0, 1.0, 0.0, "");
		configParam(Muxlicer::SPEED_PARAM, -INFINITY, INFINITY, 0.0, "");
		configParam(Muxlicer::LEVEL_PARAMS + 0, 0.0, 1.0, 1.0, "");
		configParam(Muxlicer::LEVEL_PARAMS + 1, 0.0, 1.0, 1.0, "");
		configParam(Muxlicer::LEVEL_PARAMS + 2, 0.0, 1.0, 1.0, "");
		configParam(Muxlicer::LEVEL_PARAMS + 3, 0.0, 1.0, 1.0, "");
		configParam(Muxlicer::LEVEL_PARAMS + 4, 0.0, 1.0, 1.0, "");
		configParam(Muxlicer::LEVEL_PARAMS + 5, 0.0, 1.0, 1.0, "");
		configParam(Muxlicer::LEVEL_PARAMS + 6, 0.0, 1.0, 1.0, "");
		configParam(Muxlicer::LEVEL_PARAMS + 7, 0.0, 1.0, 1.0, "");

		onReset();
	}

	void onReset() override {
		clockDivider = 250;
		clockDividerF = clockDivider;
		clockTime = 0;
		runIndex = 0;
	}

	void process(const ProcessArgs &args) override {
		if (clockTrigger.process(rescale(inputs[CLOCK_INPUT].getVoltage(), 0.1f, 2.f, 0.f, 1.f))) {
			tapped = true;
		}

		dividerPhase += dividerFreq * args.sampleTime;
		if (dividerPhase >= 1.f) {
			dividerPhase -= 1.f;

			// Index
			float address = params[ADDRESS_PARAM].getValue() + inputs[ADDRESS_INPUT].getVoltage();
			bool running = address < 0.f;
			if (running) {
				addressIndex = runIndex;
			}
			else {
				addressIndex = clamp((int) roundf(address), 0, 8-1);
			}

			// Clock frequency and phase
			if (tapped) {
				if (tapTime < 2000) {
					clockDivider = tapTime;
					clockDividerF = clockDivider;
				}
				tapTime = 0;
				tapped = false;
			}
			tapTime++;

			float speed = params[SPEED_PARAM].getValue();
			if (speed != lastSpeed) {
				clockDividerF *= powf(0.5f, speed - lastSpeed);
				clockDividerF = clamp(clockDividerF, 1.f, 2000.f);
				clockDivider = roundf(clockDividerF);
				lastSpeed = speed;
			}
			clockTime++;

			// Clock trigger
			outputs[CLOCK_OUTPUT].setVoltage(0.f);
			outputs[EOC_OUTPUT].setVoltage(0.f);

			if (clockTime >= clockDivider) {
				clockTime = 0;
				outputs[CLOCK_OUTPUT].setVoltage(10.f);

				if (running) {
					runIndex++;
					if (runIndex >= 8) {
						runIndex = 0;
						outputs[EOC_OUTPUT].setVoltage(10.f);
					}
				}
			}

			// Gates
			lights[CLOCK_LIGHT].setBrightness(isEven(addressIndex) ? 1.f : 0.f);
			for (int i = 0; i < 8; i++) {
				outputs[GATE_OUTPUTS + i].setVoltage(0.f);
				lights[GATE_LIGHTS + i].setBrightness(0.f);
			}
			outputs[ALL_GATES_OUTPUT].setVoltage(0.f);

			bool gate = true;
			if (gate) {
				outputs[GATE_OUTPUTS + addressIndex].setVoltage(10.f);
				lights[GATE_LIGHTS + addressIndex].setBrightness(1.f);
				outputs[ALL_GATES_OUTPUT].setVoltage(10.f);
			}
		}

		// Mux outputs
		for (int i = 0; i < 8; i++) {
			outputs[MUX_OUTPUTS + i].setVoltage(0.f);
		}
		float com = inputs[COM_INPUT].getVoltage();
		float level = params[LEVEL_PARAMS + addressIndex].getValue();
		outputs[MUX_OUTPUTS + addressIndex].setVoltage(level * com);
	}
};


struct MuxlicerTapBefacoTinyKnob : BefacoTinyKnob {
/*
	bool moved;

	void onDragStart(EventDragStart &e) override {
		moved = false;
		BefacoTinyKnob::onDragStart(e);
	}
	void onDragMove(EventDragMove &e) override {
		if (!e.mouseRel.isZero())
			moved = true;
		BefacoTinyKnob::onDragMove(e);
	}
	void onDragEnd(EventDragEnd &e) override {
		if (!moved) {
			Muxlicer *module = dynamic_cast<Muxlicer*>(this->module);
			module->tapped = true;
		}
		BefacoTinyKnob::onDragEnd(e);
	}
*/
};

struct MuxlicerWidget : ModuleWidget {
	MuxlicerWidget(Muxlicer *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Muxlicer.svg")));

		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<Knurlie>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParam<BefacoSwitch>(mm2px(Vec(35.72963, 10.008)), module, Muxlicer::PLAY_PARAM));
		addParam(createParam<BefacoTinyKnob>(mm2px(Vec(3.84112, 10.90256)), module, Muxlicer::ADDRESS_PARAM));
		addParam(createParam<BefacoTinyKnob>(mm2px(Vec(67.83258, 10.86635)), module, Muxlicer::GATE_MODE_PARAM));
		addParam(createParam<MuxlicerTapBefacoTinyKnob>(mm2px(Vec(28.12238, 24.62151)), module, Muxlicer::SPEED_PARAM));
		addParam(createParam<BefacoSlidePot>(mm2px(Vec(1.82728, 40.67102)), module, Muxlicer::LEVEL_PARAMS + 0));
		addParam(createParam<BefacoSlidePot>(mm2px(Vec(11.95595, 40.67102)), module, Muxlicer::LEVEL_PARAMS + 1));
		addParam(createParam<BefacoSlidePot>(mm2px(Vec(22.08462, 40.67102)), module, Muxlicer::LEVEL_PARAMS + 2));
		addParam(createParam<BefacoSlidePot>(mm2px(Vec(32.2133, 40.67102)), module, Muxlicer::LEVEL_PARAMS + 3));
		addParam(createParam<BefacoSlidePot>(mm2px(Vec(42.34195, 40.67102)), module, Muxlicer::LEVEL_PARAMS + 4));
		addParam(createParam<BefacoSlidePot>(mm2px(Vec(52.47062, 40.67102)), module, Muxlicer::LEVEL_PARAMS + 5));
		addParam(createParam<BefacoSlidePot>(mm2px(Vec(62.5993, 40.67102)), module, Muxlicer::LEVEL_PARAMS + 6));
		addParam(createParam<BefacoSlidePot>(mm2px(Vec(72.72797, 40.67102)), module, Muxlicer::LEVEL_PARAMS + 7));

		addInput(createInput<PJ301MPort>(mm2px(Vec(51.568, 11.20189)), module, Muxlicer::GATE_MODE_INPUT));
		addInput(createInput<PJ301MPort>(mm2px(Vec(21.13974, 11.23714)), module, Muxlicer::ADDRESS_INPUT));
		addInput(createInput<PJ301MPort>(mm2px(Vec(44.24461, 24.93662)), module, Muxlicer::CLOCK_INPUT));
		addInput(createInput<PJ301MPort>(mm2px(Vec(12.62135, 24.95776)), module, Muxlicer::RESET_INPUT));
		addInput(createInput<PJ301MPort>(mm2px(Vec(36.3142, 98.07911)), module, Muxlicer::COM_INPUT));
		addInput(createInput<PJ301MPort>(mm2px(Vec(16.11766, 98.09121)), module, Muxlicer::ALL_INPUT));

		addOutput(createOutput<PJ301MPort>(mm2px(Vec(59.8492, 24.95776)), module, Muxlicer::CLOCK_OUTPUT));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(0.89595, 86.78581)), module, Muxlicer::GATE_OUTPUTS + 0));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(11.02463, 86.77068)), module, Muxlicer::GATE_OUTPUTS + 1));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(21.14758, 86.77824)), module, Muxlicer::GATE_OUTPUTS + 2));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(31.27625, 86.77824)), module, Muxlicer::GATE_OUTPUTS + 3));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(41.40493, 86.77824)), module, Muxlicer::GATE_OUTPUTS + 4));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(51.56803, 86.79938)), module, Muxlicer::GATE_OUTPUTS + 5));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(61.69671, 86.79938)), module, Muxlicer::GATE_OUTPUTS + 6));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(71.79094, 86.77824)), module, Muxlicer::GATE_OUTPUTS + 7));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(56.59663, 98.06252)), module, Muxlicer::ALL_GATES_OUTPUT));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(66.72661, 98.07008)), module, Muxlicer::EOC_OUTPUT));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(0.89595, 109.27901)), module, Muxlicer::MUX_OUTPUTS + 0));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(11.05332, 109.29256)), module, Muxlicer::MUX_OUTPUTS + 1));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(21.18201, 109.29256)), module, Muxlicer::MUX_OUTPUTS + 2));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(31.27625, 109.27142)), module, Muxlicer::MUX_OUTPUTS + 3));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(41.40493, 109.27142)), module, Muxlicer::MUX_OUTPUTS + 4));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(51.5336, 109.27142)), module, Muxlicer::MUX_OUTPUTS + 5));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(61.69671, 109.29256)), module, Muxlicer::MUX_OUTPUTS + 6));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(71.82537, 109.29256)), module, Muxlicer::MUX_OUTPUTS + 7));

		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(71.28361, 28.02644)), module, Muxlicer::CLOCK_LIGHT));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(3.99336, 81.86801)), module, Muxlicer::GATE_LIGHTS + 0));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(14.09146, 81.86801)), module, Muxlicer::GATE_LIGHTS + 1));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(24.22525, 81.86801)), module, Muxlicer::GATE_LIGHTS + 2));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(34.35901, 81.86801)), module, Muxlicer::GATE_LIGHTS + 3));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(44.49277, 81.86801)), module, Muxlicer::GATE_LIGHTS + 4));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(54.62652, 81.86801)), module, Muxlicer::GATE_LIGHTS + 5));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(64.76028, 81.86801)), module, Muxlicer::GATE_LIGHTS + 6));
		addChild(createLight<SmallLight<RedLight>>(mm2px(Vec(74.89404, 81.86801)), module, Muxlicer::GATE_LIGHTS + 7));
	}
};


Model *modelMuxlicer = createModel<Muxlicer, MuxlicerWidget>("Muxlicer");

