#pragma once

#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

#include "quantizer.h"
#include "r8_random_style_base.h"
#include "random_cores/r8_randomstyles_list.h"

namespace R8 {

struct ChannelData {
    RandomInfo coreInfo{};
    uint8_t myIndex = 0;
    uint8_t isLooping = 0;
    uint8_t loopSteps = 16;
    uint8_t scale = 1;
    uint8_t probability = 100;
    uint8_t divider = 1;
    uint8_t evolve = 1;
    uint16_t slide = 0;
    uint16_t atOffset = 0;
    uint16_t attenuation = SIXTEEN_BIT_MAX;
    uint16_t currentVal = 0;
    uint16_t nextVal = 0;
    uint16_t prevVal = 0;
};

class Channel {
  public:
    struct ChannelState {
        RandomInfo info{};
        uint8_t isLooping = 0;
        uint8_t loopSteps = 16;
        uint16_t currentVal = 0;
        uint16_t nextVal = 0;
        uint16_t prevVal = 0;
        int currentLoopIndex = -1;
        uint8_t triggerCount = 0;
        std::vector<uint16_t> sequence{};
    };

    ChannelData data;

    void setup(const ChannelData &_data) {
        data = _data;
        setCore(data.coreInfo);
        quantizer_.SetScale(data.scale);
        tick();
    }

    void setCore(RandomInfo info) {
        data.coreInfo = info;
        currentCore = assign_style(data.coreInfo);
        if (currentCore) {
            currentCore->setup(data.coreInfo);
        }
    }

    void setCore() {
        currentCore = assign_style(data.coreInfo);
        if (currentCore) {
            currentCore->create(data.coreInfo);
        }
    }

    void setScale(uint8_t scale) {
        data.scale = scale;
        quantizer_.SetScale(scale);
    }

    ChannelState getState() const {
        ChannelState state;
        state.info = data.coreInfo;
        state.isLooping = data.isLooping;
        state.loopSteps = data.loopSteps;
        state.currentVal = data.currentVal;
        state.nextVal = data.nextVal;
        state.prevVal = data.prevVal;
        state.currentLoopIndex = currentLoopIndex;
        state.triggerCount = triggerCount;
        if (currentCore) {
            state.sequence = currentCore->getSequence();
        }
        return state;
    }

    void setState(const ChannelState &state) {
        data.coreInfo = state.info;
        data.isLooping = state.isLooping;
        data.loopSteps = state.loopSteps;
        currentLoopIndex = state.currentLoopIndex;
        triggerCount = state.triggerCount;
        setCore(data.coreInfo);
        if (currentCore && !state.sequence.empty()) {
            currentCore->setSequence(state.sequence);
            currentCore->setCurrentValue(state.currentVal);
            currentCore->setNextValue(state.nextVal);
        }
        data.prevVal = state.prevVal;
        data.currentVal = state.currentVal;
        data.nextVal = state.nextVal;
        slided_val = static_cast<int64_t>(data.currentVal);
    }

    uint16_t copyNextIntoCurrent() {
        data.prevVal = data.currentVal;
        data.currentVal = data.nextVal;
        return data.currentVal;
    }

    bool should_trigger() {
        // Divider counts natural trigger steps. Divider 1 always accepts this stage;
        // higher dividers accept on modulo 1, reset on modulo 0, and skip the rest.
        triggerCount++;
        if (data.divider > 1) {
            const uint8_t mod = triggerCount % data.divider;
            if (mod == 0) {
                triggerCount = 0;
                _shouldTrigger = false;
                return false;
            }
            if (mod != 1) {
                _shouldTrigger = false;
                return false;
            }
        }

        // Probability is evaluated after the divider gate. A rejected trigger still
        // advances the divider state, but does not advance the random core.
        if ((rand() % 100) >= data.probability) {
            _shouldTrigger = false;
            return false;
        }

        slide_alpha = 0.0f;
        _shouldTrigger = true;
        return true;
    }

