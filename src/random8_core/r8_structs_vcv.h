#pragma once

#include <cstdint>
#include <vector>

namespace R8 {

constexpr int NUM_CHANNELS = 8;
constexpr uint16_t TWELVE_BIT_MAX = 4095;
constexpr uint16_t SIXTEEN_BIT_MAX = 65535;
constexpr uint32_t THIRTYTWO_BIT_MAX = 4294967295u;

constexpr int NUM_AVAILABLE_SCALES = 16;
constexpr int NUM_AVAILABLE_STYLES = 8;

// output from a random core
struct RandomLoop {
    std::vector<uint16_t> randValues;
    uint8_t size = 32;
};

// random8 core styles
enum RandomStyle {
    // Hardware menu order (active styles)
    Standard,
    Rosc,
    Gamma,
    Expo,
    Weibull,
    Low_High,
    FBM,
    Perlin,

    // Additional legacy/experimental styles kept for compatibility
    Test,
    Normal,
    Pink,
    EM,
    VCO_Pseudo,
    Lorentz,
    Mers,
    Calibration,
    Binomial
};

// input to a random core
struct RandomInfo {
    RandomStyle style;
    uint8_t seed;
};

struct uint12 {
    unsigned x : 12;
};

enum CalibrationTypes {
    Bypass = 0,
    LeastSquares,
    LinearIntervals,
    Polynomial,
    TwoPoints,
    PerOctave
};

} // namespace R8
