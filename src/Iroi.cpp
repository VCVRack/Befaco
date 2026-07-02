#include "plugin.hpp"

#include "Iroi_1_0_0Patch.hpp"
#include "OwlSDKIntegration.hpp"
#include <array>
#include <atomic>

struct IroiVCV : Module {
    enum ParamId {
        FILTER_CUTOFF_PARAM,
        RESONATOR_TUNE_PARAM,
        MOD_LEVEL_PARAM,
        RESONANCE_PARAM,
        RESONATOR_FEEDBACK_PARAM,
        MOD_SPEED_PARAM,
        ECHO_DENSITY_PARAM,
        AMBIENCE_SPACETIME_PARAM,
        ECHO_REPEATS_PARAM,
        AMBIENCE_DECAY_PARAM,
        FILTER_VCA_PARAM,
        RESO_DRY_WET_PARAM,
        ECHO_DRY_WET_PARAM,
        AMB_DRY_WET_PARAM,
        MAP_MODE_PARAM,
        SHIFT_PARAM,
        RANDOM_PARAM,
        MAP_PARAM,

        // Alt parameters (shown when ALT edit mode is active)
        MOD_TYPE_PARAM,
        FILTER_MODE_PARAM,
        FILTER_POSITION_PARAM,
        RESONATOR_DISSONANCE_PARAM,
        ECHO_FILTER_PARAM,
        AMBIENCE_AUTOPAN_PARAM,

        // Mod-mapping parameters (shown in MOD edit mode)
        FILTER_CUTOFF_MOD_PARAM,
        RESONANCE_MOD_PARAM,
        RESONATOR_TUNE_MOD_PARAM,
        RESONATOR_FEEDBACK_MOD_PARAM,
        ECHO_DENSITY_MOD_PARAM,
        ECHO_REPEATS_MOD_PARAM,
        AMBIENCE_SPACETIME_MOD_PARAM,
        AMBIENCE_DECAY_MOD_PARAM,

        // CV-mapping parameters (shown in CV edit mode)
        FILTER_CUTOFF_CV_PARAM,
        RESONANCE_CV_PARAM,
        RESONATOR_TUNE_CV_PARAM,
        RESONATOR_FEEDBACK_CV_PARAM,
        ECHO_DENSITY_CV_PARAM,
        ECHO_REPEATS_CV_PARAM,
        AMBIENCE_SPACETIME_CV_PARAM,
        AMBIENCE_DECAY_CV_PARAM,

        // Random-mapping parameters (shown in RND edit mode)
        FILTER_CUTOFF_RND_PARAM,
        RESONANCE_RND_PARAM,
        RESONATOR_TUNE_RND_PARAM,
        RESONATOR_FEEDBACK_RND_PARAM,
        ECHO_DENSITY_RND_PARAM,
        ECHO_REPEATS_RND_PARAM,
        AMBIENCE_SPACETIME_RND_PARAM,
        AMBIENCE_DECAY_RND_PARAM,
        SLEW_TIME_PARAM,
        CLEAR_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        LEFT_INPUT,
        RIGHT_INPUT,
        F_VCA_INPUT,
        RESONATOR_INPUT,
        ECHO_INPUT,
        AMBIENCE_INPUT,
        SYNC_INPUT,
        RANDOM_INPUT,
        FILTER_CV_INPUT,
        RESONATOR_CV_INPUT,
        ECHO_CV_INPUT,
        AMBIENCE_CV_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        LEFT_OUTPUT,
        RIGHT_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        SYNC_LIGHT,
        ENUMS(LEVEL_LIGHT, 3),
        MOD_LIGHT,
        ENUMS(MAP_BUTTON_LED, 3),
        ENUMS(CLEAR_BUTTON_LED, 3),
        SHIFT_BUTTON_LED,
        RANDOM_BUTTON_LED,
        // prettification lights :)
        FILTER_TYPE_LP_LED,
        FILTER_TYPE_BP_LED,
        FILTER_TYPE_HP_LED,
        FILTER_TYPE_CF_LED,
        FILTER_POS_1_LED,
        FILTER_POS_2_LED,
        FILTER_POS_3_LED,
        FILTER_POS_4_LED,
        // modulation shape lights
        MODULATION_TYPE_RANDOM_LED,
        MODULATION_TYPE_SINE_LED,
        MODULATION_TYPE_RAMP_LED,
        MODULATION_TYPE_INVERTED_RAMP_LED,
        MODULATION_TYPE_SQUARE_LED,
        MODULATION_TYPE_SH_LED,
        MODULATION_TYPE_ENVF_LED,
        LIGHTS_LEN
    };

    enum EditMode {
        EDIT_MODE_NORMAL,
        EDIT_MODE_ALT,
        EDIT_MODE_MOD,
        EDIT_MODE_CV,
        EDIT_MODE_RND,
    };
    EditMode editMode = EDIT_MODE_NORMAL;

    Iroi_1_0_0Patch *patch = nullptr;
    static constexpr int kBlockSize = 32;
    AudioBuffer *bufferIn = nullptr;
    AudioBuffer *bufferOut = nullptr;
    int bufferIndex = 0;

    dsp::BooleanTrigger shiftButtonTrigger;
    dsp::BooleanTrigger randomButtonTrigger;
    dsp::BooleanTrigger mapButtonTrigger;
    dsp::BooleanTrigger clearButtonTrigger;
    dsp::SchmittTrigger randomInputTrigger;
    dsp::SchmittTrigger syncInputTrigger;
    bool useNativeRackRandomisation = false;
    ParamQuantity *slewTimeParamQuantity = nullptr;
    float clearFlashTime = 0.f;
    float randomLedBrightness = 0.f;
    std::atomic<bool> pendingRandomizeAction{false};
    std::array<float, 10> pendingSlewTargets{};
    bool hasPendingSlewTargets = false;
    uint64_t randomizeNonce = 0;
    enum MapMode {
        MAP_RANDOM,
        MAP_CV,
        MAP_MOD
    };

    struct SlewedParams {
        float targetValues[PARAMS_LEN] = {};
        float oldKnobValues[PARAMS_LEN] = {};
        float values[PARAMS_LEN] = {};
        dsp::ExponentialFilter paramFilter[PARAMS_LEN];
        bool slewActive = true;

        void update(int i, Param &param, float sampleTime) {
            float knobValue = param.getValue();
            // If user changed the knob, jump immediately.
            if (knobValue != oldKnobValues[i]) {
                paramFilter[i].out = values[i] = targetValues[i] = oldKnobValues[i] = knobValue;
            } else {
                if (slewActive) {
                    oldKnobValues[i] = values[i] = paramFilter[i].process(sampleTime, targetValues[i]);
                } else {
                    oldKnobValues[i] = values[i] = targetValues[i];
                }
                param.setValue(values[i]);
            }
        }

        void setSlewTime(float slewTime) {
            slewActive = slewTime > 0.f;
            for (int i = 0; i < PARAMS_LEN; i++) {
                paramFilter[i].setTau(0.3f * slewTime);
            }
        }

        void setTargetValue(int i, float targetValue) { targetValues[i] = targetValue; }
    };
    SlewedParams paramsSlewed;

    // Main parameters randomized by Oneiroi/Iroi logic.
    static constexpr std::array<ParamId, 10> slewedParamIds = {
        FILTER_CUTOFF_PARAM, RESONANCE_PARAM,          RESONATOR_TUNE_PARAM, RESONATOR_FEEDBACK_PARAM, ECHO_DENSITY_PARAM,
        ECHO_REPEATS_PARAM,  AMBIENCE_SPACETIME_PARAM, AMBIENCE_DECAY_PARAM, MOD_LEVEL_PARAM,          MOD_SPEED_PARAM,
    };

    struct ModTypeParamQuantity : ParamQuantity {
        std::string getDisplayValueString() override {
            switch ((Shape)getValue()) {
            case Shape::LORENZ: return "Random";
            case Shape::SINE: return "Sine";
            case Shape::RAMP: return "Ramp";
            case Shape::INVERTED_RAMP: return "Inverted ramp";
            case Shape::SQUARE: return "Square";
            case Shape::SH: return "Sample & hold";
            case Shape::EF: return "Envelope follower";
            default: return "Not recognised!";
            }
        }
    };

    struct FilterTypeParamQuantity : ParamQuantity {
        std::string getDisplayValueString() override {
            switch ((FilterMode)getValue()) {
            case FilterMode::LP: return "Lowpass";
            case FilterMode::HP: return "Highpass";
            case FilterMode::BP: return "Bandpass";
            case FilterMode::CF: return "Comb filter";
            default: return "Not recognised!";
            }
        }
    };

    struct FilterCutoffTypeParamQuantity : ParamQuantity {

        const float MIN_FREQ_LOG = log(10.f);
        const float MAX_FREQ_LOG = log(22000.f);

        std::string getDisplayValueString() override {
            // convert to Hz
            const float valueMapped = MIN_FREQ_LOG + getValue() * (MAX_FREQ_LOG - MIN_FREQ_LOG);
            const float freqHz = expf(valueMapped);
            return string::f("%.1f", freqHz);
        }

        void setDisplayValueString(const std::string s) override {
            float sv = std::atof(s.c_str());
            setValue((log(sv) - MIN_FREQ_LOG) / (MAX_FREQ_LOG - MIN_FREQ_LOG));
        }
    };

    IroiVCV() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        auto filterCutoff =
            configParam<FilterCutoffTypeParamQuantity>(FILTER_CUTOFF_PARAM, 0.f, 1.f, 1.f, "Filter cutoff", "Hz");
        filterCutoff->description = "Main cutoff frequency for the filter section.";
        configParam(RESONATOR_TUNE_PARAM, 0.f, 1.f, 0.5f, "Resonator tune");
        configParam(MOD_LEVEL_PARAM, 0.f, 1.f, 0.f, "Modulation level", "%", 0.f, 100.f);
        configParam(RESONANCE_PARAM, 0.f, 1.f, 0.f, "Resonance");
        configParam(RESONATOR_FEEDBACK_PARAM, 0.f, 1.f, 0.f, "Resonator feedback");
        configParam(MOD_SPEED_PARAM, 0.f, 1.f, 0.5f, "Modulation speed");