    void tick() {
        if (!currentCore) {
            return;
        }

        if (_shouldTrigger) {
            const int loopSteps = std::max<int>(1, data.loopSteps);
            if (data.isLooping == 0) {
                // No loop: promote next to current and generate a new next value.
                currentCore->tick();
                currentLoopIndex = data.loopSteps + 1;
            } else {
                // Loop modes walk backward through the saved sequence, matching the
                // hardware index order where index 0 is the prepared next value.
                currentLoopIndex--;
                if (currentLoopIndex <= 0) {
                    currentLoopIndex = loopSteps;
                }
                nextLoopIndex = currentLoopIndex - 1;
                if (data.isLooping == 2) {
                    // Slow loop evolves one stored value at the loop boundary. Longer
                    // loops evolve more readily, following the hardware threshold.
                    if ((rand() % 10 + 1) >= (data.loopSteps > 8 ? data.evolve : data.evolve + 2) && (currentLoopIndex == 1)) {
                        currentCore->generateAtIndex(static_cast<uint8_t>(rand() % (loopSteps + 1)));
                    }
                }
                currentCore->setCurrentValueFromIndex(static_cast<uint8_t>(currentLoopIndex));
                currentCore->setNextValueFromIndex(static_cast<uint8_t>(nextLoopIndex));
            }

            data.currentVal = currentCore->getCurrentValue();
            data.nextVal = currentCore->getNextValue();
        }
    }

    uint16_t process_val(uint16_t val, float deltaSeconds) {
        if (currentCore) {
            currentCore->advance(deltaSeconds);
        }
        const uint16_t processed = att_off_value(val);
        if (data.scale == 0) {
            return slide_value(processed, deltaSeconds);
        }
        return slide_value(get_calibrated_value(quantize_scale_value(processed)), deltaSeconds);
    }

  private:
    int nextLoopIndex = 16;
    int currentLoopIndex = 17;
    // uint16_t processed_val = 0;
    uint8_t triggerCount = 0;
    bool _shouldTrigger = true;
    long double slide_alpha = 0.0L;
    int64_t slided_val = 0;

    befaco::Quantizer quantizer_;
    std::unique_ptr<Random> currentCore;

    uint16_t slide_value(uint16_t val, float deltaSeconds) {
        if (data.slide <= 3) {
            slided_val = val;
            slide_alpha = 1.0L;
            return val;
        }

        if (slide_alpha < 1.0L) {
            slide_alpha += static_cast<long double>(deltaSeconds) * hardware_slide_rate(data.slide);
            if (slided_val < val) {
                slided_val = static_cast<int64_t>(static_cast<long double>(slided_val) +
                                                  slide_alpha * static_cast<long double>(val - slided_val));
            } else {
                slided_val = static_cast<int64_t>(static_cast<long double>(slided_val) -
                                                  slide_alpha * static_cast<long double>(slided_val - val));
            }
            return clamp_u16(static_cast<int32_t>(slided_val));
        }
        slided_val = val;
        return val;
    }

    static long double hardware_slide_rate(uint16_t slide) {
        if (slide <= 250) {
            return std::pow(1000.0L - static_cast<long double>(slide - 10), 2.2L) / 1000.0L / 100.0L;
        }
        if (slide <= 750) {
            return std::pow(1000.0L - static_cast<long double>(slide - 10), 2.5L) / 1000.0L / 10000.0L;
        }
        if (slide <= 875) {
            return std::pow(1000.0L - static_cast<long double>(slide - 10), 2.8L) / 1000.0L / 10000.0L;
        }
        return std::pow(1040.0L - static_cast<long double>(slide - 10), 3.0L) / 1000.0L / 100000.0L;
    }

    uint16_t att_off_value(uint16_t val) {
        const float scaled = static_cast<float>(val) * (static_cast<float>(data.attenuation) / SIXTEEN_BIT_MAX);
        const int32_t out = static_cast<int32_t>(std::round(scaled + data.atOffset));
        return clamp_u16(out);
    }

    befaco::QuantizeResponse quantize_scale_value(uint16_t val) {
        const uint8_t octave = static_cast<uint8_t>(val / befaco::QUANT_OCTAVE_RANGE);
        const float semitone =
            remap_float_clamp(static_cast<float>(val), static_cast<float>(befaco::QUANT_OCTAVE_RANGE * octave),
                              static_cast<float>(befaco::QUANT_OCTAVE_RANGE * (octave + 1)), 0.0f, 409.5f);
        befaco::QuantizeResponse quantized = quantizer_.Quantize_simple(semitone);
        quantized.Octave = octave;
        return quantized;
    }

    uint16_t get_calibrated_value(const befaco::QuantizeResponse &quant) {
        const float octaveFraction = quant.Value / 409.5f;
        const float octaveBase = static_cast<float>(befaco::QUANT_OCTAVE_RANGE) * quant.Octave;
        const float value = octaveBase + octaveFraction * befaco::QUANT_OCTAVE_RANGE;
        return clamp_u16(static_cast<int32_t>(std::round(value)));
    }
};

} // namespace R8
