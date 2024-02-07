#include "plugin.hpp"


/*! \brief Decode System Exclusive messages.
 SysEx messages are encoded to guarantee transmission of data bytes higher than
 127 without breaking the MIDI protocol. Use this static method to reassemble
 your received message.
 \param inSysEx The SysEx data received from MIDI in.
 \param outData The output buffer where to store the decrypted message.
 \param inLength The length of the input buffer.
 \param inFlipHeaderBits True for Korg and other who store MSB in reverse order
 \return The length of the output buffer.
 @see encodeSysEx @see getSysExArrayLength
 Code inspired from Ruin & Wesen's SysEx encoder/decoder - http://ruinwesen.com
 */
unsigned decodeSysEx(const uint8_t* inSysEx,
                     uint8_t* outData,
                     unsigned inLength,
                     bool inFlipHeaderBits) {
	unsigned count  = 0;
	uint8_t msbStorage = 0;
	uint8_t byteIndex  = 0;

	for (unsigned i = 0; i < inLength; ++i) {
		if ((i % 8) == 0) {
			msbStorage = inSysEx[i];
			byteIndex  = 6;
		}
		else {
			const uint8_t body     = inSysEx[i];
			const uint8_t shift    = inFlipHeaderBits ? 6 - byteIndex : byteIndex;
			const uint8_t msb      = uint8_t(((msbStorage >> shift) & 1) << 7);
			byteIndex--;
			outData[count++] = msb | body;
		}
	}
	return count;
}




struct MidiThing : Module {
	enum ParamId {
		REFRESH_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		A1_INPUT,
		B1_INPUT,
		C1_INPUT,
		A2_INPUT,
		B2_INPUT,
		C2_INPUT,
		A3_INPUT,
		B3_INPUT,
		C3_INPUT,
		A4_INPUT,
		B4_INPUT,
		C4_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};
	/// Port mode
	enum PORTMODE_t : uint8_t {
		NOPORTMODE = 0,
		MODE10V,
		MODEPN5V,
		MODENEG10V,
		MODE8V,
		MODE5V,

		LASTPORTMODE
	};

	const char* cfgPortModeNames[7] = {
		"No Mode",
		"0/10v",
		"-5/5v",
		"-10/0v",
		"0/8v",
		"0/5v",
		""
	};

	// use Pre-def 4 for bridge mode
	const static int VCV_BRIDGE_PREDEF = 4;

	midi::Output midiOut;

	MidiThing() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configButton(REFRESH_PARAM, "");

