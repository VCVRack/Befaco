#pragma once

#include <array>
#include <cmath>
#include <cstdint>

namespace Random8Panel {

struct RGB {
    float r = 0.f;
    float g = 0.f;
    float b = 0.f;
};

constexpr RGB rgb(uint8_t r, uint8_t g, uint8_t b) { return RGB{r / 255.f, g / 255.f, b / 255.f}; }

inline RGB mix(const RGB &a, const RGB &b, float t) {
    t = std::fmax(0.f, std::fmin(1.f, t));
    return RGB{
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
    };
}

inline RGB scale(const RGB &c, float t) { return RGB{c.r * t, c.g * t, c.b * t}; }

enum Page : int {
    PRESET = 0,
    DIVIDER,
    PROBABILITY,
    STYLE,
    OFFSET,
    SCALE,
    SLIDE,
    STEPS,
    NUM_PAGES
};

enum AppState : int {
    PERFORMANCE = 0,
    TOWARDS_MENU,
    IN_MENU,
    IN_PRESETS,
    TOWARDS_RESET,
    SHOULD_RESET,
};

enum ButtonPhase : uint8_t {
    UNPRESSED = 0,
    CLICKED,
    DOUBLE_CLICKED,
    PRESSED,
    HELD_1,
    HELD_2,
    HELD_3,
    HELD_4,
    RELEASED_1,
    RELEASED_2,
    RELEASED_3,
    RELEASED_4,
};

enum ActionType : uint8_t {
    ACTION_NONE = 0,
    ACTION_SET_LOOP_MODE,
    ACTION_LOAD_PRESET,
    ACTION_SAVE_PRESET,
    ACTION_RESET_CHANNEL_MASK,
    ACTION_RESET_ALL_CHANNELS,
};

struct Action {
    ActionType type = ACTION_NONE;
    int index = -1;
    int value = 0;
    uint8_t mask = 0;
};

struct Input {
    float dt = 0.f;
    std::array<bool, 8> buttonDown{};
    std::array<int, 8> loopModes{};
    std::array<bool, 8> presetFilled{};
    int activeChannel = 0;

    std::array<float, 8> divider{};
    std::array<float, 8> probability{};
    std::array<float, 8> style{};
    std::array<float, 8> offset{};
    std::array<float, 8> scaleValue{};
    std::array<float, 8> slide{};
    std::array<float, 8> steps{};
};

struct Output {
    bool inMenu = false;
    int currentPage = PRESET;
    AppState appState = PERFORMANCE;
    std::array<RGB, 8> lights{};
    std::array<Action, 16> actions{};
    int actionCount = 0;
};

class Button {
  public:
    void step(bool down, float dt) {
        if (down && !wasDown_) {
            pressTime_ = 0.f;
        }

        if (down) {
            pressTime_ += dt;
            if (pressTime_ > 5.f) {
                state_ = HELD_4;
            } else if (pressTime_ > 3.f) {
                state_ = HELD_3;
            } else if (pressTime_ > 1.2f) {
                state_ = HELD_2;
            } else if (pressTime_ > 1.f) {
                state_ = HELD_1;
            } else {
                state_ = PRESSED;
            }
        }

        if (!down && wasDown_) {
            if (state_ >= HELD_1 && state_ <= HELD_4) {
                state_ = static_cast<ButtonPhase>(state_ + 4);
            } else {
                if (lastShortReleaseAge_ <= 0.4f) {
                    state_ = DOUBLE_CLICKED;
                    lastShortReleaseAge_ = 10.f;
                } else {
                    state_ = CLICKED;
                    lastShortReleaseAge_ = 0.f;
                }
            }
            pressTime_ = 0.f;
        } else if (!down && !wasDown_) {
            if (state_ != UNPRESSED && state_ != PRESSED && state_ < HELD_1) {
                state_ = UNPRESSED;
            }
        }

        if (!down) {
            lastShortReleaseAge_ += dt;
        }

        wasDown_ = down;
    }