        auto echoDensity = configParam(ECHO_DENSITY_PARAM, 0.f, 1.f, 0.5f, "Echo time/density");
        echoDensity->description =
            "A 2-tap stereo ping-pong delay whose time ranges from 10 ms (counter clockwise) to 6 seconds (clockwise).\n"
            "If an external clock is used, the delay time is quantized (x1/64, x1/32, ... x1)";
        auto ambienceSpacetime = configParam(AMBIENCE_SPACETIME_PARAM, 0.f, 1.f, 0.8f, "Ambience spacetime");
        ambienceSpacetime->description = ambienceSpacetime->description =
            "Loosely based on a Schroeder reverberator, SPACETIME allows you to adjust size, filtering, and direction. At "
            "50\%\nyou get the smallest size and brightest tone, and at 0\% or 100\%, you get the biggest size and darkest "
            "tone.\nIf you set the value to less than 50%, the input signal will be reversed.";
        configParam(ECHO_REPEATS_PARAM, 0.f, 1.f, 0.f, "Echo repeats");
        configParam(AMBIENCE_DECAY_PARAM, 0.f, 1.f, 0.5f, "Ambience decay");

        auto filterVca = configParam(FILTER_VCA_PARAM, 0.f, 1.f, 1.f, "Filter VCA", "%", 0.f, 100.f);
        filterVca->description = "Filter VCA amount. When F.VCA CV is patched, this acts as a CV attenuator.";
        auto resonatorDryWet = configParam(RESO_DRY_WET_PARAM, 0.f, 1.f, 0.f, "Resonator dry/wet", "%", 0.f, 100.f);
        resonatorDryWet->description = "Resonator mix. When Resonator CV is patched, this acts as a CV attenuator.";
        auto echoDryWet = configParam(ECHO_DRY_WET_PARAM, 0.f, 1.f, 0.f, "Echo dry/wet", "%", 0.f, 100.f);
        echoDryWet->description = "Echo mix. When Echo CV is patched, this acts as a CV attenuator.";
        auto ambienceDryWet = configParam(AMB_DRY_WET_PARAM, 0.f, 1.f, 0.f, "Ambience dry/wet", "%", 0.f, 100.f);
        ambienceDryWet->description = "Ambience mix. When Ambience CV is patched, this acts as a CV attenuator.";

        auto mapMode = configSwitch(MAP_MODE_PARAM, 0.f, 2.f, 0.f, "Map mode", {"Random", "CV", "Mod"});
        mapMode->description = "Selects what MAP targets: randomization, CV mapping, or modulation.";

        auto shiftButton = configButton(SHIFT_PARAM, "Shift");
        shiftButton->description = "Accesses secondary control behavior.";
        auto randomButton = configButton(RANDOM_PARAM, "Randomize");
        randomButton->description = "Triggers a randomization event (same function as RANDOM input).";
        auto mapButton = configButton(MAP_PARAM, "Map");
        mapButton->description = "Applies mapping according to the selected map mode.";
        auto clearButton = configButton(CLEAR_PARAM, "Clear mappings");
        clearButton->description = "Clears mapping amounts for the selected map mode (CV or Mod).";

        // alt parameters
        auto modType = configParam<ModTypeParamQuantity>(MOD_TYPE_PARAM, 0.f, 6.f, 0.f, "Modulation type");
        modType->snapEnabled = true;
        auto filterMode = configParam<FilterTypeParamQuantity>(FILTER_MODE_PARAM, 0.f, 3.f, 0.f, "Filter mode");
        filterMode->snapEnabled = true;
        auto filterPosition = configParam(FILTER_POSITION_PARAM, 1.f, 4.f, 1.f, "Filter position");
        filterPosition->snapEnabled = true;
        configParam(RESONATOR_DISSONANCE_PARAM, 0.f, 1.f, 0.f, "Resonator dissonance");
        configParam(ECHO_FILTER_PARAM, 0.f, 1.f, 0.5f, "Echo filter (DJ style)");
        configParam(AMBIENCE_AUTOPAN_PARAM, 0.f, 1.f, 0.5f, "Ambience auto-pan");

        // MOD mapping params
        configParam(FILTER_CUTOFF_MOD_PARAM, 0.f, 1.f, 0.5f, "Filter cutoff modulation amount", "%", 0.f, 100.f);
        configParam(RESONANCE_MOD_PARAM, 0.f, 1.f, 0.f, "Resonance modulation amount", "%", 0.f, 100.f);
        configParam(RESONATOR_TUNE_MOD_PARAM, 0.f, 1.f, 0.f, "Resonator tune modulation amount", "%", 0.f, 100.f);
        configParam(RESONATOR_FEEDBACK_MOD_PARAM, 0.f, 1.f, 0.f, "Resonator feedback modulation amount", "%", 0.f, 100.f);
        configParam(ECHO_DENSITY_MOD_PARAM, 0.f, 1.f, 0.f, "Echo density modulation amount", "%", 0.f, 100.f);
        configParam(ECHO_REPEATS_MOD_PARAM, 0.f, 1.f, 0.f, "Echo repeats modulation amount", "%", 0.f, 100.f);
        configParam(AMBIENCE_SPACETIME_MOD_PARAM, 0.f, 1.f, 0.f, "Ambience spacetime modulation amount", "%", 0.f, 100.f);
        configParam(AMBIENCE_DECAY_MOD_PARAM, 0.f, 1.f, 0.f, "Ambience decay modulation amount", "%", 0.f, 100.f);

        // CV mapping params
        configParam(FILTER_CUTOFF_CV_PARAM, 0.f, 1.f, 1.f, "Filter cutoff CV amount", "%", 0.f, 100.f);
        configParam(RESONANCE_CV_PARAM, 0.f, 1.f, 0.f, "Resonance CV amount", "%", 0.f, 100.f);
        configParam(RESONATOR_TUNE_CV_PARAM, 0.f, 1.f, 1.f, "Resonator tune CV amount", "%", 0.f, 100.f);
        configParam(RESONATOR_FEEDBACK_CV_PARAM, 0.f, 1.f, 0.f, "Resonator feedback CV amount", "%", 0.f, 100.f);
        configParam(ECHO_DENSITY_CV_PARAM, 0.f, 1.f, 1.f, "Echo density CV amount", "%", 0.f, 100.f);
        configParam(ECHO_REPEATS_CV_PARAM, 0.f, 1.f, 0.f, "Echo repeats CV amount", "%", 0.f, 100.f);
        configParam(AMBIENCE_SPACETIME_CV_PARAM, 0.f, 1.f, 1.f, "Ambience spacetime CV amount", "%", 0.f, 100.f);
        configParam(AMBIENCE_DECAY_CV_PARAM, 0.f, 1.f, 0.f, "Ambience decay CV amount", "%", 0.f, 100.f);

        // Random mapping params
        configParam(FILTER_CUTOFF_RND_PARAM, 0.f, 1.f, 0.5f, "Filter cutoff random amount", "%", 0.f, 100.f);
        configParam(RESONANCE_RND_PARAM, 0.f, 1.f, 0.5f, "Resonance random amount", "%", 0.f, 100.f);
        configParam(RESONATOR_TUNE_RND_PARAM, 0.f, 1.f, 0.5f, "Resonator tune random amount", "%", 0.f, 100.f);
        configParam(RESONATOR_FEEDBACK_RND_PARAM, 0.f, 1.f, 0.5f, "Resonator feedback random amount", "%", 0.f, 100.f);
        configParam(ECHO_DENSITY_RND_PARAM, 0.f, 1.f, 0.5f, "Echo density random amount", "%", 0.f, 100.f);
        configParam(ECHO_REPEATS_RND_PARAM, 0.f, 1.f, 0.5f, "Echo repeats random amount", "%", 0.f, 100.f);
        configParam(AMBIENCE_SPACETIME_RND_PARAM, 0.f, 1.f, 0.5f, "Ambience spacetime random amount", "%", 0.f, 100.f);
        configParam(AMBIENCE_DECAY_RND_PARAM, 0.f, 1.f, 0.5f, "Ambience decay random amount", "%", 0.f, 100.f);

        slewTimeParamQuantity = configParam(SLEW_TIME_PARAM, 0.f, 10.0f, 0.2f, "Randomise slew time", "s");

        configInput(LEFT_INPUT, "Left");
        configInput(RIGHT_INPUT, "Right");
        auto fVcaInput = configInput(F_VCA_INPUT, "Filter VCA CV");
        fVcaInput->description = "Expected CV range: 0V to +5V.";
        auto resonatorInput = configInput(RESONATOR_INPUT, "Resonator dry/wet CV");
        resonatorInput->description = "Expected CV range: 0V to +5V.";
        auto echoInput = configInput(ECHO_INPUT, "Echo dry/wet CV");
        echoInput->description = "Expected CV range: 0V to +5V.";
        auto ambienceInput = configInput(AMBIENCE_INPUT, "Ambience dry/wet CV");
        ambienceInput->description = "Expected CV range: 0V to +5V.";
        auto syncInput = configInput(SYNC_INPUT, "Sync");
        syncInput->description = "Clock/sync input for time-based sections.";
        configInput(RANDOM_INPUT, "Random");
        auto filterCvInput = configInput(FILTER_CV_INPUT, "Filter cutoff CV");
        filterCvInput->description = "Expected CV range: -5V to +10V.";
        auto resonatorCvInput = configInput(RESONATOR_CV_INPUT, "Resonator tune CV");
        resonatorCvInput->description = "Expected CV range: -5V to +10V.";
        auto echoCvInput = configInput(ECHO_CV_INPUT, "Echo density CV");
        echoCvInput->description = "Expected CV range: -5V to +10V.";
        auto ambienceCvInput = configInput(AMBIENCE_CV_INPUT, "Ambience spacetime CV");
        ambienceCvInput->description = "Expected CV range: -5V to +10V.";

