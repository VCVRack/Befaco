#include "plugin.hpp"

#include <array>
#include <cmath>

#include "random8_core/Random8Core.h"
#include "random8_core/Random8PanelUX.h"
#include "random8_core/quantizer_scales.h"
#include "random8_core/r8_structs_vcv.h"

namespace {
constexpr float kButtonHaloRadius = 0.7f;
}

struct Random8 : Module {

    static const int NUM_CHANNELS = 8;
    static const int NUM_PAGES = 8;
    enum PageId {
        PAGE_PRESET,
        PAGE_DIVIDER,
        PAGE_PROB,
        PAGE_STYLE,
        PAGE_OFFSET,
        PAGE_SCALE,
        PAGE_SLIDE,
        PAGE_STEPS,
        PAGE_COUNT = NUM_PAGES
    };
    enum ParamId {
        ENUMS(GAIN_VALUE_PARAM, NUM_CHANNELS),
        ENUMS(DIVIDER_VALUE_PARAM, NUM_CHANNELS),
        ENUMS(PROB_VALUE_PARAM, NUM_CHANNELS),
        ENUMS(STYLE_VALUE_PARAM, NUM_CHANNELS),
        ENUMS(OFFSET_VALUE_PARAM, NUM_CHANNELS),
        ENUMS(SCALE_VALUE_PARAM, NUM_CHANNELS),
        ENUMS(SLIDE_VALUE_PARAM, NUM_CHANNELS),
        ENUMS(STEPS_VALUE_PARAM, NUM_CHANNELS),
        PRESET_PARAM,
        DIVIDER_PARAM,
        PROB_PARAM,
        STYLE_PARAM,
        OFFSET_PARAM,
        SCALE_PARAM,
        SLIDE_PARAM,
        STEPS_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        ENUMS(TRIG_INPUT, NUM_CHANNELS),
        INPUTS_LEN
    };
    enum OutputId {
        ENUMS(OUT_OUTPUT, NUM_CHANNELS),
        OUTPUTS_LEN
    };
    enum LightId {
        ENUMS(CH_LIGHT, NUM_CHANNELS),
        ENUMS(PRESET_LIGHT, 3),
        ENUMS(DIVIDER_LIGHT, 3),
        ENUMS(PROB_LIGHT, 3),
        ENUMS(STYLE_LIGHT, 3),
        ENUMS(OFFSET_LIGHT, 3),
        ENUMS(SCALE_LIGHT, 3),
        ENUMS(SLIDE_LIGHT, 3),
        ENUMS(STEPS_LIGHT, 3),
        LIGHTS_LEN
    };

    const int pageButtonParams[NUM_PAGES] = {PRESET_PARAM, DIVIDER_PARAM, PROB_PARAM,  STYLE_PARAM,
                                             OFFSET_PARAM, SCALE_PARAM,   SLIDE_PARAM, STEPS_PARAM};

    const int pageValueParams[NUM_PAGES] = {GAIN_VALUE_PARAM,   DIVIDER_VALUE_PARAM, PROB_VALUE_PARAM,  STYLE_VALUE_PARAM,
                                            OFFSET_VALUE_PARAM, SCALE_VALUE_PARAM,   SLIDE_VALUE_PARAM, STEPS_VALUE_PARAM};
    const char *pageNames[NUM_PAGES] = {"Preset", "Divider", "Probability", "Style", "Offset", "Scale", "Slide", "Steps"};
    const int pageLightBases[NUM_PAGES] = {PRESET_LIGHT, DIVIDER_LIGHT, PROB_LIGHT,  STYLE_LIGHT,
                                           OFFSET_LIGHT, SCALE_LIGHT,   SLIDE_LIGHT, STEPS_LIGHT};
    int currentPage = PAGE_PRESET;
    bool inMenu = false;
    int panelAppState = Random8Panel::PERFORMANCE;
    int lastTouchedMenuChannel = 0;
    std::array<int, NUM_CHANNELS> loopModes{};
    std::array<json_t *, NUM_CHANNELS> presetSlots{};
    Random8Core core;
    Random8Panel::UX panelUx;
    std::array<std::array<float, NUM_CHANNELS>, NUM_PAGES> lastPageValues{};
    std::array<Random8ChannelParams, NUM_CHANNELS> lastChannelParams{};
    std::array<bool, NUM_CHANNELS> channelParamsDirty{};

    struct ScaleParamQuantity : ParamQuantity {
        std::string getDisplayValueString() override {
            int index = static_cast<int>(std::round(getValue()));
            index = clamp(index, 0, R8::NUM_AVAILABLE_SCALES - 1);
            return befaco::scale_names[index];
        }
    };