    ButtonPhase phase() const { return state_; }

    bool isHeld() const { return state_ >= HELD_1 && state_ <= HELD_4; }

    float pressTime() const { return wasDown_ ? pressTime_ : 0.f; }

    float holdVisual() const {
        if (state_ == HELD_1) {
            return std::fmax(0.f, std::fmin(1.f, (pressTime_ - 1.f) / 0.2f));
        }
        if (state_ == HELD_3) {
            return std::fmax(0.f, std::fmin(1.f, (pressTime_ - 3.f) / 2.f));
        }
        return 1.f;
    }

    void clearTransient() {
        if (!wasDown_ && state_ != UNPRESSED) {
            state_ = UNPRESSED;
        }
    }

    void reset() {
        pressTime_ = 0.f;
        lastShortReleaseAge_ = 10.f;
        wasDown_ = false;
        state_ = UNPRESSED;
    }

  private:
    float pressTime_ = 0.f;
    float lastShortReleaseAge_ = 10.f;
    bool wasDown_ = false;
    ButtonPhase state_ = UNPRESSED;
};

class UX {
  public:
    void reset() {
        appState_ = PERFORMANCE;
        currentPage_ = PRESET;
        lastButtonPressed_ = -1;
        presetSaveArmed_ = -1;
        flashWhite_ = 0.f;
        flashPurple_ = 0.f;
        phase_ = 0.f;
        for (Button &button : buttons_) {
            button.reset();
        }
    }

    void setPerformanceMode() {
        appState_ = PERFORMANCE;
        currentPage_ = PRESET;
        lastButtonPressed_ = -1;
        presetSaveArmed_ = -1;
        for (Button &button : buttons_) {
            button.reset();
        }
    }

    void enterPage(Page page) {
        const int pageIndex = clampInt(static_cast<int>(page), 0, NUM_PAGES - 1);
        currentPage_ = pageIndex;
        appState_ = (pageIndex == PRESET) ? IN_PRESETS : IN_MENU;
        lastButtonPressed_ = pageIndex;
        presetSaveArmed_ = -1;
        for (Button &button : buttons_) {
            button.reset();
        }
    }

    Output step(const Input &input) {
        phase_ += input.dt;
        flashWhite_ = std::fmax(0.f, flashWhite_ - input.dt);
        flashPurple_ = std::fmax(0.f, flashPurple_ - input.dt);

        for (int i = 0; i < 8; ++i) {
            buttons_[i].step(input.buttonDown[i], input.dt);
        }

        Output out;
        processStateMachine(input, out);
        render(input, out);

        out.inMenu = (appState_ == IN_MENU || appState_ == IN_PRESETS);
        out.currentPage = currentPage_;
        out.appState = appState_;

        for (Button &button : buttons_) {
            button.clearTransient();
        }
        return out;
    }

