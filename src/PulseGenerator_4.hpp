#pragma once
#include <rack.hpp>


/** When triggered, holds a high value for a specified time before going low again */
struct PulseGenerator_4 {
	simd::float_4 remaining = 0.f;

	/** Immediately disables the pulse */
	void reset() {
		remaining = 0.f;
	}

	/** Advances the state by `deltaTime`. Returns whether the pulse is in the HIGH state. */
	simd::float_4 process(float deltaTime) {

		simd::float_4 mask = (remaining > 0.f);

		remaining -= ifelse(mask, deltaTime, 0.f);
		return ifelse(mask, simd::float_4::mask(), 0.f);
	}

	/** Begins a trigger with the given `duration`. */
	void trigger(simd::float_4 mask, float duration = 1e-3f) {
		// Keep the previous pulse if the existing pulse will be held longer than the currently requested one.
		remaining = ifelse(mask & (duration > remaining), duration, remaining);
	}
};