    struct StyleParamQuantity : ParamQuantity {
        std::string getDisplayValueString() override {
            static const char *styleNames[] = {
                "Standard", "Rosc", "Gamma", "Expo", "Weibull", "Low-High", "FBM", "Perlin",
            };
            int index = static_cast<int>(std::round(getValue()));
            index = clamp(index, 0, R8::NUM_AVAILABLE_STYLES - 1);
            return styleNames[index];
        }
    };

    static const char *getStyleName(int style) {
        static const char *styleNames[] = {
            "Standard", "Rosc", "Gamma", "Expo", "Weibull", "Low-High", "FBM", "Perlin",
        };
        style = clamp(style, 0, R8::NUM_AVAILABLE_STYLES - 1);
        return styleNames[style];
    }

    static const char *getLoopModeName(int loopMode) {
        switch (loopMode) {
        case 1: return "Loop";
        case 2: return "Evolve";
        default: return "Off";
        }
    }

    Random8() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        for (int i = 0; i < NUM_CHANNELS; i++) {
            auto gain = configParam(GAIN_VALUE_PARAM + i, 0.f, 1.f, 0.5f, string::f("Gain Ch. %d", i + 1));
            gain->description = "Sets the output gain for the channel, pre-quantization.";

            auto divider = configParam(DIVIDER_VALUE_PARAM + i, 1.f, 8.f, 1.f, string::f("Divider Ch. %d", i + 1));
            divider->snapEnabled = true;
            divider->description = "Counts incoming triggers, output only changes after a specified number of steps.";

            auto prob = configParam(PROB_VALUE_PARAM + i, 0.f, 100.f, 100.f, string::f("Probability Ch. %d", i + 1), "%");
            prob->description = "Sets the probability that a trigger will be accepted.";

            const float numStyles = static_cast<float>(R8::NUM_AVAILABLE_STYLES - 1);
            auto style =
                configParam<StyleParamQuantity>(STYLE_VALUE_PARAM + i, 0.f, numStyles, 0.f, string::f("Style Ch. %d", i + 1));
            style->snapEnabled = true;
            style->description = "Selects the random generation algorithm for the output signal.";

            auto offset = configParam(OFFSET_VALUE_PARAM + i, 0.f, 10.f, 0.f, string::f("Offset Ch. %d", i + 1));
            offset->description = "Adds a voltage offset to the output signal (0V to +10V).";

            auto scale =
                configParam<ScaleParamQuantity>(SCALE_VALUE_PARAM + i, 0.f, static_cast<float>(R8::NUM_AVAILABLE_SCALES - 1),
                                                1.f, string::f("Scale Ch. %d", i + 1));
            scale->snapEnabled = true;
            scale->description = "Quantizes the output signal to the selected musical scale (leftmost is no quantization).";

            auto slide = configParam(SLIDE_VALUE_PARAM + i, 0.f, 1.f, 0.f, string::f("Slide Ch. %d", i + 1));
            slide->description = "Applies portamento (slew) to the output signal, after quantisation.";

            auto steps = configParam(STEPS_VALUE_PARAM + i, 1.f, 32.f, 16.f, string::f("Steps Ch. %d", i + 1));
            steps->snapEnabled = true;
            steps->description = "Length of the loop when in LOOP Mode.";
        }
        for (int i = 0; i < NUM_PAGES; i++) {
            configButton(pageButtonParams[i], string::f("Ch. %d", i + 1));
        }

        for (int i = 0; i < NUM_CHANNELS; i++) {
            configInput(TRIG_INPUT + i, string::f("Ch. %d trigger", i + 1));
            configOutput(OUT_OUTPUT + i, string::f("Ch. %d", i + 1));
        }