        configOutput(LEFT_OUTPUT, "Left");
        configOutput(RIGHT_OUTPUT, "Right");

        configBypass(LEFT_INPUT, LEFT_OUTPUT);
        configBypass(RIGHT_INPUT, RIGHT_OUTPUT);

        fast_log_set_table(fast_log_table, fast_log_table_size);
        fast_pow_set_table(fast_pow_table, fast_pow_table_size);

        patch = new Iroi_1_0_0Patch();

        bufferIn = AudioBuffer::create(2, kBlockSize);
        bufferOut = AudioBuffer::create(2, kBlockSize);
    }

    ~IroiVCV() {
        delete patch;
        AudioBuffer::destroy(bufferIn);
        AudioBuffer::destroy(bufferOut);
    }

    void onAdd(const AddEvent &e) override {
        Module::onAdd(e);
        // SHIFT is used as an edit-mode modifier. Since it's implemented as a
        // latching widget in VCV, force it off on module load so patches don't
        // unexpectedly open in ALT edit mode.
        params[SHIFT_PARAM].setValue(0.f);
        editMode = EDIT_MODE_NORMAL;
    }

    void onSampleRateChange() override {
        float newSampleRate = APP->engine->getSampleRate();
        float oldSampleRate = patch->getPatchState()->sampleRate;
        // this destroys and recreates the patch, so only do it if the sample rate has actually changed
        if (newSampleRate != oldSampleRate) {
            patch->updateSampleRateBlockSize(APP->engine->getSampleRate(), BLOCKSIZE);
        }
    }

    void updatePatchParameters(PatchCtrls *patchCtrls, PatchCvs *patchCvs, Ui *ui) {
        // Faders (also act as CV attenuators when CV is patched)
        patchCtrls->filterVol =
            params[FILTER_VCA_PARAM].getValue() * clamp(inputs[F_VCA_INPUT].getNormalVoltage(5.f) / 5.f, 0.f, 1.f);
        patchCtrls->resonatorVol =
            params[RESO_DRY_WET_PARAM].getValue() * clamp(inputs[RESONATOR_INPUT].getNormalVoltage(5.f) / 5.f, 0.f, 1.f);
        patchCtrls->echoVol =
            params[ECHO_DRY_WET_PARAM].getValue() * clamp(inputs[ECHO_INPUT].getNormalVoltage(5.f) / 5.f, 0.f, 1.f);
        patchCtrls->ambienceVol =
            params[AMB_DRY_WET_PARAM].getValue() * clamp(inputs[AMBIENCE_INPUT].getNormalVoltage(5.f) / 5.f, 0.f, 1.f);

        // filter
        ui->SetFilterCutoffMain(params[FILTER_CUTOFF_PARAM].getValue());
        patchCtrls->filterResonance = params[RESONANCE_PARAM].getValue();
        patchCtrls->filterMode = params[FILTER_MODE_PARAM].getValue() / 4.0f;
        patchCtrls->filterPosition = (params[FILTER_POSITION_PARAM].getValue() - 1) / 3.0f;

        // resonator
        patchCtrls->resonatorTune = params[RESONATOR_TUNE_PARAM].getValue();
        patchCtrls->resonatorFeedback = params[RESONATOR_FEEDBACK_PARAM].getValue();
        patchCtrls->resonatorDissonance = params[RESONATOR_DISSONANCE_PARAM].getValue();

        // echo
        patchCtrls->echoDensity = params[ECHO_DENSITY_PARAM].getValue();
        patchCtrls->echoRepeats = params[ECHO_REPEATS_PARAM].getValue();
        patchCtrls->echoFilter = params[ECHO_FILTER_PARAM].getValue();

        // ambience
        patchCtrls->ambienceSpacetime = params[AMBIENCE_SPACETIME_PARAM].getValue();
        patchCtrls->ambienceDecay = params[AMBIENCE_DECAY_PARAM].getValue();
        patchCtrls->ambienceAutoPan = params[AMBIENCE_AUTOPAN_PARAM].getValue();

        // modulation
        patchCtrls->modSpeed = params[MOD_SPEED_PARAM].getValue();
        patchCtrls->modLevel = params[MOD_LEVEL_PARAM].getValue();
        patchCtrls->modType = params[MOD_TYPE_PARAM].getValue() / 6.f;

        // MOD amounts
        patchCtrls->filterCutoffModAmount = params[FILTER_CUTOFF_MOD_PARAM].getValue();
        patchCtrls->filterResonanceModAmount = params[RESONANCE_MOD_PARAM].getValue();
        patchCtrls->resonatorTuneModAmount = params[RESONATOR_TUNE_MOD_PARAM].getValue();
        patchCtrls->resonatorFeedbackModAmount = params[RESONATOR_FEEDBACK_MOD_PARAM].getValue();
        patchCtrls->echoDensityModAmount = params[ECHO_DENSITY_MOD_PARAM].getValue();
        patchCtrls->echoRepeatsModAmount = params[ECHO_REPEATS_MOD_PARAM].getValue();
        patchCtrls->ambienceSpacetimeModAmount = params[AMBIENCE_SPACETIME_MOD_PARAM].getValue();
        patchCtrls->ambienceDecayModAmount = params[AMBIENCE_DECAY_MOD_PARAM].getValue();

        // CV amounts
        patchCtrls->filterCutoffCvAmount = params[FILTER_CUTOFF_CV_PARAM].getValue();
        patchCtrls->filterResonanceCvAmount = params[RESONANCE_CV_PARAM].getValue();
        patchCtrls->resonatorTuneCvAmount = params[RESONATOR_TUNE_CV_PARAM].getValue();
        patchCtrls->resonatorFeedbackCvAmount = params[RESONATOR_FEEDBACK_CV_PARAM].getValue();
        patchCtrls->echoDensityCvAmount = params[ECHO_DENSITY_CV_PARAM].getValue();
        patchCtrls->echoRepeatsCvAmount = params[ECHO_REPEATS_CV_PARAM].getValue();
        patchCtrls->ambienceSpacetimeCvAmount = params[AMBIENCE_SPACETIME_CV_PARAM].getValue();
        patchCtrls->ambienceDecayCvAmount = params[AMBIENCE_DECAY_CV_PARAM].getValue();

        // CV signals (-5V..+10V mapped to -0.5..1.0)
        patchCvs->filterCutoff = patchCvs->filterResonance = clamp(inputs[FILTER_CV_INPUT].getVoltage() / 10.f, -0.5f, 1.f);
        patchCvs->resonatorTune = patchCvs->resonatorFeedback =
            clamp(inputs[RESONATOR_CV_INPUT].getVoltage() / 10.f, -0.5f, 1.f);
        patchCvs->echoDensity = patchCvs->echoRepeats = clamp(inputs[ECHO_CV_INPUT].getVoltage() / 10.f, -0.5f, 1.f);
        patchCvs->ambienceSpacetime = patchCvs->ambienceDecay =
            clamp(inputs[AMBIENCE_CV_INPUT].getVoltage() / 10.f, -0.5f, 1.f);

        // Random amounts
        patchCtrls->filterCutoffRndAmount = params[FILTER_CUTOFF_RND_PARAM].getValue();
        patchCtrls->filterResonanceRndAmount = params[RESONANCE_RND_PARAM].getValue();
        patchCtrls->resonatorTuneRndAmount = params[RESONATOR_TUNE_RND_PARAM].getValue();
        patchCtrls->resonatorFeedbackRndAmount = params[RESONATOR_FEEDBACK_RND_PARAM].getValue();
        patchCtrls->echoDensityRndAmount = params[ECHO_DENSITY_RND_PARAM].getValue();
        patchCtrls->echoRepeatsRndAmount = params[ECHO_REPEATS_RND_PARAM].getValue();
        patchCtrls->ambienceSpacetimeRndAmount = params[AMBIENCE_SPACETIME_RND_PARAM].getValue();
        patchCtrls->ambienceDecayRndAmount = params[AMBIENCE_DECAY_RND_PARAM].getValue();

        // Map selector: firmware expects CV=0, RND≈0.33, MOD=1
        const int mapMode = (int)std::round(params[MAP_MODE_PARAM].getValue());
        switch (mapMode) {
        default:
        case 0: patchCtrls->mapTarget = 0.33f; break; // Random
        case 1: patchCtrls->mapTarget = 0.f; break;   // CV
        case 2: patchCtrls->mapTarget = 1.f; break;   // Mod
        }
    }

    void updateSlewedParams(float sampleTime) {
        paramsSlewed.setSlewTime(params[SLEW_TIME_PARAM].getValue());
        for (ParamId paramToSlew : slewedParamIds) {
            paramsSlewed.update(paramToSlew, params[paramToSlew], sampleTime);
        }
    }

    float getRandomizedValue(ParamId paramId, float amount) {
        const float value = params[paramId].getValue();

        float r1 = rack::random::uniform();
        float r2 = rack::random::uniform();

        float sign = (r2 < 0.5f) ? 1.f : -1.f;
        if (sign < 0.f && value <= 0.f) {
            sign = 1.f;
        } else if (sign > 0.f && value >= 1.f) {
            sign = -1.f;
        }

        float next = value + r1 * amount * sign;
        next = clamp(next, 0.f, 1.f);
        return next;
    }

    void setRandomTarget(ParamId paramId, float amount, bool immediate) {
        float next = getRandomizedValue(paramId, amount);
        if (immediate) {
            params[paramId].setValue(next);
        }
        paramsSlewed.setTargetValue(paramId, next);
    }

    float getRandomAmountForParam(ParamId paramId) {
        switch (paramId) {
        case FILTER_CUTOFF_PARAM: return params[FILTER_CUTOFF_RND_PARAM].getValue();
        case RESONANCE_PARAM: return params[RESONANCE_RND_PARAM].getValue();
        case RESONATOR_TUNE_PARAM: return params[RESONATOR_TUNE_RND_PARAM].getValue();
        case RESONATOR_FEEDBACK_PARAM: return params[RESONATOR_FEEDBACK_RND_PARAM].getValue();
        case ECHO_DENSITY_PARAM: return params[ECHO_DENSITY_RND_PARAM].getValue();
        case ECHO_REPEATS_PARAM: return params[ECHO_REPEATS_RND_PARAM].getValue();
        case AMBIENCE_SPACETIME_PARAM: return params[AMBIENCE_SPACETIME_RND_PARAM].getValue();
        case AMBIENCE_DECAY_PARAM: return params[AMBIENCE_DECAY_RND_PARAM].getValue();
        case MOD_LEVEL_PARAM:
        case MOD_SPEED_PARAM: return 0.5f;
        default: return 0.f;
        }
    }

    std::array<float, slewedParamIds.size()> captureSlewedParamValues() {
        std::array<float, slewedParamIds.size()> values{};
        for (size_t i = 0; i < slewedParamIds.size(); i++) {
            values[i] = params[slewedParamIds[i]].getValue();
        }
        return values;
    }

    std::array<float, slewedParamIds.size()> generateRandomizedValues() {
        std::array<float, slewedParamIds.size()> values{};
        for (size_t i = 0; i < slewedParamIds.size(); i++) {
            ParamId paramId = slewedParamIds[i];
            values[i] = getRandomizedValue(paramId, getRandomAmountForParam(paramId));
        }
        return values;
    }

    void applyRandomizedValues(const std::array<float, slewedParamIds.size()> &values, bool immediate) {
        for (size_t i = 0; i < slewedParamIds.size(); i++) {
            ParamId paramId = slewedParamIds[i];
            if (immediate) {
                params[paramId].setValue(values[i]);
            }
            paramsSlewed.setTargetValue(paramId, values[i]);
        }

        if (immediate) {
            hasPendingSlewTargets = false;
        } else {
            pendingSlewTargets = values;
            hasPendingSlewTargets = true;
            randomizeNonce++;
            if (params[SLEW_TIME_PARAM].getValue() > 0.f) {
                randomLedBrightness = 1.f;
            }
        }
    }

    void randomizeRelevantParams(const RandomizeEvent &e) {
        if (useNativeRackRandomisation) {
            Module::onRandomize(e);
            return;
        }

        // Keep slewed behavior for native Rack randomize actions (context menu / Cmd+R)
        // and serialize target state so history can undo/redo transitions.
        applyRandomizedValues(generateRandomizedValues(), false);
    }

    void onRandomize(const RandomizeEvent &e) override { randomizeRelevantParams(e); }

    bool consumePendingRandomizeAction() { return pendingRandomizeAction.exchange(false); }

    void clearCvMappings() {
        params[FILTER_CUTOFF_CV_PARAM].setValue(0.f);
        params[RESONANCE_CV_PARAM].setValue(0.f);
        params[RESONATOR_TUNE_CV_PARAM].setValue(0.f);
        params[RESONATOR_FEEDBACK_CV_PARAM].setValue(0.f);
        params[ECHO_DENSITY_CV_PARAM].setValue(0.f);
        params[ECHO_REPEATS_CV_PARAM].setValue(0.f);
        params[AMBIENCE_SPACETIME_CV_PARAM].setValue(0.f);
        params[AMBIENCE_DECAY_CV_PARAM].setValue(0.f);
    }

    void clearModMappings() {
        params[FILTER_CUTOFF_MOD_PARAM].setValue(0.f);
        params[RESONANCE_MOD_PARAM].setValue(0.f);
        params[RESONATOR_TUNE_MOD_PARAM].setValue(0.f);
        params[RESONATOR_FEEDBACK_MOD_PARAM].setValue(0.f);
        params[ECHO_DENSITY_MOD_PARAM].setValue(0.f);
        params[ECHO_REPEATS_MOD_PARAM].setValue(0.f);
        params[AMBIENCE_SPACETIME_MOD_PARAM].setValue(0.f);
        params[AMBIENCE_DECAY_MOD_PARAM].setValue(0.f);
    }

    void clearMappingsForCurrentMode() {
        const MapMode mapMode = (MapMode)std::round(params[MAP_MODE_PARAM].getValue());
        if (mapMode == MAP_CV) {
            clearCvMappings();
        } else if (mapMode == MAP_MOD) {
            clearModMappings();
        }
    }

    void triggerClearFlash() { clearFlashTime = 0.12f; }

    void processButtons() {
        auto shiftEvent = shiftButtonTrigger.processEvent(params[SHIFT_PARAM].getValue());
        if (shiftEvent == dsp::BooleanTrigger::Event::TRIGGERED) {
            patch->buttonChanged(SHIFT_BUTTON, Patch::ON, 0);
        } else if (shiftEvent == dsp::BooleanTrigger::Event::UNTRIGGERED) {
            patch->buttonChanged(SHIFT_BUTTON, Patch::OFF, 0);
        }

        auto randomButtonEvent = randomButtonTrigger.processEvent(params[RANDOM_PARAM].getValue());
        if (randomButtonEvent == dsp::BooleanTrigger::Event::TRIGGERED) {
            patch->buttonChanged(RANDOM_BUTTON, Patch::ON, 0);
            pendingRandomizeAction.store(true);
        } else if (randomButtonEvent == dsp::BooleanTrigger::Event::UNTRIGGERED) {
            patch->buttonChanged(RANDOM_BUTTON, Patch::OFF, 0);
        }

        auto mapButtonEvent = mapButtonTrigger.processEvent(params[MAP_PARAM].getValue());
        if (mapButtonEvent == dsp::BooleanTrigger::Event::TRIGGERED) {
            patch->buttonChanged(MAP_BUTTON, Patch::ON, 0);
        } else if (mapButtonEvent == dsp::BooleanTrigger::Event::UNTRIGGERED) {
            patch->buttonChanged(MAP_BUTTON, Patch::OFF, 0);
        }

        auto clearButtonEvent = clearButtonTrigger.processEvent(params[CLEAR_PARAM].getValue());
        if (clearButtonEvent == dsp::BooleanTrigger::Event::TRIGGERED && params[SHIFT_PARAM].getValue() > 0.5f) {
            clearMappingsForCurrentMode();
            triggerClearFlash();
        }

        auto randomInputEvent = randomInputTrigger.processEvent(inputs[RANDOM_INPUT].getVoltage());
        if (randomInputEvent == dsp::SchmittTrigger::Event::TRIGGERED) {
            patch->buttonChanged(RANDOM_IN, Patch::ON, 0);
            pendingRandomizeAction.store(true);
        } else if (randomInputEvent == dsp::SchmittTrigger::Event::UNTRIGGERED) {
            patch->buttonChanged(RANDOM_IN, Patch::OFF, 0);
        }

        auto syncInputEvent = syncInputTrigger.processEvent(inputs[SYNC_INPUT].getVoltage());
        if (syncInputEvent == dsp::SchmittTrigger::Event::TRIGGERED) {
            patch->buttonChanged(SYNC_IN, Patch::ON, 0);
        } else if (syncInputEvent == dsp::SchmittTrigger::Event::UNTRIGGERED) {
            patch->buttonChanged(SYNC_IN, Patch::OFF, 0);
        }
    }

    void updateLights(const ProcessArgs &args, const PatchCtrls *patchCtrls, const Ui *ui) {
        const float blockTime = args.sampleTime * kBlockSize;
        const float slewTime = params[SLEW_TIME_PARAM].getValue();
        if (randomLedBrightness > 0.f) {
            const float tau = std::max(0.3f * slewTime, 0.01f);
            randomLedBrightness *= std::max(0.f, 1.f - blockTime / tau);
        }

        lights[SYNC_LIGHT].setBrightnessSmooth(patch->isButtonPressed(SYNC_IN), args.sampleTime * kBlockSize);
        lights[LEVEL_LIGHT + 0].setBrightnessSmooth(ui->leds_[LED_INPUT_PEAK]->Get(), args.sampleTime * kBlockSize);
        lights[LEVEL_LIGHT + 1].setBrightnessSmooth(
            lights[LEVEL_LIGHT + 0].getBrightness() < 0.5f ? ui->leds_[LED_INPUT]->Get() : 0.f, args.sampleTime * kBlockSize);
        lights[LEVEL_LIGHT + 2].setBrightness(0.f);
        lights[MOD_LIGHT].setBrightness(patch->getParameterValue(MOD_LED_PARAM));
        lights[SHIFT_BUTTON_LED].setBrightness(ui->leds_[LED_SHIFT]->Get());
        lights[RANDOM_BUTTON_LED].setBrightness(std::max(ui->leds_[LED_RANDOM]->Get(), randomLedBrightness));

        const bool mapOn = ui->IsMapOn();
        const int mapMode = (int)std::round(params[MAP_MODE_PARAM].getValue());

        // RGB MAP LED color scheme:
        // - Random: white (R+G+B)
        // - CV: blue
        // - Mod: green
        float mapR = 0.f;
        float mapG = 0.f;
        float mapB = 0.f;
        if (mapOn) {
            switch (mapMode) {
            default:
            case 0: // Random
                mapR = 1.f;
                mapG = 1.f;
                mapB = 1.f;
                break;
            case 1: // CV
                mapB = 1.f;
                break;
            case 2: // Mod
                mapG = 1.f;
                break;
            }
        }

        lights[MAP_BUTTON_LED + 0].setBrightness(mapR);
        lights[MAP_BUTTON_LED + 1].setBrightness(mapG);
        lights[MAP_BUTTON_LED + 2].setBrightness(mapB);

        if (clearFlashTime > 0.f) {
            clearFlashTime -= args.sampleTime * kBlockSize;
            const MapMode mapMode = (MapMode)std::round(params[MAP_MODE_PARAM].getValue());

            lights[CLEAR_BUTTON_LED + 0].setBrightness(0.0f);
            lights[CLEAR_BUTTON_LED + 1].setBrightness(mapMode == MAP_MOD);
            lights[CLEAR_BUTTON_LED + 2].setBrightness(mapMode == MAP_CV);
        } else {
            lights[CLEAR_BUTTON_LED + 0].setBrightness(0.f);
            lights[CLEAR_BUTTON_LED + 1].setBrightness(0.f);
            lights[CLEAR_BUTTON_LED + 2].setBrightness(0.f);
        }

        FilterMode mode = (FilterMode)std::round(4 * patchCtrls->filterMode);
        lights[FILTER_TYPE_LP_LED].setBrightness(mode == FilterMode::LP);
        lights[FILTER_TYPE_BP_LED].setBrightness(mode == FilterMode::BP);
        lights[FILTER_TYPE_HP_LED].setBrightness(mode == FilterMode::HP);
        lights[FILTER_TYPE_CF_LED].setBrightness(mode == FilterMode::CF);

        FilterPosition position = (FilterPosition)std::round(3 * patchCtrls->filterPosition);
        lights[FILTER_POS_1_LED].setBrightness(position == FilterPosition::POSITION_1);
        lights[FILTER_POS_2_LED].setBrightness(position == FilterPosition::POSITION_2);
        lights[FILTER_POS_3_LED].setBrightness(position == FilterPosition::POSITION_3);
        lights[FILTER_POS_4_LED].setBrightness(position == FilterPosition::POSITION_4);

        Shape modulationShape = (Shape)std::round(patchCtrls->modType * (Shape::NOF_SHAPES - 1));
        lights[MODULATION_TYPE_RANDOM_LED].setBrightness(modulationShape == Shape::LORENZ);
        lights[MODULATION_TYPE_SINE_LED].setBrightness(modulationShape == Shape::SINE);
        lights[MODULATION_TYPE_RAMP_LED].setBrightness(modulationShape == Shape::RAMP);
        lights[MODULATION_TYPE_INVERTED_RAMP_LED].setBrightness(modulationShape == Shape::INVERTED_RAMP);
        lights[MODULATION_TYPE_SQUARE_LED].setBrightness(modulationShape == Shape::SQUARE);
        lights[MODULATION_TYPE_SH_LED].setBrightness(modulationShape == Shape::SH);
        lights[MODULATION_TYPE_ENVF_LED].setBrightness(modulationShape == Shape::EF);
    }

    void process(const ProcessArgs &args) override {
        if (!patch || !bufferIn || !bufferOut) {
            return;
        }

        PatchCtrls *patchCtrls = patch->getPatchCtrls();
        PatchCvs *patchCvs = patch->getPatchCvs();
        PatchState *patchState = patch->getPatchState();
        Ui *ui = patch->getUi();

        if (!patchCtrls || !patchCvs || !patchState || !ui) {
            return;
        }

        processButtons();

        const bool shiftActive = params[SHIFT_PARAM].getValue() > 0.5f;
        if (shiftActive) {
            editMode = EDIT_MODE_ALT;
        } else if (ui->IsMapOn()) {
            const int mapMode = (int)std::round(params[MAP_MODE_PARAM].getValue());
            switch (mapMode) {
            default:
            case 0: editMode = EDIT_MODE_RND; break;
            case 1: editMode = EDIT_MODE_CV; break;
            case 2: editMode = EDIT_MODE_MOD; break;
            }
        } else {
            editMode = EDIT_MODE_NORMAL;
        }

        if (bufferIndex == kBlockSize) {
            updateSlewedParams(args.sampleTime * kBlockSize);
            updatePatchParameters(patchCtrls, patchCvs, ui);

            bufferOut->copyFrom(*bufferIn);
            patch->processAudio(*bufferOut);
            bufferIndex = 0;

            updateLights(args, patchCtrls, ui);
        }

        bufferIn->getSamples(LEFT_CHANNEL)[bufferIndex] = inputs[LEFT_INPUT].getVoltageSum() / 5.f;
        if (inputs[RIGHT_INPUT].isConnected()) {
            bufferIn->getSamples(RIGHT_CHANNEL)[bufferIndex] = inputs[RIGHT_INPUT].getVoltageSum() / 5.f;
        } else {
            bufferIn->getSamples(RIGHT_CHANNEL)[bufferIndex] = bufferIn->getSamples(LEFT_CHANNEL)[bufferIndex];
        }

        if (outputs[RIGHT_OUTPUT].isConnected()) {
            outputs[LEFT_OUTPUT].setVoltage(5.f * bufferOut->getSamples(LEFT_CHANNEL)[bufferIndex]);
            outputs[RIGHT_OUTPUT].setVoltage(5.f * bufferOut->getSamples(RIGHT_CHANNEL)[bufferIndex]);
        } else {
            float out =
                2.5f * (bufferOut->getSamples(LEFT_CHANNEL)[bufferIndex] + bufferOut->getSamples(RIGHT_CHANNEL)[bufferIndex]);
            outputs[LEFT_OUTPUT].setVoltage(out);
        }

        bufferIndex++;
    }

    json_t *dataToJson() override {
        json_t *rootJ = json_object();
        json_object_set_new(rootJ, "useNativeRackRandomisation", json_boolean(useNativeRackRandomisation));
        json_object_set_new(rootJ, "hasPendingSlewTargets", json_boolean(hasPendingSlewTargets));
        json_object_set_new(rootJ, "randomizeNonce", json_integer((json_int_t)randomizeNonce));

        json_t *targetsJ = json_array();
        for (size_t i = 0; i < slewedParamIds.size(); i++) {
            json_array_append_new(targetsJ, json_real(pendingSlewTargets[i]));
        }
        json_object_set_new(rootJ, "pendingSlewTargets", targetsJ);
        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        json_t *useNativeRackRandomisationJ = json_object_get(rootJ, "useNativeRackRandomisation");
        if (useNativeRackRandomisationJ) {
            useNativeRackRandomisation = json_boolean_value(useNativeRackRandomisationJ);
        }

        json_t *hasPendingJ = json_object_get(rootJ, "hasPendingSlewTargets");
        if (hasPendingJ) {
            hasPendingSlewTargets = json_boolean_value(hasPendingJ);
        }

        json_t *nonceJ = json_object_get(rootJ, "randomizeNonce");
        if (nonceJ) {
            randomizeNonce = (uint64_t)json_integer_value(nonceJ);
        }

        json_t *targetsJ = json_object_get(rootJ, "pendingSlewTargets");
        if (targetsJ && json_is_array(targetsJ)) {
            for (size_t i = 0; i < slewedParamIds.size(); i++) {
                json_t *vJ = json_array_get(targetsJ, i);
                if (vJ) {
                    pendingSlewTargets[i] = json_number_value(vJ);
                }
            }
        }

        if (hasPendingSlewTargets) {
            for (size_t i = 0; i < slewedParamIds.size(); i++) {
                paramsSlewed.setTargetValue(slewedParamIds[i], pendingSlewTargets[i]);
            }
        } else {
            // Keep targets aligned with current values if no pending slew state is active.
            auto currentValues = captureSlewedParamValues();
            for (size_t i = 0; i < slewedParamIds.size(); i++) {
                paramsSlewed.setTargetValue(slewedParamIds[i], currentValues[i]);
            }
        }
    }
};