		for (int i = 0; i < NUM_INPUTS; ++i) {
			portModes[i] = MODE10V;
			configInput(A1_INPUT + i, string::f("Port %d", i + 1));
		}
	}

	void onReset() override {
		midiOut.reset();

	}

	void refreshConfig() {
		for (int row = 0; row < 4; ++row) {
			for (int col = 0; col < 3; ++col) {
				requestParamOverSysex(row, col, 2);
			}
		}
	}

	// request that MidiThing loads a pre-defined template, 1-4
	void setPredef(uint8_t predef) {
		predef = clamp(predef, 1, 4);
		midi::Message msg;
		msg.bytes.resize(8);
		// Midi spec is zeroo indexed
		uint8_t predefToSend = predef - 1;
		msg.bytes = {0xF0, 0x7D, 0x17, 0x00, 0x00, 0x02, 0x00, predefToSend, 0xF7};
		midiOut.setChannel(0);
		midiOut.sendMessage(msg);
		// DEBUG("Predef %d msg request sent: %s", predef, msg.toString().c_str());
	}

	uint8_t port = 0;
	void setVoltageMode(uint8_t row, uint8_t col, uint8_t outputMode_) {
		port = 3 * row + col;
		// +1 because enum starts at 1
		portModes[port] = (PORTMODE_t)(outputMode_ + 1);

		midi::Message msg;
		msg.bytes.resize(8);
		// F0 7D 17 2n 02 02 00 0m F7
		// Where n = 0 based port number
		// and m is the volt output mode to select from:
		msg.bytes = {0xF0, 0x7D, 0x17, static_cast<unsigned char>(32 + port), 0x02, 0x02, 0x00, portModes[port], 0xF7};
		midiOut.sendMessage(msg);
		// DEBUG("Voltage mode msg sent: port %d (%d), mode %d", port, static_cast<unsigned char>(32 + port), portModes[port]);
	}


	midi::InputQueue inputQueue;
	void requestParamOverSysex(uint8_t row, uint8_t col, uint8_t mode) {

		midi::Message msg;
		msg.bytes.resize(8);
		// F0 7D 17 00 01 03 00 nm pp F7
		uint8_t port = 3 * row + col;
		//Where n is:
		// 0 = Full configuration request. The module will send only pre def, port functions and modified parameters
		// 2 = Send Port configuration
		// 4 = Send MIDI Channel configuration
		// 6 = Send Voice Configuration

		uint8_t n = mode * 16;
		uint8_t m = port; // element number: 0-11 port number, 1-16 channel or voice number
		uint8_t pp = 2;
		msg.bytes = {0xF0, 0x7D, 0x17, 0x00, 0x01, 0x03, 0x00, static_cast<uint8_t>(n + m), pp, 0xF7};
		midiOut.sendMessage(msg);
		// DEBUG("API request mode msg sent: port %d, pp %s", port, msg.toString().c_str());
	}

	int getVoltageMode(uint8_t row, uint8_t col) {
		// -1 because menu is zero indexed but enum is not
		int channel = clamp(3 * row + col, 0, NUM_INPUTS - 1);
		return portModes[channel] - 1;
	}

	const static int NUM_INPUTS = 12;
	bool isClipping[NUM_INPUTS] = {};

	bool checkIsVoltageWithinRange(uint8_t channel, float voltage) {
		const float tol = 0.001;
		switch (portModes[channel]) {
			case MODE10V: return 0 - tol < voltage && voltage < 10 + tol;
			case MODEPN5V: return -5 - tol < voltage && voltage < 5 + tol;
			case MODENEG10V: return -10 - tol < voltage && voltage < 0 + tol;
			case MODE8V: return 0 - tol < voltage && voltage < 8 + tol;
			case MODE5V: return 0 - tol < voltage && voltage < 5 + tol;
			default: return false;
		}
	}

	uint16_t rescaleVoltageForChannel(uint8_t channel, float voltage) {
		switch (portModes[channel]) {
			case MODE10V: return rescale(clamp(voltage, 0.f, 10.f), 0.f, +10.f, 0, 16383);
			case MODEPN5V: return rescale(clamp(voltage, -5.f, 5.f), -5.f, +5.f, 0, 16383);
			case MODENEG10V: return rescale(clamp(voltage, -10.f, 0.f), -10.f, +0.f, 0, 16383);
			case MODE8V: return rescale(clamp(voltage, 0.f, 8.f), 0.f, +8.f, 0, 16383);
			case MODE5V: return rescale(clamp(voltage, 0.f, 5.f), 0.f, +5.f, 0, 16383);
			default: return 0;
		}
	}

	dsp::BooleanTrigger buttonTrigger;
	dsp::Timer rateLimiterTimer;
	PORTMODE_t portModes[NUM_INPUTS] = {};
	void process(const ProcessArgs& args) override {

		if (buttonTrigger.process(params[REFRESH_PARAM].getValue())) {			
			setPredef(4);
			refreshConfig();
		}

		midi::Message msg;
		uint8_t outData[32] = {};
		while (inputQueue.tryPop(&msg, args.frame)) {
			// DEBUG("msg (size: %d): %s", msg.getSize(), msg.toString().c_str());

			uint8_t outLen = decodeSysEx(&msg.bytes[0], outData, msg.bytes.size(), false);
			if (outLen > 3) {

				int channel = (outData[2] & 0x0f) >> 0;

				if (channel >= 0 && channel < NUM_INPUTS) {
					if (outData[outLen - 1] < LASTPORTMODE) {
						portModes[channel] = (PORTMODE_t) outData[outLen - 1];
						// DEBUG("Channel %d, %d: mode %d (%s)", outData[2], channel, portModes[channel], cfgPortModeNames[portModes[channel]]);
					}

				}
			}
		}


		// MIDI baud rate is 31250 b/s, or 3125 B/s.
		// CC messages are 3 bytes, so we can send a maximum of 1041 CC messages per second.
		// Since multiple CCs can be generated, play it safe and limit the CC rate to 200 Hz.
		const float rateLimiterPeriod = 1 / 200.f;
		bool rateLimiterTriggered = (rateLimiterTimer.process(args.sampleTime) >= rateLimiterPeriod);
		if (rateLimiterTriggered)
			rateLimiterTimer.time -= rateLimiterPeriod;

		if (rateLimiterTriggered) {

			for (int c = 0; c < NUM_INPUTS; ++c) {

				if (!inputs[A1_INPUT + c].isConnected()) {
					continue;
				}
				const float channelVoltage = inputs[A1_INPUT + c].getVoltage();
				uint16_t pw = rescaleVoltageForChannel(c, channelVoltage);
				isClipping[c] = !checkIsVoltageWithinRange(c, channelVoltage);
				midi::Message m;
				m.setStatus(0xe);
				m.setNote(pw & 0x7f);
				m.setValue((pw >> 7) & 0x7f);
				m.setFrame(args.frame);

				midiOut.setChannel(c);
				midiOut.sendMessage(m);
			}
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "midiOutput", midiOut.toJson());
		json_object_set_new(rootJ, "inputQueue", inputQueue.toJson());

		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* midiOutputJ = json_object_get(rootJ, "midiOutput");
		if (midiOutputJ) {
			midiOut.fromJson(midiOutputJ);
		}

		json_t* midiInputQueueJ = json_object_get(rootJ, "inputQueue");
		if (midiInputQueueJ) {
			inputQueue.fromJson(midiInputQueueJ);
		}

		refreshConfig();
	}
};