  private:
    static constexpr uint8_t kResetAllButtonsMask = (1u << 0) | (1u << 7);
    static constexpr RGB kBlack = rgb(0x00, 0x00, 0x00);
    static constexpr RGB kWhite = rgb(0xFF, 0xFF, 0xFF);
    static constexpr RGB kGray = rgb(0x7F, 0x7F, 0x7F);
    static constexpr std::array<RGB, 8> kMenuColors = {
        rgb(0xBE, 0x00, 0xD0), // preset
        rgb(0x00, 0x3F, 0xFF), // divider
        rgb(0x00, 0xFF, 0xBB), // probability
        rgb(0xCA, 0xDF, 0x99), // style
        rgb(0xFF, 0x00, 0x55), // offset
        rgb(0xFE, 0xFE, 0xFE), // scale
        rgb(0xA4, 0xDD, 0x00), // slide
        rgb(0xFF, 0x00, 0x00), // steps
    };
    static constexpr std::array<RGB, 16> kScaleColors = {
        // page 1
        rgb(0xFE, 0xFE, 0xFE), // Off
        rgb(0x8E, 0xA0, 0xA0), // Chromatic
        rgb(0xFE, 0x70, 0x00), // Major (Ionian)
        rgb(0xAB, 0x4C, 0x00), // Major Pentatonic
        rgb(0x00, 0x65, 0xFE), // Aeolian
        rgb(0x00, 0x3A, 0xA8), // Minor Pentatonic
        rgb(0xB0, 0x2F, 0x88), // Dorian
        rgb(0x00, 0x00, 0x8B), // Blues
        // page 2
        rgb(0xC7, 0x00, 0x33), // Harmonic Minor
        rgb(0x92, 0xA8, 0x00), // Mixolydian
        rgb(0x92, 0x00, 0xC1), // Whole Tone
        rgb(0xA3, 0x00, 0x00), // Phrygian
        rgb(0x08, 0x2C, 0xBD), // Lydian
        rgb(0x7B, 0x00, 0x5A), // Melodic Minor
        rgb(0x00, 0xB0, 0x16), // Hirajoshi
        rgb(0x8E, 0xA0, 0xA0), // Octaves
    };
    static constexpr std::array<RGB, 8> kStyleColors = {
        rgb(0xCA, 0xDF, 0x99), // Standard
        rgb(0xBF, 0x00, 0x00), // Rosc
        rgb(0xFF, 0x6F, 0x00), // Gamma
        rgb(0xFF, 0xFF, 0x00), // Expo
        rgb(0x00, 0xFF, 0x01), // Weibull
        rgb(0x00, 0x00, 0xA2), // Low-High
        rgb(0x77, 0x00, 0xA4), // FBM
        rgb(0xCC, 0x00, 0x6F), // Perlin
    };
    static constexpr std::array<RGB, 4> kStepColors = {
        rgb(0xFF, 0x00, 0x00),
        rgb(0xFE, 0x55, 0x00),
        rgb(0xFE, 0xFE, 0x00),
        rgb(0x00, 0xFF, 0x00),
    };

    void pushAction(Output &out, Action action) {
        if (out.actionCount < static_cast<int>(out.actions.size())) {
            out.actions[out.actionCount++] = action;
        }
    }