// adapted from VCV Free
struct BefacoButtonIroi : app::SvgSwitch {
    BefacoButtonIroi() {
        momentary = true;
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/components/BefacoButton.svg")));
    }
};
struct BefacoButtonIroiToggle : app::SvgSwitch {
    BefacoButtonIroiToggle() {
        momentary = false;
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/components/BefacoButton.svg")));
    }
};
template <typename TBase> struct VCVBezelLightBig : TBase {
    VCVBezelLightBig() {
        this->borderColor = color::WHITE_TRANSPARENT;
        this->bgColor = color::WHITE_TRANSPARENT;
        this->box.size = mm2px(math::Vec(9, 9));
    }
};
using BefacoRedLightButton = LightButton<BefacoButtonIroi, VCVBezelLightBig<TRedLight<app::ModuleLightWidget>>>;
using BefacoLightButton = LightButton<BefacoButtonIroi, VCVBezelLightBig<TRedGreenBlueLight<app::ModuleLightWidget>>>;
using BefacoRedLightToggleButton = LightButton<BefacoButtonIroiToggle, VCVBezelLightBig<TRedLight<app::ModuleLightWidget>>>;

template <bool filled> struct OneiroiLedT : TSvgLight<RedLight> {

    OneiroiLedT() {}

    void draw(const DrawArgs &args) override {}
    void drawLayer(const DrawArgs &args, int layer) override {
        if (layer == 1) {

            if (!sw->svg) {
                return;
            }

            if (module && !module->isBypassed()) {

                for (auto s = sw->svg->handle->shapes; s; s = s->next) {
                    if (filled) {
                        s->fill.color = ((int)(color.a * 255) << 24) + (((int)(color.b * 255)) << 16) +
                                        (((int)(color.g * 255)) << 8) + (int)(color.r * 255);
                        s->fill.type = NSVG_PAINT_COLOR;
                    } else {
                        s->stroke.color = ((int)(color.a * 255) << 24) + (((int)(color.b * 255)) << 16) +
                                          (((int)(color.g * 255)) << 8) + (int)(color.r * 255);
                        s->stroke.type = NSVG_PAINT_COLOR;
                    }
                }

                nvgGlobalCompositeBlendFunc(args.vg, NVG_ONE_MINUS_DST_COLOR, NVG_ONE);
                svgDraw(args.vg, sw->svg->handle);
                drawHalo(args);
            }
        }
        Widget::drawLayer(args, layer);
    }
};
typedef OneiroiLedT<true> OneiroiShapeLed;
typedef OneiroiLedT<false> OneiroiPathLed;