struct MidiThingPort : PJ301MPort {
	int row = 0, col = 0;
	MidiThing* module;

	void appendContextMenu(Menu* menu) override {

		menu->addChild(new MenuSeparator());
		std::string label = string::f("Voltage Mode Port %d", 3 * row + col + 1);

		menu->addChild(createIndexSubmenuItem(label,
		{"0 to 10v", "-5 to 5v", "-10 to 0v", "0 to 8v", "0 to 5v"},
		[ = ]() {
			return module->getVoltageMode(row, col);
		},
		[ = ](int mode) {
			module->setVoltageMode(row, col, mode);
		}
		                                     ));

		menu->addChild(createIndexSubmenuItem("Get Port Info",
		{"Full", "Port", "MIDI", "Voice"},
		[ = ]() {
			return -1;
		},
		[ = ](int mode) {
			module->requestParamOverSysex(row, col, 2 * mode);
		}
		                                     ));
	}
};

// dervied from https://github.com/countmodula/VCVRackPlugins/blob/v2.0.0/src/components/CountModulaLEDDisplay.hpp
struct LEDDisplay : LightWidget {
	float fontSize = 9;
	Vec textPos = Vec(1, 13);
	int numChars = 7;
	int row = 0, col = 0;
	MidiThing* module;

	LEDDisplay() {
		box.size = mm2px(Vec(9.298, 5.116));
	}

	void setCentredPos(Vec pos) {
		box.pos.x = pos.x - box.size.x / 2;
		box.pos.y = pos.y - box.size.y / 2;
	}