    void processStateMachine(const Input &input, Output &out) {
        switch (appState_) {
        case PERFORMANCE:
            for (int i = 0; i < 8; ++i) {
                switch (buttons_[i].phase()) {
                case CLICKED: {
                    int nextMode = input.loopModes[i] == 0 ? 1 : (input.loopModes[i] == 1 ? 2 : 1);
                    pushAction(out, {ACTION_SET_LOOP_MODE, i, nextMode, 0});
                    break;
                }
                case DOUBLE_CLICKED:
                    pushAction(out, {ACTION_SET_LOOP_MODE, i, 0, 0});
                    flashWhite_ = 0.15f;
                    break;
                case HELD_1:
                case HELD_2:
                case HELD_3:
                    lastButtonPressed_ = i;
                    currentPage_ = i;
                    appState_ = TOWARDS_MENU;
                    break;
                case HELD_4:
                    lastButtonPressed_ = i;
                    currentPage_ = i;
                    appState_ = TOWARDS_RESET;
                    break;
                default: break;
                }
                if (appState_ != PERFORMANCE) {
                    break;
                }
            }
            break;

        case TOWARDS_MENU: {
            const int i = lastButtonPressed_;
            if (i < 0) {
                appState_ = PERFORMANCE;
                break;
            }
            switch (buttons_[i].phase()) {
            case HELD_3: currentPage_ = i; break;
            case HELD_4: appState_ = TOWARDS_RESET; break;
            case RELEASED_1:
            case RELEASED_2:
            case RELEASED_3:
                currentPage_ = i;
                appState_ = (currentPage_ == PRESET) ? IN_PRESETS : IN_MENU;
                break;
            case RELEASED_4: appState_ = SHOULD_RESET; break;
            default: break;
            }
            break;
        }

        case IN_MENU:
        case IN_PRESETS:
            for (int i = 0; i < 8; ++i) {
                switch (buttons_[i].phase()) {
                case CLICKED:
                    if (appState_ == IN_PRESETS) {
                        if (i == PRESET) {
                            presetSaveArmed_ = -1;
                            appState_ = PERFORMANCE;
                        } else if (input.presetFilled[i]) {
                            presetSaveArmed_ = -1;
                            pushAction(out, {ACTION_LOAD_PRESET, i, 0, 0});
                            appState_ = PERFORMANCE;
                        } else {
                            presetSaveArmed_ = -1;
                            pushAction(out, {ACTION_SAVE_PRESET, i, 0, 0});
                            flashPurple_ = 0.25f;
                            appState_ = PERFORMANCE;
                        }
                    } else {
                        appState_ = PERFORMANCE;
                    }
                    break;
                case DOUBLE_CLICKED:
                    if (appState_ != IN_PRESETS) {
                        appState_ = PERFORMANCE;
                    }
                    break;
                case HELD_1:
                case HELD_2:
                    if (appState_ == IN_PRESETS && i != PRESET) {
                        presetSaveArmed_ = i;
                    } else {
                        lastButtonPressed_ = i;
                        currentPage_ = i;
                    }
                    break;
                case HELD_3:
                    if (appState_ == IN_PRESETS && i != PRESET) {
                        presetSaveArmed_ = i;
                    } else {
                        lastButtonPressed_ = i;
                        currentPage_ = i;
                        appState_ = TOWARDS_MENU;
                        out.inMenu = true;
                    }
                    break;
                case HELD_4:
                    if (appState_ == IN_PRESETS && i != PRESET) {
                        presetSaveArmed_ = i;
                    } else {
                        lastButtonPressed_ = i;
                        currentPage_ = i;
                        appState_ = TOWARDS_RESET;
                        out.inMenu = false;
                    }
                    break;
                case RELEASED_1:
                case RELEASED_2:
                case RELEASED_3:
                case RELEASED_4:
                    if (appState_ == IN_PRESETS && i != PRESET && presetSaveArmed_ == i) {
                        pushAction(out, {ACTION_SAVE_PRESET, i, 0, 0});
                        flashPurple_ = 0.25f;
                        presetSaveArmed_ = -1;
                        appState_ = PERFORMANCE;
                    } else if (appState_ != IN_PRESETS && buttons_[i].phase() == RELEASED_4) {
                        appState_ = SHOULD_RESET;
                    } else {
                        currentPage_ = i;
                        appState_ = (currentPage_ == PRESET) ? IN_PRESETS : IN_MENU;
                    }
                    break;
                default: break;
                }
                if (appState_ != IN_MENU && appState_ != IN_PRESETS) {
                    break;
                }
            }
            break;

        case TOWARDS_RESET: {
            uint8_t heldMask = 0;
            uint8_t heldOrReleasedMask = 0;
            bool releasedVeryLongHold = false;
            for (int i = 0; i < 8; ++i) {
                ButtonPhase phase = buttons_[i].phase();
                if (phase == PRESSED || (phase >= HELD_1 && phase <= HELD_4)) {
                    heldMask |= static_cast<uint8_t>(1u << i);
                    heldOrReleasedMask |= static_cast<uint8_t>(1u << i);
                }
                if (phase == RELEASED_4) {
                    heldOrReleasedMask |= static_cast<uint8_t>(1u << i);
                    releasedVeryLongHold = true;
                }
            }

            if (releasedVeryLongHold) {
                if ((heldOrReleasedMask & 0x81u) == 0x81u) {
                    pushAction(out, {ACTION_RESET_ALL_CHANNELS, -1, 0, 0});
                } else {
                    pushAction(out, {ACTION_RESET_CHANNEL_MASK, -1, 0, heldOrReleasedMask});
                }
                flashWhite_ = 0.15f;
                appState_ = PERFORMANCE;
            } else if ((heldMask & kResetAllButtonsMask) == kResetAllButtonsMask) {
                appState_ = SHOULD_RESET;
            }
            break;
        }

        case SHOULD_RESET: {
            uint8_t heldMask = 0;
            bool topBottom = false;
            for (int i = 0; i < 8; ++i) {
                ButtonPhase phase = buttons_[i].phase();
                if (phase == PRESSED || (phase >= HELD_1 && phase <= HELD_4)) {
                    heldMask |= static_cast<uint8_t>(1u << i);
                }
                if (phase == RELEASED_4) {
                    heldMask |= static_cast<uint8_t>(1u << i);
                }
            }
            topBottom = (heldMask & kResetAllButtonsMask) == kResetAllButtonsMask;
            if (topBottom) {
                pushAction(out, {ACTION_RESET_ALL_CHANNELS, -1, 0, 0});
                flashWhite_ = 0.15f;
                appState_ = PERFORMANCE;
            } else {
                for (int i = 0; i < 8; ++i) {
                    if (buttons_[i].phase() == RELEASED_4) {
                        pushAction(out, {ACTION_RESET_CHANNEL_MASK, -1, 0, heldMask});
                        flashWhite_ = 0.15f;
                        appState_ = PERFORMANCE;
                        break;
                    }
                }
            }
            break;
        }
        }
    }