struct OneiroiLedFilterTypeLowpass : OneiroiShapeLed {
    OneiroiLedFilterTypeLowpass() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/iroi/filter_type_lp.svg")));
    }
};
struct OneiroiLedFilterTypeBandpass : OneiroiShapeLed {
    OneiroiLedFilterTypeBandpass() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/iroi/filter_type_bp.svg")));
    }
};
struct OneiroiLedFilterTypeHighpass : OneiroiShapeLed {
    OneiroiLedFilterTypeHighpass() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/iroi/filter_type_hp.svg")));
    }
};
struct OneiroiLedFilterTypeCombFilter : OneiroiShapeLed {
    OneiroiLedFilterTypeCombFilter() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/iroi/filter_type_cf.svg")));
    }
};

struct OneiroiLedFilterPosition1 : OneiroiShapeLed {
    OneiroiLedFilterPosition1() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/iroi/filter_pos_1.svg")));
    }
};
struct OneiroiLedFilterPosition2 : OneiroiShapeLed {
    OneiroiLedFilterPosition2() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/iroi/filter_pos_2.svg")));
    }
};
struct OneiroiLedFilterPosition3 : OneiroiShapeLed {
    OneiroiLedFilterPosition3() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/iroi/filter_pos_3.svg")));
    }
};
struct OneiroiLedFilterPosition4 : OneiroiShapeLed {
    OneiroiLedFilterPosition4() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/iroi/filter_pos_4.svg")));
    }
};

