#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
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
    virtual RandomLoop CreateSequence() = 0;

    virtual uint16_t getNextValue() { return nextValue; }
    virtual uint16_t getCurrentValue() { return currentValue; }

    virtual void generateIntoNextValue() { nextValue = generate(); }

    virtual void generateAtIndex(uint8_t index) { randLoop.randValues[index] = generate(); }

    virtual void setCurrentValueFromIndex(uint8_t index) { currentValue = randLoop.randValues[index]; }

    virtual void setNextValueFromIndex(uint8_t index) { nextValue = randLoop.randValues[index]; }

    virtual void tick() {
        currentValue = randLoop.randValues.front();
        randLoop.randValues.insert(randLoop.randValues.begin(), generate());
        randLoop.randValues.pop_back();
        nextValue = randLoop.randValues.front();
    }

    std::vector<uint16_t> getSequence() const { return randLoop.randValues; }

    void setSequence(const std::vector<uint16_t> &seq) {
        randLoop.randValues = seq;
        const std::size_t sequenceSize = static_cast<std::size_t>(randLoop.size) + 1;
        if (randLoop.randValues.size() < sequenceSize) {
            randLoop.randValues.resize(sequenceSize, 0);
        } else if (randLoop.randValues.size() > sequenceSize) {
            randLoop.randValues.resize(sequenceSize);
        }
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