    void render(const Input &input, Output &out) {
        if (flashWhite_ > 0.f) {
            out.lights.fill(kWhite);
            return;
        }
        if (flashPurple_ > 0.f) {
            out.lights.fill(rgb(0x33, 0x00, 0xFF));
            return;
        }

        if (appState_ == PERFORMANCE) {
            float evolvePulse = 0.5f * (std::sin(phase_ * 6.28318530718f) + 1.f);
            for (int i = 0; i < 8; ++i) {
                if (input.loopModes[i] == 1) {
                    out.lights[i] = rgb(0xFF, 0x7F, 0x00);
                } else if (input.loopModes[i] == 2) {
                    out.lights[i] = scale(rgb(0xFF, 0x7F, 0x00), evolvePulse);
                } else {
                    out.lights[i] = kBlack;
                }
            }
            return;
        }

        if (appState_ == TOWARDS_MENU) {
            RGB menuColor = kMenuColors[currentPage_];
            ButtonPhase phase = buttons_[lastButtonPressed_].phase();
            if (phase == HELD_1) {
                out.lights.fill(mix(kBlack, menuColor, buttons_[lastButtonPressed_].holdVisual()));
            } else if (phase == HELD_2) {
                out.lights.fill(menuColor);
            } else if (phase == HELD_3) {
                out.lights.fill(mix(menuColor, kWhite, buttons_[lastButtonPressed_].holdVisual()));
            } else if (phase == HELD_4) {
                float pulse = std::fmod(phase_ * 8.f, 1.f) < 0.5f ? 1.f : 0.35f;
                out.lights.fill(scale(kWhite, pulse));
            } else {
                renderMenuBars(input, out);
            }
            return;
        }

        if (appState_ == TOWARDS_RESET || appState_ == SHOULD_RESET) {
            float pulse = std::fmod(phase_ * 8.f, 1.f) < 0.5f ? 1.f : 0.35f;
            out.lights.fill(scale(kWhite, pulse));
            return;
        }

        if (appState_ == IN_PRESETS && presetSaveArmed_ >= 1 && presetSaveArmed_ <= 7) {
            float pulse = std::fmod(phase_ * 8.f, 1.f) < 0.5f ? 1.f : 0.3f;
            out.lights.fill(scale(rgb(0xBE, 0x00, 0xD0), pulse));
            return;
        }

        renderMenuBars(input, out);
    }

