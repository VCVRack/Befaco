#pragma once

#include "rack.hpp"


/** When triggered, holds a high value for a specified time before going low again */
struct PulseGenerator_4 {
    
    simd::float_4 remaining = simd::float_4::zero();

    /** Immediately disables the pulse */
    void reset() {
            remaining = simd::float_4::zero();
    }

    /** Advances the state by `deltaTime`. Returns whether the pulse is in the HIGH state. */
    inline simd::float_4 process(float deltaTime) {

        simd::float_4 mask = (remaining > simd::float_4::zero());

        remaining -= ifelse(mask, simd::float_4(deltaTime), simd::float_4::zero());
        return ifelse(mask, simd::float_4::mask(), simd::float_4::zero());
    }

    /** Begins a trigger with the given `duration`. */
    inline void trigger(simd::float_4 mask, float duration = 1e-3f) {
        // Keep the previous pulse if the existing pulse will be held longer than the currently requested one.
        simd::float_4 duration_4 = simd::float_4(duration);
        remaining = ifelse( mask&(duration_4>remaining), duration_4, remaining); 
    }
};