struct OneiroiLedModRandom : OneiroiPathLed {
    OneiroiLedModRandom() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/iroi/modulation_type_0_random.svg")));
    }
};
struct OneiroiLedModSine : OneiroiPathLed {
    OneiroiLedModSine() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/iroi/modulation_type_1_sine.svg")));
    }
};
struct OneiroiLedModRamp : OneiroiPathLed {
    OneiroiLedModRamp() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/iroi/modulation_type_2_ramp.svg")));
    }
};
struct OneiroiLedModInvertedRamp : OneiroiPathLed {
    OneiroiLedModInvertedRamp() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/iroi/modulation_type_3_inverted_ramp.svg")));
    }
};
struct OneiroiLedModSquare : OneiroiPathLed {
    OneiroiLedModSquare() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/iroi/modulation_type_4_square.svg")));
    }
};
struct OneiroiLedModSH : OneiroiPathLed {
    OneiroiLedModSH() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/iroi/modulation_type_5_sh.svg")));
    }
};
struct OneiroiLedModEnvFollower : OneiroiShapeLed {
    OneiroiLedModEnvFollower() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/iroi/modulation_type_6_envf.svg")));
    }
};

struct IroiWidget : ModuleWidget {
    ParamWidget *modulationTypeWidget = nullptr;
    ParamWidget *filterModeParamWidget = nullptr;
    ParamWidget *filterPositionWidget = nullptr;
    ParamWidget *dissonanceWidget = nullptr;
    ParamWidget *echoFilterWidget = nullptr;
    ParamWidget *autoPanWidget = nullptr;

    ParamWidget *filterCutoffModWidget = nullptr;
    ParamWidget *resonanceModWidget = nullptr;
    ParamWidget *resonatorTuneModWidget = nullptr;
    ParamWidget *resonatorFeedbackModWidget = nullptr;
    ParamWidget *echoDensityModWidget = nullptr;
    ParamWidget *echoRepeatsModWidget = nullptr;
    ParamWidget *ambienceSpacetimeModWidget = nullptr;
    ParamWidget *ambienceDecayModWidget = nullptr;

    ParamWidget *filterCutoffCvWidget = nullptr;
    ParamWidget *resonanceCvWidget = nullptr;
    ParamWidget *resonatorTuneCvWidget = nullptr;
    ParamWidget *resonatorFeedbackCvWidget = nullptr;
    ParamWidget *echoDensityCvWidget = nullptr;
    ParamWidget *echoRepeatsCvWidget = nullptr;
    ParamWidget *ambienceSpacetimeCvWidget = nullptr;
    ParamWidget *ambienceDecayCvWidget = nullptr;

    ParamWidget *filterCutoffRndWidget = nullptr;
    ParamWidget *resonanceRndWidget = nullptr;
    ParamWidget *resonatorTuneRndWidget = nullptr;
    ParamWidget *resonatorFeedbackRndWidget = nullptr;
    ParamWidget *echoDensityRndWidget = nullptr;
    ParamWidget *echoRepeatsRndWidget = nullptr;
    ParamWidget *ambienceSpacetimeRndWidget = nullptr;
    ParamWidget *ambienceDecayRndWidget = nullptr;

    ParamWidget *mapButtonWidget = nullptr;
    ParamWidget *clearButtonWidget = nullptr;

    struct IroiRandomizeSlewAction : history::ModuleAction {
        std::array<float, IroiVCV::slewedParamIds.size()> oldValues{};
        std::array<float, IroiVCV::slewedParamIds.size()> newValues{};

        IroiRandomizeSlewAction() { name = "randomize module"; }

        static IroiVCV *getIroiModuleById(int64_t moduleId) {
            app::ModuleWidget *mw = APP->scene->rack->getModule(moduleId);
            if (!mw) {
                return nullptr;
            }
            return dynamic_cast<IroiVCV *>(mw->module);
        }

        void undo() override {
            IroiVCV *module = getIroiModuleById(moduleId);
            if (!module) {
                return;
            }
            module->applyRandomizedValues(oldValues, true);
        }

        void redo() override {
            IroiVCV *module = getIroiModuleById(moduleId);
            if (!module) {
                return;
            }
            module->applyRandomizedValues(newValues, false);
        }
    };

