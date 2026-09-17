#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <array>
#include <vector>

#include "r8_utils_vcv.h"

namespace R8 {

// abstract base class for each Random algorithm to derive from
class Random {
  public:
    Random() {
        if (randLoop.randValues.size() != randLoop.size + 1) {
            randLoop.randValues.resize(randLoop.size + 1);
        }
    }
    virtual ~Random() {}

    virtual void setup(RandomInfo _info) { info = _info; }
    // Recreate/reset style internals when style changes at runtime.
    // Default to setup() so derived styles that only override setup()
    // are fully initialized.
    virtual void create(RandomInfo _info) { setup(_info); }
    virtual void advance(float /*deltaSeconds*/) {}
    virtual uint16_t generate() = 0;
    virtual RandomLoop& CreateSequence() = 0;

    virtual uint16_t getNextValue() { return nextValue; }
    virtual uint16_t getCurrentValue() { return currentValue; }

    void resetForSetup() {
        std::fill(randLoop.randValues.begin(), randLoop.randValues.end(), 0);
        currentValue = 0;
        nextValue = 0;
    }

    virtual void generateIntoNextValue() { nextValue = generate(); }

    virtual void generateAtIndex(uint8_t index) { randLoop.randValues[index] = generate(); }

    virtual void setCurrentValueFromIndex(uint8_t index) { currentValue = randLoop.randValues[index]; }

    virtual void setNextValueFromIndex(uint8_t index) { nextValue = randLoop.randValues[index]; }

    virtual void tick() {
        currentValue = randLoop.randValues.front();
        for (std::size_t i = randLoop.randValues.size() - 1; i > 0; --i) {
            randLoop.randValues[i] = randLoop.randValues[i - 1];
        }
        randLoop.randValues.front() = generate();
        nextValue = randLoop.randValues.front();
    }

    std::array<uint16_t, RANDOM_SEQUENCE_SIZE> getSequence() const {
        std::array<uint16_t, RANDOM_SEQUENCE_SIZE> sequence{};
        std::copy(randLoop.randValues.begin(), randLoop.randValues.end(), sequence.begin());
        return sequence;
    }

    void setSequence(const std::array<uint16_t, RANDOM_SEQUENCE_SIZE> &seq) {
        std::copy(seq.begin(), seq.end(), randLoop.randValues.begin());
    }

    void setCurrentValue(uint16_t value) { currentValue = value; }

    void setNextValue(uint16_t value) { nextValue = value; }

  protected:
    uint16_t nextValue = 0;
    uint16_t currentValue = 0;
    RandomLoop randLoop;
    RandomInfo info{};
};

} // namespace R8