	void drawBackground(const DrawArgs& args) override {
		// Background
		NVGcolor backgroundColor = nvgRGB(0x20, 0x20, 0x20);
		NVGcolor borderColor = nvgRGB(0x10, 0x10, 0x10);
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0.0, 0.0, box.size.x, box.size.y, 2.0);
		nvgFillColor(args.vg, backgroundColor);
		nvgFill(args.vg);
		nvgStrokeWidth(args.vg, 1.0);
		nvgStrokeColor(args.vg, borderColor);
		nvgStroke(args.vg);
	}

	void drawLight(const DrawArgs& args) override {
		// Background
		NVGcolor backgroundColor = nvgRGB(0x20, 0x20, 0x20);
		NVGcolor borderColor = nvgRGB(0x10, 0x10, 0x10);
		NVGcolor textColor = nvgRGB(0xff, 0x10, 0x10);

		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0.0, 0.0, box.size.x, box.size.y, 2.0);
		nvgFillColor(args.vg, backgroundColor);
		nvgFill(args.vg);
		nvgStrokeWidth(args.vg, 1.0);

		if (module) {
			const bool isClipping = module->isClipping[col + row * 3];
			if (isClipping) {
				borderColor = nvgRGB(0xff, 0x20, 0x20);
			}
		}

		nvgStrokeColor(args.vg, borderColor);
		nvgStroke(args.vg);

		std::shared_ptr<Font> font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/miso.otf"));

		if (font && font->handle >= 0) {

			std::string text = "?-?v";  // fallback if module not yet defined
			if (module) {
				text = module->cfgPortModeNames[module->getVoltageMode(row, col) + 1];
			}
			char buffer[numChars + 1];
			int l = text.size();
			if (l > numChars)
				l = numChars;

			nvgGlobalTint(args.vg, color::WHITE);

			text.copy(buffer, l);
			buffer[l] = '\0';

			nvgFontSize(args.vg, fontSize);
			nvgFontFaceId(args.vg, font->handle);
			nvgFillColor(args.vg, textColor);
			nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
			NVGtextRow textRow;
			nvgTextBreakLines(args.vg, text.c_str(), NULL, box.size.x, &textRow, 1);
			nvgTextBox(args.vg, textPos.x, textPos.y, box.size.x, textRow.start, textRow.end);
		}
	}

	void onButton(const ButtonEvent& e) override {
		if (e.button == GLFW_MOUSE_BUTTON_RIGHT && e.action == GLFW_PRESS) {
			ui::Menu* menu = createMenu();

			menu->addChild(createMenuLabel(string::f("Voltage mode port %d:", col + 3 * row + 1)));

			const std::string labels[5] = {"0 to 10v", "-5 to 5v", "-10 to 0v", "0 to 8v", "0 to 5v"};

			for (int i = 0; i < 5; ++i) {
				menu->addChild(createCheckMenuItem(labels[i], "",
				[ = ]() {
					return module->getVoltageMode(row, col) == i;
				},
				[ = ]() {
					module->setVoltageMode(row, col, i);;
				}
				                                  ));
			}

			e.consume(this);
			return;
		}

		LightWidget::onButton(e);
	}

};


struct MidiThingWidget : ModuleWidget {

	struct LedDisplayCenterChoiceEx : LedDisplayChoice {
		LedDisplayCenterChoiceEx() {
			box.size = mm2px(math::Vec(0, 8.0));
			color = nvgRGB(0xf0, 0xf0, 0xf0);
			bgColor = nvgRGBAf(0, 0, 0, 0);
			textOffset = math::Vec(0, 16);
		}