        controlDivider.setDivision(64);
        channelParamsDirty.fill(true);
        updateLastPageValues();
        syncCoreParams();
    }

    ~Random8() override {
        for (json_t *preset : presetSlots) {
            if (preset) {
                json_decref(preset);
            }
        }
    }

    dsp::SchmittTrigger trig[NUM_CHANNELS];
    dsp::ClockDivider controlDivider;
    float controlDt = 0.f;

    void setButtonLight(int index, float r, float g, float b) {
        const int base = pageLightBases[index];
        lights[base + 0].setBrightness(r);
        lights[base + 1].setBrightness(g);
        lights[base + 2].setBrightness(b);
    }

    void resetChannelToDefaults(int channel) {
        if (channel < 0 || channel >= NUM_CHANNELS) {
            return;
        }

        params[GAIN_VALUE_PARAM + channel].setValue(0.5f);
        params[DIVIDER_VALUE_PARAM + channel].setValue(1.f);
        params[PROB_VALUE_PARAM + channel].setValue(100.f);
        params[STYLE_VALUE_PARAM + channel].setValue(0.f);
        params[OFFSET_VALUE_PARAM + channel].setValue(0.f);
        params[SCALE_VALUE_PARAM + channel].setValue(1.f);
        params[SLIDE_VALUE_PARAM + channel].setValue(0.f);
        params[STEPS_VALUE_PARAM + channel].setValue(16.f);

        core.resetChannel(channel);
        loopModes[channel] = 0;
        channelParamsDirty[channel] = true;
    }

    void onReset(const ResetEvent &e) override {
        inMenu = false;
        currentPage = PAGE_PRESET;
        lastTouchedMenuChannel = 0;
        panelUx.reset();

        for (int i = 0; i < NUM_CHANNELS; i++) {
            resetChannelToDefaults(i);
        }

        for (int i = 0; i < NUM_PAGES; i++) {
            params[pageButtonParams[i]].setValue(0.f);
        }

        for (int page = 0; page < NUM_PAGES; page++) {
            for (int channel = 0; channel < NUM_CHANNELS; channel++) {
                lastPageValues[page][channel] = params[pageValueParams[page] + channel].getValue();
            }
        }
        channelParamsDirty.fill(true);
        syncCoreParams();
        controlDivider.reset();
        controlDt = 0.f;

        Module::onReset(e);
    }

    Random8ChannelParams readChannelParams(int channel) {
        Random8ChannelParams channelParams;
        channelParams.attenuation = params[GAIN_VALUE_PARAM + channel].getValue();
        channelParams.divider = static_cast<int>(std::round(params[DIVIDER_VALUE_PARAM + channel].getValue()));
        channelParams.probability = static_cast<int>(std::round(params[PROB_VALUE_PARAM + channel].getValue()));
        channelParams.style = static_cast<int>(std::round(params[STYLE_VALUE_PARAM + channel].getValue()));
        channelParams.offset = params[OFFSET_VALUE_PARAM + channel].getValue() / 10.f;
        channelParams.scale = static_cast<int>(std::round(params[SCALE_VALUE_PARAM + channel].getValue()));
        channelParams.slide = params[SLIDE_VALUE_PARAM + channel].getValue();
        channelParams.steps = static_cast<int>(std::round(params[STEPS_VALUE_PARAM + channel].getValue()));
        channelParams.loopMode = loopModes[channel];
        return channelParams;
    }

    std::vector<std::string> getChannelSettingLabels(int channel) {
        if (channel < 0 || channel >= NUM_CHANNELS) {
            return {};
        }

        std::vector<std::string> labels;
        labels.reserve(9);
        labels.push_back(string::f("GAIN: %d%%", static_cast<int>(std::round(params[GAIN_VALUE_PARAM + channel].getValue() * 100.f))));
        labels.push_back(string::f("DIVIDER: %d", static_cast<int>(std::round(params[DIVIDER_VALUE_PARAM + channel].getValue()))));
        labels.push_back(string::f("PROB: %d%%", static_cast<int>(std::round(params[PROB_VALUE_PARAM + channel].getValue()))));
        labels.push_back(string::f("STYLE: %s", getStyleName(static_cast<int>(std::round(params[STYLE_VALUE_PARAM + channel].getValue())))));
        labels.push_back(string::f("OFFSET: %.1fV", params[OFFSET_VALUE_PARAM + channel].getValue()));
        labels.push_back(
            string::f("SCALE: %s", befaco::scale_names[clamp(static_cast<int>(std::round(params[SCALE_VALUE_PARAM + channel].getValue())),
                                                             0, R8::NUM_AVAILABLE_SCALES - 1)]));
        labels.push_back(string::f("SLIDE: %d%%", static_cast<int>(std::round(params[SLIDE_VALUE_PARAM + channel].getValue() * 100.f))));
        labels.push_back(string::f("STEPS: %d", static_cast<int>(std::round(params[STEPS_VALUE_PARAM + channel].getValue()))));
        labels.push_back(string::f("LOOP: %s", getLoopModeName(loopModes[channel])));
        return labels;
    }

    static bool channelParamsChanged(const Random8ChannelParams &a, const Random8ChannelParams &b) {
        constexpr float kFloatEpsilon = 1e-6f;
        return std::fabs(a.attenuation - b.attenuation) > kFloatEpsilon || std::fabs(a.offset - b.offset) > kFloatEpsilon ||
               std::fabs(a.slide - b.slide) > kFloatEpsilon || a.scale != b.scale || a.style != b.style ||
               a.divider != b.divider || a.probability != b.probability || a.steps != b.steps || a.loopMode != b.loopMode;
    }

    void syncCoreParams() {
        for (int channel = 0; channel < NUM_CHANNELS; channel++) {
            const Random8ChannelParams channelParams = readChannelParams(channel);
            if (channelParamsDirty[channel] || channelParamsChanged(channelParams, lastChannelParams[channel])) {
                core.setChannelParams(channel, channelParams);
                lastChannelParams[channel] = channelParams;
                channelParamsDirty[channel] = false;
            }
        }
    }

    void updateLastTouchedMenuChannel() {
        if (!inMenu || currentPage < 0 || currentPage >= NUM_PAGES) {
            return;
        }

        for (int channel = 0; channel < NUM_CHANNELS; channel++) {
            const float value = params[pageValueParams[currentPage] + channel].getValue();
            if (std::fabs(value - lastPageValues[currentPage][channel]) > 1e-4f) {
                lastTouchedMenuChannel = channel;
            }
        }
    }

    void updateLastPageValues() {
        for (int page = 0; page < NUM_PAGES; page++) {
            for (int channel = 0; channel < NUM_CHANNELS; channel++) {
                lastPageValues[page][channel] = params[pageValueParams[page] + channel].getValue();
            }
        }
    }

    Random8Panel::Input readPanelInput(float dt) {
        Random8Panel::Input panelInput;
        panelInput.dt = dt;
        for (int i = 0; i < NUM_CHANNELS; i++) {
            panelInput.buttonDown[i] = params[pageButtonParams[i]].getValue() > 0.5f;
            panelInput.loopModes[i] = loopModes[i];
            panelInput.presetFilled[i] = presetSlots[i] != nullptr;
            panelInput.divider[i] = params[DIVIDER_VALUE_PARAM + i].getValue();
            panelInput.probability[i] = params[PROB_VALUE_PARAM + i].getValue();
            panelInput.style[i] = params[STYLE_VALUE_PARAM + i].getValue();
            panelInput.offset[i] = params[OFFSET_VALUE_PARAM + i].getValue();
            panelInput.scaleValue[i] = params[SCALE_VALUE_PARAM + i].getValue();
            panelInput.slide[i] = params[SLIDE_VALUE_PARAM + i].getValue();
            panelInput.steps[i] = params[STEPS_VALUE_PARAM + i].getValue();
        }
        panelInput.activeChannel = clamp(lastTouchedMenuChannel, 0, NUM_CHANNELS - 1);
        return panelInput;
    }

    void handlePanelActions(const Random8Panel::Output &panelOutput) {
        for (int actionIndex = 0; actionIndex < panelOutput.actionCount; actionIndex++) {
            const Random8Panel::Action &action = panelOutput.actions[actionIndex];
            switch (action.type) {
            case Random8Panel::ACTION_SET_LOOP_MODE:
                if (action.index >= 0 && action.index < NUM_CHANNELS) {
                    loopModes[action.index] = action.value;
                }
                break;
            case Random8Panel::ACTION_LOAD_PRESET:
                if (action.index >= 0 && action.index < NUM_CHANNELS) {
                    loadPresetSlot(action.index);
                }
                break;
            case Random8Panel::ACTION_SAVE_PRESET:
                if (action.index >= 0 && action.index < NUM_CHANNELS) {
                    savePresetSlot(action.index);
                }
                break;
            case Random8Panel::ACTION_RESET_CHANNEL_MASK:
                for (int channel = 0; channel < NUM_CHANNELS; channel++) {
                    if (action.mask & (1 << channel)) {
                        resetChannelToDefaults(channel);
                    }
                }
                break;
            case Random8Panel::ACTION_RESET_ALL_CHANNELS:
                for (int channel = 0; channel < NUM_CHANNELS; channel++) {
                    resetChannelToDefaults(channel);
                }
                break;
            case Random8Panel::ACTION_NONE: break;
            }
        }
    }

    void applyPanelOutput(const Random8Panel::Output &panelOutput) {
        inMenu = panelOutput.inMenu;
        currentPage = panelOutput.currentPage;
        panelAppState = panelOutput.appState;
        for (int page = 0; page < NUM_PAGES; page++) {
            const Random8Panel::RGB &c = panelOutput.lights[page];
            setButtonLight(page, c.r, c.g, c.b);
        }
    }

    void syncControlState(float dt) {
        updateLastTouchedMenuChannel();
        const Random8Panel::Output panelOutput = panelUx.step(readPanelInput(dt));
        handlePanelActions(panelOutput);
        applyPanelOutput(panelOutput);
        updateLastPageValues();
        syncCoreParams();
        updateChannelLights();
    }

    void setPanelPerformanceMode() {
        panelUx.setPerformanceMode();
        inMenu = false;
        currentPage = PAGE_PRESET;
        panelAppState = Random8Panel::PERFORMANCE;
    }

    void setPanelPage(int page) {
        const int clampedPage = clamp(page, 0, NUM_PAGES - 1);
        panelUx.enterPage(static_cast<Random8Panel::Page>(clampedPage));
        inMenu = true;
        currentPage = clampedPage;
        panelAppState = (clampedPage == PAGE_PRESET) ? Random8Panel::IN_PRESETS : Random8Panel::IN_MENU;
    }

    void processTriggers() {
        float normalledGateVoltage = 0.f;
        for (int i = 0; i < NUM_CHANNELS; i++) {
            const float gateVoltage = inputs[TRIG_INPUT + i].getNormalVoltage(normalledGateVoltage);
            const bool triggered = trig[i].process(gateVoltage);
            if (triggered) {
                core.onTrigger(i);
            }

            // set up normalling for other trigger inputs
            normalledGateVoltage =
                inputs[TRIG_INPUT + i].isConnected() ? inputs[TRIG_INPUT + i].getVoltage() : normalledGateVoltage;
        }
    }

    void updateChannelLights() {
        for (int i = 0; i < NUM_CHANNELS; i++) {
            const float outputVolts = core.getOutputVolts(i);
            lights[CH_LIGHT + i].setBrightness(outputVolts / 10.0f);
        }
    }

    void writeOutputs() {
        for (int i = 0; i < NUM_CHANNELS; i++) {
            outputs[OUT_OUTPUT + i].setVoltage(core.getOutputVolts(i));
        }
    }

    void process(const ProcessArgs &args) override {
        controlDt += args.sampleTime;
        if (controlDivider.process()) {
            syncControlState(controlDt);
            controlDt = 0.f;
        }

        processTriggers();
        core.process(args.sampleTime);
        writeOutputs();
    }

    json_t *stateToJson() {
        json_t *root = json_object();

        json_t *loops = json_array();
        for (int i = 0; i < NUM_CHANNELS; i++) {
            json_array_append_new(loops, json_integer(loopModes[i]));
        }
        json_object_set_new(root, "loopModes", loops);

        json_t *channels = json_array();
        for (int i = 0; i < NUM_CHANNELS; i++) {
            R8::Channel::ChannelState state = core.getChannelState(i);
            json_t *chan = json_object();
            json_object_set_new(chan, "style", json_integer(static_cast<int>(state.info.style)));
            json_object_set_new(chan, "seed", json_integer(state.info.seed));
            json_object_set_new(chan, "isLooping", json_integer(state.isLooping));
            json_object_set_new(chan, "loopSteps", json_integer(state.loopSteps));
            json_object_set_new(chan, "currentLoopIndex", json_integer(state.currentLoopIndex));
            json_object_set_new(chan, "triggerCount", json_integer(state.triggerCount));
            json_object_set_new(chan, "currentVal", json_integer(state.currentVal));
            json_object_set_new(chan, "nextVal", json_integer(state.nextVal));
            json_object_set_new(chan, "prevVal", json_integer(state.prevVal));

            json_t *sequence = json_array();
            for (uint16_t value : state.sequence) {
                json_array_append_new(sequence, json_integer(value));
            }
            json_object_set_new(chan, "sequence", sequence);
            json_array_append_new(channels, chan);
        }
        json_object_set_new(root, "channels", channels);
        return root;
    }

    json_t *presetToJson() {
        json_t *root = stateToJson();

        json_t *paramsJson = json_array();
        for (int i = 0; i < NUM_CHANNELS; i++) {
            json_t *chan = json_object();
            json_object_set_new(chan, "gain", json_real(params[GAIN_VALUE_PARAM + i].getValue()));
            json_object_set_new(chan, "divider", json_real(params[DIVIDER_VALUE_PARAM + i].getValue()));
            json_object_set_new(chan, "probability", json_real(params[PROB_VALUE_PARAM + i].getValue()));
            json_object_set_new(chan, "style", json_real(params[STYLE_VALUE_PARAM + i].getValue()));
            json_object_set_new(chan, "offset", json_real(params[OFFSET_VALUE_PARAM + i].getValue()));
            json_object_set_new(chan, "scale", json_real(params[SCALE_VALUE_PARAM + i].getValue()));
            json_object_set_new(chan, "slide", json_real(params[SLIDE_VALUE_PARAM + i].getValue()));
            json_object_set_new(chan, "steps", json_real(params[STEPS_VALUE_PARAM + i].getValue()));
            json_array_append_new(paramsJson, chan);
        }
        json_object_set_new(root, "params", paramsJson);
        return root;
    }

    void setParamFromJson(json_t *chan, const char *key, int paramId) {
        json_t *field = json_object_get(chan, key);
        if (field) {
            params[paramId].setValue(json_number_value(field));
        }
    }

    void presetFromJson(json_t *root) {
        dataFromJson(root);

        json_t *paramsJson = json_object_get(root, "params");
        if (!paramsJson) {
            return;
        }
        for (int i = 0; i < NUM_CHANNELS; i++) {
            json_t *chan = json_array_get(paramsJson, i);
            if (!chan) {
                continue;
            }

            setParamFromJson(chan, "gain", GAIN_VALUE_PARAM + i);
            setParamFromJson(chan, "divider", DIVIDER_VALUE_PARAM + i);
            setParamFromJson(chan, "probability", PROB_VALUE_PARAM + i);
            setParamFromJson(chan, "style", STYLE_VALUE_PARAM + i);
            setParamFromJson(chan, "offset", OFFSET_VALUE_PARAM + i);
            setParamFromJson(chan, "scale", SCALE_VALUE_PARAM + i);
            setParamFromJson(chan, "slide", SLIDE_VALUE_PARAM + i);
            setParamFromJson(chan, "steps", STEPS_VALUE_PARAM + i);
        }
    }

    void setPresetSlot(int slot, json_t *data) {
        if (slot < 0 || slot >= NUM_CHANNELS) {
            return;
        }
        if (presetSlots[slot]) {
            json_decref(presetSlots[slot]);
        }
        presetSlots[slot] = data;
    }

    void savePresetSlot(int slot) {
        if (slot < 0 || slot >= NUM_CHANNELS) {
            return;
        }
        setPresetSlot(slot, presetToJson());
    }

    bool loadPresetSlot(int slot) {
        if (slot < 0 || slot >= NUM_CHANNELS || !presetSlots[slot]) {
            return false;
        }
        presetFromJson(presetSlots[slot]);
        return true;
    }

    json_t *dataToJson() override {
        json_t *root = stateToJson();

        json_t *presets = json_array();
        for (int i = 0; i < NUM_CHANNELS; i++) {
            if (presetSlots[i]) {
                json_array_append(presets, presetSlots[i]);
            } else {
                json_array_append_new(presets, json_null());
            }
        }
        json_object_set_new(root, "presets", presets);
        return root;
    }

    void dataFromJson(json_t *root) override {
        json_t *loops = json_object_get(root, "loopModes");
        if (loops) {
            for (int i = 0; i < NUM_CHANNELS; i++) {
                json_t *value = json_array_get(loops, i);
                if (value) {
                    loopModes[i] = json_integer_value(value);
                }
            }
        }

        json_t *channels = json_object_get(root, "channels");
        if (channels) {
            for (int i = 0; i < NUM_CHANNELS; i++) {
                json_t *chan = json_array_get(channels, i);
                if (!chan) {
                    continue;
                }
                R8::Channel::ChannelState state;
                json_t *field = nullptr;

                field = json_object_get(chan, "style");
                if (field) {
                    state.info.style = static_cast<R8::RandomStyle>(json_integer_value(field));
                }
                field = json_object_get(chan, "seed");
                if (field) {
                    state.info.seed = json_integer_value(field);
                }
                field = json_object_get(chan, "isLooping");
                if (field) {
                    state.isLooping = json_integer_value(field);
                }
                field = json_object_get(chan, "loopSteps");
                if (field) {
                    state.loopSteps = json_integer_value(field);
                }
                field = json_object_get(chan, "currentLoopIndex");
                if (field) {
                    state.currentLoopIndex = json_integer_value(field);
                }
                field = json_object_get(chan, "triggerCount");
                if (field) {
                    state.triggerCount = json_integer_value(field);
                }
                field = json_object_get(chan, "currentVal");
                if (field) {
                    state.currentVal = json_integer_value(field);
                }
                field = json_object_get(chan, "nextVal");
                if (field) {
                    state.nextVal = json_integer_value(field);
                }
                field = json_object_get(chan, "prevVal");
                if (field) {
                    state.prevVal = json_integer_value(field);
                }

                json_t *sequence = json_object_get(chan, "sequence");
                if (sequence) {
                    size_t index = 0;
                    json_t *seqValue = nullptr;
                    json_array_foreach(sequence, index, seqValue) {
                        state.sequence.push_back(static_cast<uint16_t>(json_integer_value(seqValue)));
                    }
                }

                core.setChannelState(i, state);
            }
        }

        json_t *presets = json_object_get(root, "presets");
        if (presets) {
            for (int i = 0; i < NUM_CHANNELS; i++) {
                json_t *preset = json_array_get(presets, i);
                if (preset && json_is_object(preset)) {
                    setPresetSlot(i, json_deep_copy(preset));
                } else {
                    setPresetSlot(i, nullptr);
                }
            }
        }

        channelParamsDirty.fill(true);
    }
};

