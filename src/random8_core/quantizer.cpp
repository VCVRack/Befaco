#include "quantizer.h"

using namespace befaco;

Quantizer::Quantizer() { SetScale(1); }

Quantizer::~Quantizer() {}

void Quantizer::SetScale(uint8_t scale) {
    if (scale < scaleCount) {
        _scale = scale;
    }
}

float Quantizer::GetValueForNote(int note) {
    if (note < scales[_scale].notes_count) {
        return scales[_scale].notes[note];
    }
    return 0.0f;
}

QuantizeResponse Quantizer::Quantize_simple(float in) {
    QuantizeResponse response;
    if (scales[_scale].num_notes == 0) {
        response.Value = in;
        return response;
    }

    float range = 409.5f / static_cast<float>(scales[_scale].num_notes);
    for (uint8_t i = 0; i < scales[_scale].num_notes + 1; i++) {
        if (in < (range * i + range)) {
            response.Value = scales[_scale].notes[i];
            response.Note = i;
            return response;
        }
    }
    return response;
}