    void renderMenuBars(const Input &input, Output &out) {
        out.lights.fill(kBlack);
        switch (currentPage_) {
        case PRESET: {
            const RGB presetUsed = rgb(0xCC, 0x00, 0x6F);
            const RGB presetUnused = rgb(0x77, 0x00, 0xA4);
            out.lights[0] = pulseColor(kGray);
            for (int i = 1; i < 8; ++i) {
                out.lights[i] = input.presetFilled[i] ? presetUsed : presetUnused;
            }
            break;
        }
        case DIVIDER:
            renderFill(out, clampInt(static_cast<int>(std::lround(input.divider[input.activeChannel])) - 1, 0, 7),
                       kMenuColors[DIVIDER]);
            break;
        case PROBABILITY:
            renderFill(out,
                       clampInt(static_cast<int>(std::lround((input.probability[input.activeChannel] / 100.f) * 7.f)), 0, 7),
                       kMenuColors[PROBABILITY]);
            break;
        case STYLE:
            renderPaletteFill(out, clampInt(static_cast<int>(std::lround(input.style[input.activeChannel])), 0, 7),
                              kStyleColors, true);
            break;
        case OFFSET:
            renderFill(out, clampInt(static_cast<int>(std::lround((input.offset[input.activeChannel] / 10.f) * 7.f)), 0, 7),
                       kMenuColors[OFFSET]);
            break;
        case SCALE:
            renderScale(out, clampInt(static_cast<int>(std::lround(input.scaleValue[input.activeChannel])), 0, 15));
            break;
        case SLIDE:
            renderFill(out, clampInt(static_cast<int>(std::lround(input.slide[input.activeChannel] * 7.f)), 0, 7),
                       kMenuColors[SLIDE]);
            break;
        case STEPS: renderSteps(out, clampInt(static_cast<int>(std::lround(input.steps[input.activeChannel])), 1, 32)); break;
        default: break;
        }

        if (lastButtonPressed_ >= 0 && lastButtonPressed_ < 8) {
            const RGB selectedMenuButtonColor = out.lights[lastButtonPressed_];
            if (selectedMenuButtonColor.r == 0.f && selectedMenuButtonColor.g == 0.f && selectedMenuButtonColor.b == 0.f) {
                out.lights[lastButtonPressed_] = pulseColor(kGray);
            } else {
                out.lights[lastButtonPressed_] = pulseColor(selectedMenuButtonColor);
            }
        }
    }

    template <size_t N> void renderPaletteFill(Output &out, int value, const std::array<RGB, N> &palette, bool reverse) {
        for (int i = 0; i < 8; ++i) {
            int idx = reverse ? (7 - i) : i;
            out.lights[i] = idx <= value ? palette[idx] : kBlack;
        }
    }

    void renderFill(Output &out, int value, RGB color) {
        for (int i = 0; i < 8; ++i) {
            out.lights[i] = (7 - i) <= value ? color : kBlack;
        }
    }

    void renderScale(Output &out, int value) {
        int wrapped = value % 8;
        int page = clampInt(value / 8, 0, 1);
        for (int i = 0; i < 8; ++i) {
            int idx = (7 - i) + (page * 8);
            out.lights[i] = (7 - i) <= wrapped ? kScaleColors[idx] : kBlack;
        }
    }

    void renderSteps(Output &out, int value) {
        int zeroBased = value - 1;
        int wrapped = zeroBased % 8;
        int page = clampInt(zeroBased / 8, 0, 3);
        for (int i = 0; i < 8; ++i) {
            out.lights[i] = (7 - i) <= wrapped ? kStepColors[page] : kBlack;
        }
    }

    RGB pulseColor(RGB base) const {
        float pulse = std::fmod(phase_ * 8.f, 1.f) < 0.5f ? 1.f : 0.35f;
        return scale(base, pulse);
    }

    static int clampInt(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

    AppState appState_ = PERFORMANCE;
    int currentPage_ = PRESET;
    int lastButtonPressed_ = -1;
    int presetSaveArmed_ = -1;
    float flashWhite_ = 0.f;
    float flashPurple_ = 0.f;
    float phase_ = 0.f;
    std::array<Button, 8> buttons_{};
};

} // namespace Random8Panel