template <typename TBase> struct VCVBezelLightSmall : TBase {
    VCVBezelLightSmall() {
        this->borderColor = color::WHITE_TRANSPARENT;
        this->bgColor = color::WHITE_TRANSPARENT;
        // Slightly oversize the lit area so it visually fills the button cap
        // like Befaco's larger lit buttons do.
        this->box.size = mm2px(math::Vec(5.0, 5.0));
    }

    void drawTightHalo(const Widget::DrawArgs &args, float radiusScale) {
        // Match Rack's default halo behavior gates.
        if (args.fb || this->color.a <= 0.f) {
            return;
        }
        const float halo = settings::haloBrightness;
        if (halo <= 0.f) {
            return;
        }

        const math::Vec c = this->box.size.div(2);
        const float r = std::min(this->box.size.x, this->box.size.y) * 0.5f;
        const float outer = r * (1.f + clamp(radiusScale, 0.2f, 2.f));

        NVGcolor innerColor = color::mult(this->color, halo);
        NVGcolor outerColor = innerColor;
        outerColor.a = 0.f;
        NVGpaint paint = nvgRadialGradient(args.vg, c.x, c.y, r * 0.2f, outer, innerColor, outerColor);
        nvgBeginPath(args.vg);
        nvgRect(args.vg, c.x - outer, c.y - outer, outer * 2.f, outer * 2.f);
        nvgFillPaint(args.vg, paint);
        nvgFill(args.vg);
    }

    void drawLayer(const Widget::DrawArgs &args, int layer) override {
        if (layer == 1) {
            if (this->module) {
                drawTightHalo(args, kButtonHaloRadius);
            }
            this->drawLight(args);
        }

        Widget::drawLayer(args, layer);
    }

    void drawHalo(const Widget::DrawArgs &args) override {
        // Halo is conditionally drawn in drawLayer().
    }
};