    IroiWidget(IroiVCV *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/panels/Iroi.svg")));

        addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<Knurlie>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<Knurlie>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<Knurlie>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        addParam(
            createParamCentered<Davies1900hDarkGreyKnob>(mm2px(Vec(29.009, 38.369)), module, IroiVCV::FILTER_CUTOFF_PARAM));
        addParam(
            createParamCentered<Davies1900hDarkGreyKnob>(mm2px(Vec(62.726, 38.359)), module, IroiVCV::RESONATOR_TUNE_PARAM));
        addParam(createParamCentered<BefacoTinyKnobBlack>(mm2px(Vec(12.377, 49.17)), module, IroiVCV::MOD_LEVEL_PARAM));
        addParam(createParamCentered<BefacoTinyKnobLightGrey>(mm2px(Vec(45.648, 49.153)), module, IroiVCV::RESONANCE_PARAM));
        addParam(createParamCentered<BefacoTinyKnobLightGrey>(mm2px(Vec(79.801, 49.153)), module,
                                                              IroiVCV::RESONATOR_FEEDBACK_PARAM));
        addParam(createParamCentered<BefacoTinyKnobBlack>(mm2px(Vec(12.377, 70.804)), module, IroiVCV::MOD_SPEED_PARAM));
        addParam(createParamCentered<Davies1900hDarkGreyKnob>(mm2px(Vec(45.65, 70.793)), module, IroiVCV::ECHO_DENSITY_PARAM));
        addParam(createParamCentered<Davies1900hDarkGreyKnob>(mm2px(Vec(79.804, 70.765)), module,
                                                              IroiVCV::AMBIENCE_SPACETIME_PARAM));
        addParam(createParamCentered<BefacoTinyKnobLightGrey>(mm2px(Vec(29.008, 78.512)), module, IroiVCV::ECHO_REPEATS_PARAM));
        addParam(
            createParamCentered<BefacoTinyKnobLightGrey>(mm2px(Vec(62.725, 78.414)), module, IroiVCV::AMBIENCE_DECAY_PARAM));
        addParam(createParam<BefacoSlidePotSmall>(mm2px(Vec(10.253, 87.842)), module, IroiVCV::FILTER_VCA_PARAM));
        addParam(createParam<BefacoSlidePotSmall>(mm2px(Vec(55.761, 87.842)), module, IroiVCV::RESO_DRY_WET_PARAM));
        addParam(createParam<BefacoSlidePotSmall>(mm2px(Vec(69.189, 87.842)), module, IroiVCV::ECHO_DRY_WET_PARAM));
        addParam(createParam<BefacoSlidePotSmall>(mm2px(Vec(82.616, 87.842)), module, IroiVCV::AMB_DRY_WET_PARAM));
        addParam(createParam<CKSSNarrow3>(mm2px(Vec(43.656, 90.752)), module, IroiVCV::MAP_MODE_PARAM));
        addParam(createLightParamCentered<BefacoRedLightToggleButton>(mm2px(Vec(29.009, 94.492)), module, IroiVCV::SHIFT_PARAM,
                                                                      IroiVCV::SHIFT_BUTTON_LED));
        addParam(createLightParamCentered<BefacoRedLightButton>(mm2px(Vec(29.009, 110.464)), module, IroiVCV::RANDOM_PARAM,
                                                                IroiVCV::RANDOM_BUTTON_LED));
        mapButtonWidget = createLightParamCentered<BefacoLightButton>(mm2px(Vec(45.65, 110.464)), module, IroiVCV::MAP_PARAM,
                                                                      IroiVCV::MAP_BUTTON_LED);
        addParam(mapButtonWidget);
        clearButtonWidget = createLightParamCentered<BefacoLightButton>(mm2px(Vec(45.65, 110.464)), module,
                                                                        IroiVCV::CLEAR_PARAM, IroiVCV::CLEAR_BUTTON_LED);
        clearButtonWidget->hide();
        addParam(clearButtonWidget);

        // alternate knobs (hidden by default), mapped like Oneiroi
        modulationTypeWidget =
            createParamCentered<BefacoTinyKnobRed>(mm2px(Vec(12.377, 70.804)), module, IroiVCV::MOD_TYPE_PARAM);
        modulationTypeWidget->hide();
        addParam(modulationTypeWidget);
        filterModeParamWidget =
            createParamCentered<Davies1900hRedKnob>(mm2px(Vec(29.009, 38.369)), module, IroiVCV::FILTER_MODE_PARAM);
        filterModeParamWidget->hide();
        addParam(filterModeParamWidget);
        filterPositionWidget =
            createParamCentered<BefacoTinyKnobRed>(mm2px(Vec(45.648, 49.153)), module, IroiVCV::FILTER_POSITION_PARAM);
        filterPositionWidget->hide();
        addParam(filterPositionWidget);
        dissonanceWidget =
            createParamCentered<Davies1900hRedKnob>(mm2px(Vec(62.726, 38.359)), module, IroiVCV::RESONATOR_DISSONANCE_PARAM);
        dissonanceWidget->hide();
        addParam(dissonanceWidget);
        echoFilterWidget =
            createParamCentered<Davies1900hRedKnob>(mm2px(Vec(45.65, 70.793)), module, IroiVCV::ECHO_FILTER_PARAM);
        echoFilterWidget->hide();
        addParam(echoFilterWidget);
        autoPanWidget =
            createParamCentered<Davies1900hRedKnob>(mm2px(Vec(79.804, 70.765)), module, IroiVCV::AMBIENCE_AUTOPAN_PARAM);
        autoPanWidget->hide();
        addParam(autoPanWidget);

        // MOD amount knobs (hidden by default), green color scheme
        filterCutoffModWidget =
            createParamCentered<Davies1900hGreenKnob>(mm2px(Vec(29.009, 38.369)), module, IroiVCV::FILTER_CUTOFF_MOD_PARAM);
        filterCutoffModWidget->hide();
        addParam(filterCutoffModWidget);
        resonanceModWidget =
            createParamCentered<BefacoTinyKnobGreen>(mm2px(Vec(45.648, 49.153)), module, IroiVCV::RESONANCE_MOD_PARAM);
        resonanceModWidget->hide();
        addParam(resonanceModWidget);
        resonatorTuneModWidget =
            createParamCentered<Davies1900hGreenKnob>(mm2px(Vec(62.726, 38.359)), module, IroiVCV::RESONATOR_TUNE_MOD_PARAM);
        resonatorTuneModWidget->hide();
        addParam(resonatorTuneModWidget);
        resonatorFeedbackModWidget =
            createParamCentered<BefacoTinyKnobGreen>(mm2px(Vec(79.801, 49.153)), module, IroiVCV::RESONATOR_FEEDBACK_MOD_PARAM);
        resonatorFeedbackModWidget->hide();
        addParam(resonatorFeedbackModWidget);
        echoDensityModWidget =
            createParamCentered<Davies1900hGreenKnob>(mm2px(Vec(45.65, 70.793)), module, IroiVCV::ECHO_DENSITY_MOD_PARAM);
        echoDensityModWidget->hide();
        addParam(echoDensityModWidget);
        echoRepeatsModWidget =
            createParamCentered<BefacoTinyKnobGreen>(mm2px(Vec(29.008, 78.512)), module, IroiVCV::ECHO_REPEATS_MOD_PARAM);
        echoRepeatsModWidget->hide();
        addParam(echoRepeatsModWidget);
        ambienceSpacetimeModWidget = createParamCentered<Davies1900hGreenKnob>(mm2px(Vec(79.804, 70.765)), module,
                                                                               IroiVCV::AMBIENCE_SPACETIME_MOD_PARAM);
        ambienceSpacetimeModWidget->hide();
        addParam(ambienceSpacetimeModWidget);
        ambienceDecayModWidget =
            createParamCentered<BefacoTinyKnobGreen>(mm2px(Vec(62.725, 78.414)), module, IroiVCV::AMBIENCE_DECAY_MOD_PARAM);
        ambienceDecayModWidget->hide();
        addParam(ambienceDecayModWidget);

        // CV amount knobs (hidden by default), blue color scheme
        filterCutoffCvWidget =
            createParamCentered<Davies1900hBlueKnob>(mm2px(Vec(29.009, 38.369)), module, IroiVCV::FILTER_CUTOFF_CV_PARAM);
        filterCutoffCvWidget->hide();
        addParam(filterCutoffCvWidget);
        resonanceCvWidget =
            createParamCentered<BefacoTinyKnobBlue>(mm2px(Vec(45.648, 49.153)), module, IroiVCV::RESONANCE_CV_PARAM);
        resonanceCvWidget->hide();
        addParam(resonanceCvWidget);
        resonatorTuneCvWidget =
            createParamCentered<Davies1900hBlueKnob>(mm2px(Vec(62.726, 38.359)), module, IroiVCV::RESONATOR_TUNE_CV_PARAM);
        resonatorTuneCvWidget->hide();
        addParam(resonatorTuneCvWidget);
        resonatorFeedbackCvWidget =
            createParamCentered<BefacoTinyKnobBlue>(mm2px(Vec(79.801, 49.153)), module, IroiVCV::RESONATOR_FEEDBACK_CV_PARAM);
        resonatorFeedbackCvWidget->hide();
        addParam(resonatorFeedbackCvWidget);
        echoDensityCvWidget =
            createParamCentered<Davies1900hBlueKnob>(mm2px(Vec(45.65, 70.793)), module, IroiVCV::ECHO_DENSITY_CV_PARAM);
        echoDensityCvWidget->hide();
        addParam(echoDensityCvWidget);
        echoRepeatsCvWidget =
            createParamCentered<BefacoTinyKnobBlue>(mm2px(Vec(29.008, 78.512)), module, IroiVCV::ECHO_REPEATS_CV_PARAM);
        echoRepeatsCvWidget->hide();
        addParam(echoRepeatsCvWidget);
        ambienceSpacetimeCvWidget =
            createParamCentered<Davies1900hBlueKnob>(mm2px(Vec(79.804, 70.765)), module, IroiVCV::AMBIENCE_SPACETIME_CV_PARAM);
        ambienceSpacetimeCvWidget->hide();
        addParam(ambienceSpacetimeCvWidget);
        ambienceDecayCvWidget =
            createParamCentered<BefacoTinyKnobBlue>(mm2px(Vec(62.725, 78.414)), module, IroiVCV::AMBIENCE_DECAY_CV_PARAM);
        ambienceDecayCvWidget->hide();
        addParam(ambienceDecayCvWidget);

        // RND amount knobs (hidden by default), white color scheme
        filterCutoffRndWidget =
            createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(29.009, 38.369)), module, IroiVCV::FILTER_CUTOFF_RND_PARAM);
        filterCutoffRndWidget->hide();
        addParam(filterCutoffRndWidget);
        resonanceRndWidget =
            createParamCentered<BefacoTinyKnobWhite>(mm2px(Vec(45.648, 49.153)), module, IroiVCV::RESONANCE_RND_PARAM);
        resonanceRndWidget->hide();
        addParam(resonanceRndWidget);
        resonatorTuneRndWidget =
            createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(62.726, 38.359)), module, IroiVCV::RESONATOR_TUNE_RND_PARAM);
        resonatorTuneRndWidget->hide();
        addParam(resonatorTuneRndWidget);
        resonatorFeedbackRndWidget =
            createParamCentered<BefacoTinyKnobWhite>(mm2px(Vec(79.801, 49.153)), module, IroiVCV::RESONATOR_FEEDBACK_RND_PARAM);
        resonatorFeedbackRndWidget->hide();
        addParam(resonatorFeedbackRndWidget);
        echoDensityRndWidget =
            createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(45.65, 70.793)), module, IroiVCV::ECHO_DENSITY_RND_PARAM);
        echoDensityRndWidget->hide();
        addParam(echoDensityRndWidget);
        echoRepeatsRndWidget =
            createParamCentered<BefacoTinyKnobWhite>(mm2px(Vec(29.008, 78.512)), module, IroiVCV::ECHO_REPEATS_RND_PARAM);
        echoRepeatsRndWidget->hide();
        addParam(echoRepeatsRndWidget);
        ambienceSpacetimeRndWidget = createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(79.804, 70.765)), module,
                                                                               IroiVCV::AMBIENCE_SPACETIME_RND_PARAM);
        ambienceSpacetimeRndWidget->hide();
        addParam(ambienceSpacetimeRndWidget);
        ambienceDecayRndWidget =
            createParamCentered<BefacoTinyKnobWhite>(mm2px(Vec(62.725, 78.414)), module, IroiVCV::AMBIENCE_DECAY_RND_PARAM);
        ambienceDecayRndWidget->hide();
        addParam(ambienceDecayRndWidget);

        addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(5.008, 15.005)), module, IroiVCV::LEFT_INPUT));
        addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(15.169, 15.005)), module, IroiVCV::RIGHT_INPUT));
        addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(25.331, 15.005)), module, IroiVCV::F_VCA_INPUT));
        addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(35.492, 15.005)), module, IroiVCV::RESONATOR_INPUT));
        addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(45.653, 15.005)), module, IroiVCV::ECHO_INPUT));
        addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(55.815, 15.005)), module, IroiVCV::AMBIENCE_INPUT));
        addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(65.976, 15.005)), module, IroiVCV::SYNC_INPUT));
        addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(12.369, 30.719)), module, IroiVCV::RANDOM_INPUT));
        addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(45.68, 30.679)), module, IroiVCV::FILTER_CV_INPUT));
        addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(79.837, 30.709)), module, IroiVCV::RESONATOR_CV_INPUT));
        addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(28.992, 59.93)), module, IroiVCV::ECHO_CV_INPUT));
        addInput(createInputCentered<BefacoInputPort>(mm2px(Vec(62.763, 59.93)), module, IroiVCV::AMBIENCE_CV_INPUT));

        addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(76.137, 15.005)), module, IroiVCV::LEFT_OUTPUT));
        addOutput(createOutputCentered<BefacoOutputPort>(mm2px(Vec(86.299, 15.005)), module, IroiVCV::RIGHT_OUTPUT));

        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(5.036, 38.383)), module, IroiVCV::SYNC_LIGHT));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(12.368629, 59.975697)), module, IroiVCV::MOD_LIGHT));
        addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(86.386, 38.383)), module, IroiVCV::LEVEL_LIGHT));

        {
            // Coordinates parsed from res/panels/Iroi.svg layer "extra_leds".
            // The source items use transform="translate(0,1179.6467)", already
            // resolved into these final panel-space mm positions.
            constexpr float filterLedXNudge = -0.4f;
            addChild(createLight<OneiroiLedFilterTypeLowpass>(mm2px(Vec(21.188878f + filterLedXNudge, 44.5849f)), module,
                                                              IroiVCV::FILTER_TYPE_LP_LED));
            addChild(createLight<OneiroiLedFilterTypeBandpass>(mm2px(Vec(21.188878f + filterLedXNudge, 30.0338f)), module,
                                                               IroiVCV::FILTER_TYPE_BP_LED));
            addChild(createLight<OneiroiLedFilterTypeHighpass>(mm2px(Vec(34.920087f + filterLedXNudge, 30.0338f)), module,
                                                               IroiVCV::FILTER_TYPE_HP_LED));
            addChild(createLight<OneiroiLedFilterTypeCombFilter>(mm2px(Vec(34.920344f + filterLedXNudge, 44.5849f)), module,
                                                                 IroiVCV::FILTER_TYPE_CF_LED));
        }

        {
            // Filter position glyphs from the same "extra_leds" layer.
            constexpr float filterPosLedXNudge = -0.4f;
            addChild(createLight<OneiroiLedFilterPosition1>(mm2px(Vec(38.472104f + filterPosLedXNudge, 52.3725f)), module,
                                                            IroiVCV::FILTER_POS_1_LED));
            addChild(createLight<OneiroiLedFilterPosition2>(mm2px(Vec(39.859615f + filterPosLedXNudge, 42.2005f)), module,
                                                            IroiVCV::FILTER_POS_2_LED));
            addChild(createLight<OneiroiLedFilterPosition3>(mm2px(Vec(50.35912f + filterPosLedXNudge, 42.2005f)), module,
                                                            IroiVCV::FILTER_POS_3_LED));
            addChild(createLight<OneiroiLedFilterPosition4>(mm2px(Vec(51.701269f + filterPosLedXNudge, 52.3725f)), module,
                                                            IroiVCV::FILTER_POS_4_LED));
        }

        {
            // Coordinates parsed directly from res/panels/Iroi.svg (extra_leds)
            // using Inkscape query bounds on labeled IDs, including transforms.
            // IDs:
            // - path535: smooth_rand
            // - path536: sine
            // - path542: ramp
            // - path538: inv_ramp
            // - path537: square
            // - path534: stepped_rand (S&H)
            // - text548: envf
            addChild(
                createLight<OneiroiLedModRandom>(mm2px(Vec(5.5172f, 73.7293f)), module, IroiVCV::MODULATION_TYPE_RANDOM_LED));
            addChild(createLight<OneiroiLedModSine>(mm2px(Vec(4.3874f, 69.0126f)), module, IroiVCV::MODULATION_TYPE_SINE_LED));
            addChild(createLight<OneiroiLedModRamp>(mm2px(Vec(6.4419f, 64.1951f)), module, IroiVCV::MODULATION_TYPE_RAMP_LED));
            addChild(createLight<OneiroiLedModInvertedRamp>(mm2px(Vec(11.0332f, 62.6996f)), module,
                                                            IroiVCV::MODULATION_TYPE_INVERTED_RAMP_LED));
            addChild(
                createLight<OneiroiLedModSquare>(mm2px(Vec(16.9580f, 64.4493f)), module, IroiVCV::MODULATION_TYPE_SQUARE_LED));
            addChild(createLight<OneiroiLedModSH>(mm2px(Vec(19.0408f, 69.0589f)), module, IroiVCV::MODULATION_TYPE_SH_LED));
            addChild(createLight<OneiroiLedModEnvFollower>(mm2px(Vec(16.7897f, 73.7045f)), module,
                                                           IroiVCV::MODULATION_TYPE_ENVF_LED));
        }
    }

    void step() override {
        IroiVCV *iroi = dynamic_cast<IroiVCV *>(module);
        auto editMode = iroi ? iroi->editMode : IroiVCV::EDIT_MODE_NORMAL;

        const bool showAlt = editMode == IroiVCV::EDIT_MODE_ALT;
        const bool showMod = editMode == IroiVCV::EDIT_MODE_MOD;
        const bool showCv = editMode == IroiVCV::EDIT_MODE_CV;
        const bool showRnd = editMode == IroiVCV::EDIT_MODE_RND;

        modulationTypeWidget->visible = showAlt;
        filterModeParamWidget->visible = showAlt;
        filterPositionWidget->visible = showAlt;
        dissonanceWidget->visible = showAlt;
        echoFilterWidget->visible = showAlt;
        autoPanWidget->visible = showAlt;

        filterCutoffModWidget->visible = showMod;
        resonanceModWidget->visible = showMod;
        resonatorTuneModWidget->visible = showMod;
        resonatorFeedbackModWidget->visible = showMod;
        echoDensityModWidget->visible = showMod;
        echoRepeatsModWidget->visible = showMod;
        ambienceSpacetimeModWidget->visible = showMod;
        ambienceDecayModWidget->visible = showMod;

        filterCutoffCvWidget->visible = showCv;
        resonanceCvWidget->visible = showCv;
        resonatorTuneCvWidget->visible = showCv;
        resonatorFeedbackCvWidget->visible = showCv;
        echoDensityCvWidget->visible = showCv;
        echoRepeatsCvWidget->visible = showCv;
        ambienceSpacetimeCvWidget->visible = showCv;
        ambienceDecayCvWidget->visible = showCv;

        filterCutoffRndWidget->visible = showRnd;
        resonanceRndWidget->visible = showRnd;
        resonatorTuneRndWidget->visible = showRnd;
        resonatorFeedbackRndWidget->visible = showRnd;
        echoDensityRndWidget->visible = showRnd;
        echoRepeatsRndWidget->visible = showRnd;
        ambienceSpacetimeRndWidget->visible = showRnd;
        ambienceDecayRndWidget->visible = showRnd;

        mapButtonWidget->visible = !showAlt;
        clearButtonWidget->visible = showAlt;

        if (iroi && iroi->consumePendingRandomizeAction()) {
            if (iroi->useNativeRackRandomisation) {
                randomizeAction();
            } else {
                IroiRandomizeSlewAction *h = new IroiRandomizeSlewAction;
                h->moduleId = iroi->id;
                h->oldValues = iroi->captureSlewedParamValues();
                h->newValues = iroi->generateRandomizedValues();

                // Apply now so user hears the slewed randomization immediately.
                iroi->applyRandomizedValues(h->newValues, false);

                if (APP->history) {
                    APP->history->push(h);
                } else {
                    delete h;
                }
            }
        }

        ModuleWidget::step();
    }

    // for context menu
    struct SlewTimeSider : ui::Slider {
        IroiVCV *module = nullptr;

        explicit SlewTimeSider(IroiVCV *module_, ParamQuantity *q_) {
            module = module_;
            quantity = q_;
            this->box.size.x = 200.0f;
        }

        bool isEnabled() const { return !(module && module->useNativeRackRandomisation); }

        void draw(const DrawArgs &args) override {
            if (!isEnabled()) {
                nvgSave(args.vg);
                nvgGlobalAlpha(args.vg, BND_DISABLED_ALPHA);
                ui::Slider::draw(args);
                nvgRestore(args.vg);
                return;
            }
            ui::Slider::draw(args);
        }

        void onDragStart(const DragStartEvent &e) override {
            if (!isEnabled()) {
                return;
            }
            ui::Slider::onDragStart(e);
        }

        void onDragMove(const DragMoveEvent &e) override {
            if (!isEnabled()) {
                return;
            }
            ui::Slider::onDragMove(e);
        }

        void onDragEnd(const DragEndEvent &e) override {
            if (!isEnabled()) {
                return;
            }
            ui::Slider::onDragEnd(e);
        }

        void onDoubleClick(const DoubleClickEvent &e) override {
            if (!isEnabled()) {
                return;
            }
            ui::Slider::onDoubleClick(e);
        }
    };

    void appendContextMenu(Menu *menu) override {
        IroiVCV *module = dynamic_cast<IroiVCV *>(this->module);
        assert(module);

        menu->addChild(
            createIndexPtrSubmenuItem("Knob mode", {"Normal", "Alt (hold Shift)", "Mod", "CV", "Rnd"}, &module->editMode));
        menu->addChild(new MenuSeparator());
        menu->addChild(createBoolPtrMenuItem("Use native VCV randomisation", "", &module->useNativeRackRandomisation));
        menu->addChild(new SlewTimeSider(module, module->slewTimeParamQuantity));
    }
};

Model *modelIroi = createModel<IroiVCV, IroiWidget>("Iroi");