		void drawLayer(const DrawArgs& args, int layer) override {
			nvgScissor(args.vg, RECT_ARGS(args.clipBox));
			if (layer == 1) {
				if (bgColor.a > 0.0) {
					nvgBeginPath(args.vg);
					nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
					nvgFillColor(args.vg, bgColor);
					nvgFill(args.vg);
				}

				std::shared_ptr<window::Font> font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/miso.otf"));

				if (font && font->handle >= 0 && !text.empty()) {
					nvgFillColor(args.vg, color);
					nvgFontFaceId(args.vg, font->handle);
					nvgTextLetterSpacing(args.vg, -0.6f);
					nvgFontSize(args.vg, 10);
					nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
					NVGtextRow textRow;
					nvgTextBreakLines(args.vg, text.c_str(), NULL, box.size.x, &textRow, 1);
					nvgTextBox(args.vg, textOffset.x, textOffset.y, box.size.x, textRow.start, textRow.end);
				}
			}
			nvgResetScissor(args.vg);
		}
	};


	struct MidiDriverItem : ui::MenuItem {
		midi::Port* port;
		int driverId;
		void onAction(const event::Action& e) override {
			port->setDriverId(driverId);
		}
	};

	struct MidiDriverChoice : LedDisplayCenterChoiceEx {
		midi::Port* port;
		void onAction(const event::Action& e) override {
			if (!port)
				return;
			createContextMenu();
		}

		virtual ui::Menu* createContextMenu() {
			ui::Menu* menu = createMenu();
			menu->addChild(createMenuLabel("MIDI driver"));
			for (int driverId : midi::getDriverIds()) {
				MidiDriverItem* item = new MidiDriverItem;
				item->port = port;
				item->driverId = driverId;
				item->text = midi::getDriver(driverId)->getName();
				item->rightText = CHECKMARK(item->driverId == port->driverId);
				menu->addChild(item);
			}
			return menu;
		}

		void step() override {
			text = port ? port->getDriver()->getName() : "";
			if (text.empty()) {
				text = "(No driver)";
				color.a = 0.5f;
			}
			else {
				color.a = 1.f;
			}
		}
	};

	struct MidiDeviceItem : ui::MenuItem {
		midi::Port* port;
		int deviceId;
		void onAction(const event::Action& e) override {
			port->setDeviceId(deviceId);
		}
	};

	struct MidiDeviceChoice : LedDisplayCenterChoiceEx {
		midi::Port* port;
		void onAction(const event::Action& e) override {
			if (!port)
				return;
			createContextMenu();
		}

		virtual ui::Menu* createContextMenu() {
			ui::Menu* menu = createMenu();
			menu->addChild(createMenuLabel("MIDI device"));
			{
				MidiDeviceItem* item = new MidiDeviceItem;
				item->port = port;
				item->deviceId = -1;
				item->text = "(No device)";
				item->rightText = CHECKMARK(item->deviceId == port->deviceId);
				menu->addChild(item);
			}
			for (int deviceId : port->getDeviceIds()) {
				MidiDeviceItem* item = new MidiDeviceItem;
				item->port = port;
				item->deviceId = deviceId;
				item->text = port->getDeviceName(deviceId);
				item->rightText = CHECKMARK(item->deviceId == port->deviceId);
				menu->addChild(item);
			}
			return menu;
		}

		void step() override {
			text = port ? port->getDeviceName(port->deviceId) : "";
			if (text.empty()) {
				text = "(No device)";
				color.a = 0.5f;
			}
			else {
				color.a = 1.f;
			}
		}
	};

	struct MidiWidget : LedDisplay {
		MidiDriverChoice* driverChoice;
		LedDisplaySeparator* driverSeparator;
		MidiDeviceChoice* deviceChoice;
		LedDisplaySeparator* deviceSeparator;