typedef LightButton<VCVBezelSmallLight, VCVBezelLightSmall<RedGreenBlueLight>> Random8Button;

struct Random8ChannelButton : Random8Button {
    int channel = -1;

    void appendContextMenu(ui::Menu *menu) override {
        Random8Button::appendContextMenu(menu);

        Random8 *random8 = dynamic_cast<Random8 *>(module);
        if (!random8 || channel < 0 || channel >= Random8::NUM_CHANNELS) {
            return;
        }

        menu->addChild(new MenuSeparator());
        menu->addChild(createMenuItem(string::f("Initialize channel %d", channel + 1), "",
                                      [=]() { random8->resetChannelToDefaults(channel); }));
    }
};

struct Random8Widget : ModuleWidget {
    std::array<std::array<ParamWidget *, Random8::NUM_CHANNELS>, Random8::NUM_PAGES> pageKnobs{};
    std::array<Random8ChannelButton *, Random8::NUM_CHANNELS> channelButtons{};
    int lastVisiblePage = -1;

    Random8Widget(Random8 *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Random8.svg")));

        addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        const int pageValueParams[Random8::NUM_PAGES] = {
            Random8::GAIN_VALUE_PARAM,   Random8::DIVIDER_VALUE_PARAM, Random8::PROB_VALUE_PARAM,  Random8::STYLE_VALUE_PARAM,
            Random8::OFFSET_VALUE_PARAM, Random8::SCALE_VALUE_PARAM,   Random8::SLIDE_VALUE_PARAM, Random8::STEPS_VALUE_PARAM};

        const float inputY[Random8::NUM_CHANNELS] = {14.646f, 28.527f, 42.407f, 56.287f, 70.167f, 84.048f, 97.928f, 111.808f};
        const float lightY[Random8::NUM_CHANNELS] = {11.176f, 25.052f, 38.927f, 52.802f, 66.678f, 80.553f, 94.429f, 108.304f};

        for (int page = 0; page < Random8::NUM_PAGES; page++) {
            for (int i = 0; i < Random8::NUM_CHANNELS; i++) {
                auto *knob = createParamCentered<Trimpot>(mm2px(Vec(26.133f, inputY[i])), module, pageValueParams[page] + i);
                addParam(knob);
                pageKnobs[page][i] = knob;
            }
        }

        for (int i = 0; i < Random8::NUM_CHANNELS; i++) {
            addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(15.0f, inputY[i])), module, Random8::TRIG_INPUT + i));
            addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(5.004f, inputY[i])), module, Random8::OUT_OUTPUT + i));
            addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(36.0f, lightY[i])), module, Random8::CH_LIGHT + i));
            auto *button = createLightParamCentered<Random8ChannelButton>(
                mm2px(Vec(35.966f, inputY[i] + 3.423f)), module, Random8::PRESET_PARAM + i, Random8::PRESET_LIGHT + i * 3);
            button->channel = i;
            channelButtons[i] = button;
            addParam(button);
        }
        setPage(Random8::PAGE_PRESET);
    }

    void step() override {
        ModuleWidget::step();
        int page = Random8::PAGE_PRESET;
        if (module) {
            Random8 *random8 = static_cast<Random8 *>(module);
            page = random8->inMenu ? random8->currentPage : Random8::PAGE_PRESET;
        }
        if (page != lastVisiblePage) {
            setPage(page);
        }
        updateButtonDescriptions();
    }

    void updateButtonDescriptions() {
        auto *random8 = dynamic_cast<Random8 *>(module);
        if (!random8) {
            return;
        }

        const std::string menuDesc =
            "Press to exit menu.\nHold to select another menu page.\nVery long press to reset channel/all.";

        for (int i = 0; i < Random8::NUM_CHANNELS; i++) {
            auto *button = channelButtons[i];
            if (!button || !button->getParamQuantity()) {
                continue;
            }
            auto *pq = button->getParamQuantity();
            if (random8->panelAppState == Random8Panel::IN_PRESETS) {
                if (i == Random8::PAGE_PRESET) {
                    pq->description = "Press to exit preset mode.";
                } else if (random8->presetSlots[i]) {
                    pq->description =
                        "Press to load preset from this slot.\nHold and release to overwrite this slot.\nReturns to "
                        "performance mode.";
                } else {
                    pq->description =
                        "Press to save current state to this empty slot.\nHold and release to save as well.\nReturns to "
                        "performance mode.";
                }
            } else if (random8->inMenu) {
                pq->description = menuDesc;
            } else {
                pq->description = string::f("Short press to change loop mode.\nLong press to enter %s menu.\nDouble tap to "
                                            "exit loop mode.\nVery long press to reset channel.",
                                            random8->pageNames[i]);
            }
        }
    }

    void setPage(int page) {
        for (int p = 0; p < Random8::NUM_PAGES; p++) {
            const bool visible = (p == page);
            for (int i = 0; i < Random8::NUM_CHANNELS; i++) {
                if (pageKnobs[p][i]) {
                    pageKnobs[p][i]->visible = visible;
                }
            }
        }
        lastVisiblePage = page;
    }

    void appendContextMenu(Menu *menu) override {
        ModuleWidget::appendContextMenu(menu);
        auto *random8 = dynamic_cast<Random8 *>(module);
        if (!random8) {
            return;
        }

        menu->addChild(new MenuSeparator());
        menu->addChild(createMenuLabel("Presets:"));
        std::vector<std::string> presetLabels;
        presetLabels.reserve(Random8::NUM_CHANNELS);
        for (int i = 0; i < Random8::NUM_CHANNELS; i++) {
            const bool used = random8->presetSlots[i] != nullptr;
            presetLabels.push_back(string::f("Slot %d (%s)", i + 1, used ? "used" : "unused"));
        }
        menu->addChild(createIndexSubmenuItem(
            "Load preset", presetLabels, [=]() { return presetLabels.size(); },
            [=](size_t index) {
                if (random8) {
                    random8->loadPresetSlot(static_cast<int>(index));
                }
            }));
        menu->addChild(createIndexSubmenuItem(
            "Save preset", presetLabels, [=]() { return presetLabels.size(); },
            [=](size_t index) {
                if (random8) {
                    random8->savePresetSlot(static_cast<int>(index));
                }
            }));

        menu->addChild(new MenuSeparator());
        menu->addChild(createSubmenuItem("View channel settings", "", [=](Menu *menu) {
            for (int channel = 0; channel < Random8::NUM_CHANNELS; channel++) {
                menu->addChild(createSubmenuItem(string::f("Channel %d", channel + 1), "", [=](Menu *menu) {
                    for (const std::string &label : random8->getChannelSettingLabels(channel)) {
                        menu->addChild(createMenuLabel(label));
                    }
                }));
            }
        }));

        menu->addChild(new MenuSeparator());
        menu->addChild(createMenuLabel("Modes:"));
        menu->addChild(createBoolMenuItem(
            "Performance Mode", "", [=]() { return !random8->inMenu; },
            [=](bool val) {
                if (val) {
                    random8->setPanelPerformanceMode();
                }
            }));
        menu->addChild(new MenuSeparator());
        for (int p = 0; p < Random8::NUM_PAGES; p++) {
            menu->addChild(createBoolMenuItem(
                random8->pageNames[p], "", [=]() { return (random8->currentPage == p && random8->inMenu); },
                [=](bool val) {
                    if (val) {
                        random8->setPanelPage(p);
                    }
                }));
        }
    }
};

Model *modelRandom8 = createModel<Random8, Random8Widget>("Random8");
