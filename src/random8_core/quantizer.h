#pragma once

#include "quantizer_scales.h"
#include <cstdint>

namespace befaco {

constexpr int NUM_OCTAVES = 10;
constexpr int QUANT_OCTAVE_RANGE = (UINT16_MAX / NUM_OCTAVES);

struct QuantizeResponse {
    uint8_t Note = 0;
    uint8_t Octave = 0;
    float Value = 0.0f;
};

class Quantizer {
  public:
    Quantizer();
    ~Quantizer();

    QuantizeResponse Quantize_simple(float in);
    void SetScale(uint8_t scale);
    float GetValueForNote(int note);

  private:
    int _scale = 0;
    int _octave = 0;
    int _index = 0;
    int _nudge = 0;
    int _above = 0;
    int _below = 0;
    int _current = 0;
    QuantizeResponse _last{};
    int _temp = 0;
};

} // namespace befaco