		void setMidiPort(midi::Port* port) {

			clearChildren();
			math::Vec pos;

			MidiDriverChoice* driverChoice = createWidget<MidiDriverChoice>(pos);
			driverChoice->box.size = Vec(box.size.x, 20.f);
			//driverChoice->textOffset = Vec(6.f, 14.7f);
			driverChoice->color = nvgRGB(0xf0, 0xf0, 0xf0);
			driverChoice->port = port;
			addChild(driverChoice);
			pos = driverChoice->box.getBottomLeft();
			this->driverChoice = driverChoice;

			this->driverSeparator = createWidget<LedDisplaySeparator>(pos);
			this->driverSeparator->box.size.x = box.size.x;
			addChild(this->driverSeparator);

			MidiDeviceChoice* deviceChoice = createWidget<MidiDeviceChoice>(pos);
			deviceChoice->box.size = Vec(box.size.x, 21.f);
			//deviceChoice->textOffset = Vec(6.f, 14.7f);
			deviceChoice->color = nvgRGB(0xf0, 0xf0, 0xf0);
			deviceChoice->port = port;
			addChild(deviceChoice);
			pos = deviceChoice->box.getBottomLeft();
			this->deviceChoice = deviceChoice;
		}
	};


	MidiThingWidget(MidiThing* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/MidiThing.svg")));

		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		MidiWidget* midiInputWidget = createWidget<MidiWidget>(Vec(1.5f, 36.4f)); //mm2px(Vec(0.5f, 10.f)));
		midiInputWidget->box.size = mm2px(Vec(5.08 * 6 - 1, 13.5f));
		midiInputWidget->setMidiPort(module ? &module->midiOut : NULL);
		addChild(midiInputWidget);

		addParam(createParamCentered<BefacoButton>(mm2px(Vec(21.12, 57.32)), module, MidiThing::REFRESH_PARAM));

		const float xStartLed = 0.2 + 0.628;
		const float yStartLed = 28.019;

		for (int row = 0; row < 4; row++) {
			for (int col = 0; col < 3; col++) {

				LEDDisplay* display = createWidget<LEDDisplay>(mm2px(Vec(xStartLed + 9.751 * col, yStartLed + 5.796 * row)));
				display->module = module;
				display->row = row;
				display->col = col;
				addChild(display);

				auto input = createInputCentered<MidiThingPort>(mm2px(Vec(5.08 + 10 * col, 69.77 + 14.225 * row)), module, MidiThing::A1_INPUT + 3 * row + col);
				input->row = row;
				input->col = col;
				input->module = module;
				addInput(input);


			}
		}
	}

	void appendContextMenu(Menu* menu) override {
		MidiThing* module = dynamic_cast<MidiThing*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator());

		menu->addChild(createSubmenuItem("Select Device", "",
		[ = ](Menu * menu) {

			for (auto driverId : rack::midi::getDriverIds()) {
				midi::Driver* driver = midi::getDriver(driverId);
				const bool activeDriver = module->midiOut.getDriverId() == driverId;

				menu->addChild(createSubmenuItem(driver->getName(), CHECKMARK(activeDriver),
				[ = ](Menu * menu) {

					for (auto deviceId : driver->getOutputDeviceIds()) {
						const bool activeDevice = activeDriver && module->midiOut.getDeviceId() == deviceId;

						menu->addChild(createMenuItem(driver->getOutputDeviceName(deviceId),
						                              CHECKMARK(activeDevice),
						[ = ]() {
							module->midiOut.setDriverId(driverId);
							module->midiOut.setDeviceId(deviceId);

							module->inputQueue.setDriverId(driverId);
							module->inputQueue.setDeviceId(deviceId);
							module->inputQueue.setChannel(0); // TODO update

							module->refreshConfig();
							
							// DEBUG("Updating Output MIDI settings - driver: %s, device: %s",
							//        driver->getName().c_str(), driver->getOutputDeviceName(deviceId).c_str());
						}));
					}
				}));
			}
		}));


		menu->addChild(createIndexSubmenuItem("Select Hardware Preset",
		{"Predef 1", "Predef 2", "Predef 3", "Predef 4 (VCV Mode)"},
		[ = ]() {
			return -1;
		},
		[ = ](int mode) {
			module->setPredef(mode + 1);
			module->refreshConfig();
		}
		                                     ));

	}
};


Model* modelMidiThing = createModel<MidiThing, MidiThingWidget>("MidiThingV2");