#pragma once

#include <cassert>
#include <cmath>
#include <cstdint>

#include "r8_structs_vcv.h"

namespace R8 {

inline uint16_t remap(uint16_t value, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max) {
    return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

inline float remap_float(float value, float in_min, float in_max, float out_min, float out_max) {
    return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

inline float remap_float_clamp(float value, float in_min, float in_max, float out_min, float out_max) {
    return std::fmax(std::fmin(remap_float(value, in_min, in_max, out_min, out_max), out_max), out_min);
}

inline uint16_t sixteen_to_twelve(uint16_t val) { return static_cast<uint16_t>(std::round(val / 16.0)); }

inline uint16_t twelve_to_sixteen(uint16_t val) { return static_cast<uint16_t>(std::round(val * 16.0)); }

inline uint16_t scale_by(uint16_t val, uint16_t factor) {
    return static_cast<uint16_t>(std::round(val * factor / static_cast<double>(SIXTEEN_BIT_MAX)));
}

inline uint8_t scale_by(uint8_t val, uint8_t factor) { return static_cast<uint8_t>(std::round(val * factor / 255.0)); }

inline uint16_t mapResolution(uint16_t value, uint16_t from, uint16_t to) {
    if (from != to) {
        if (from > to) {
            value = (value < static_cast<uint16_t>(1 << (from - to))) ? 0 : ((value + 1) >> (from - to)) - 1;
        } else {
            if (value != 0) {
                value = ((value + 1) << (to - from)) - 1;
            }
        }
    }
    return value;
}

inline uint16_t clamp_u16(int32_t value) {
    if (value < 0) {
        return 0;
    }
    if (value > static_cast<int32_t>(SIXTEEN_BIT_MAX)) {
        return SIXTEEN_BIT_MAX;
    }
    return static_cast<uint16_t>(value);
}

} // namespace R8
