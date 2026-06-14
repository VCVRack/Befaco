// Random8
// Copyright (c) 2024 Befaco / VanTa
// Open-source software
// Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported
// See LICENSE.txt for the complete license text

// based on c++11 std library: https://en.cppreference.com/w/cpp/named_req/RandomNumberDistribution

#pragma once
#include <cstdint>
#include <iostream>
#include <random>
#include <stdio.h>

#include "../r8_random_style_base.h"
namespace R8 {

static inline uint16_t clamp_u16_float(float x) {
    if (x <= 0.0f) {
        return 0;
    }
    if (x >= 65535.0f) {
        return 65535;
    }
    return static_cast<uint16_t>(x);
}

class NormalDist : public Random {
  public:
    NormalDist(float mean, float dev) {
        _mean = mean;
        _stddev = dev;
    }
    ~NormalDist() override {}

    void setup(RandomInfo _info) override {
        info = _info;
        mt_.seed(info.seed);
        CreateSequence();
        setCurrentValueFromIndex(0);
    }

    template <class T>
    T GetRand(T lower, float upper) {
        std::normal_distribution<T> dist(lower, upper);
        return dist(mt_);
    }

    uint16_t generate() override { return clamp_u16_float(normal_dist() * 32000.0f); }

    RandomLoop CreateSequence() override {
        for (int i = 0; i < randLoop.size; i++) {
            randLoop.randValues.pop_back();
            randLoop.randValues.insert(randLoop.randValues.begin(), generate());
        }
        return randLoop;
    }

    uint16_t normal_dist() { return static_cast<uint16_t>(GetRand<float>(_mean, _stddev)); }

  private:
    float _mean;
    float _stddev;
    std::mt19937 mt_{0};
};

//===============================================================//
// https://en.wikipedia.org/wiki/Binomial_distribution
class BinomialDist : public Random {
  public:
    BinomialDist() {}
    ~BinomialDist() override {}

    void setup(RandomInfo _info) override {
        info = _info;
        mt_.seed(info.seed);
        CreateSequence();
        setCurrentValueFromIndex(0);
    }

    template <class T>
    T GetRand(T trials, float probability) {
        std::binomial_distribution<T> dist(trials, probability);
        return dist(mt_);
    }

    uint16_t generate() override { return binomial_dist(); }

    RandomLoop CreateSequence() override {
        for (int i = 0; i < randLoop.size; i++) {
            randLoop.randValues.pop_back();
            randLoop.randValues.insert(randLoop.randValues.begin(), generate());
        }
        return randLoop;
    }

    uint16_t binomial_dist() { return static_cast<uint16_t>(GetRand<uint16_t>(65535, 0.5f)); }

  private:
    std::mt19937 mt_{0};
};

//===============================================================//
// https://en.wikipedia.org/wiki/Cauchy_distribution
class LorentzDist : public Random {
  public:
    LorentzDist() {}
    ~LorentzDist() override {}

    void setup(RandomInfo _info) override {
        info = _info;
        mt_.seed(info.seed);
        CreateSequence();
        setCurrentValueFromIndex(0);
    }

    template <class T>
    T GetRand(T lower, T upper) {
        std::cauchy_distribution<T> dist(lower, upper);
        return dist(mt_);
    }

    uint16_t generate() override { return clamp_u16_float(lorentz_dist() * 65535.0f); }

    RandomLoop CreateSequence() override {
        for (int i = 0; i < randLoop.size; i++) {
            randLoop.randValues.pop_back();
            randLoop.randValues.insert(randLoop.randValues.begin(), generate());
        }
        return randLoop;
    }

    float lorentz_dist() { return GetRand<float>(32767.5f, 0.5f); }

  private:
    std::mt19937 mt_{0};
};

//===============================================================//
class Mersenne : public Random {
  public:
    Mersenne() {}
    ~Mersenne() override {}

    template <class T>
    T GetRand(T lower, T upper) {
        std::uniform_real_distribution<T> dist(lower, upper);
        return dist(mt_);
    }

    void setup(RandomInfo _info) override {
        info = _info;
        mt_.seed(info.seed);
        CreateSequence();
        setCurrentValueFromIndex(0);
    }

    uint16_t generate() override { return mersenne_dist(); }

    RandomLoop CreateSequence() override {
        for (int i = 0; i < randLoop.size; i++) {
            randLoop.randValues.pop_back();
            randLoop.randValues.insert(randLoop.randValues.begin(), generate());
        }
        return randLoop;
    }

    uint16_t mersenne_dist() { return static_cast<uint16_t>(GetRand<float>(0.0f, 1.0f) * 65535); }

  private:
    std::mt19937 mt_;
};

//===============================================================//
// https://en.wikipedia.org/wiki/Gamma_distribution
class GammaDist : public Random {
  public:
    GammaDist(float shape, float rate) {
        _shape = shape;
        _rate = rate;
    }
    ~GammaDist() override {}

    void setup(RandomInfo _info) override {
        info = _info;
        mt_.seed(info.seed);
        CreateSequence();
        setCurrentValueFromIndex(0);
    }

    template <class T>
    T GetRand(T lower, float upper) {
        std::gamma_distribution<T> dist(lower, upper);
        return dist(mt_);
    }

    uint16_t generate() override { return gamma_dist(); }

    RandomLoop CreateSequence() override {
        for (int i = 0; i < randLoop.size; i++) {
            randLoop.randValues.pop_back();
            randLoop.randValues.insert(randLoop.randValues.begin(), generate());
        }
        return randLoop;
    }

    uint16_t gamma_dist() {
        // Rejection sample into [0, 1] normalized range to avoid hard clipping pileups at 10V.
        // This preserves the intended distribution shape in-range for histogram/sonic behavior.
        constexpr int kMaxAttempts = 64;
        float y = 0.0f;
        for (int i = 0; i < kMaxAttempts; i++) {
            y = GetRand<float>(_shape, _rate);
            if (y >= 0.0f && y <= 1.0f) {
                return clamp_u16_float(y * 65535.0f);
            }
        }
        // Fallback after bounded retries.
        if (y < 0.0f) {
            y = 0.0f;
        } else if (y > 1.0f) {
            y = 1.0f;
        }
        return clamp_u16_float(y * 65535.0f);
    }

  private:
    float _shape;
    float _rate;
    std::mt19937 mt_{0};
};

//===============================================================//
// https://en.wikipedia.org/wiki/Weibull_distribution
class WeibullDist : public Random {
  public:
    WeibullDist(float shape, float scale) {
        _shape = shape;
        _scale = scale;
    }
    ~WeibullDist() override {}

    void setup(RandomInfo _info) override {
        info = _info;
        mt_.seed(info.seed);
        CreateSequence();
        setCurrentValueFromIndex(0);
    }

    template <class T>
    T GetRand(T lower, float upper) {
        std::weibull_distribution<T> dist(lower, upper);
        return dist(mt_);
    }

    uint16_t generate() override { return weibull_dist(); }

    RandomLoop CreateSequence() override {
        for (int i = 0; i < randLoop.size; i++) {
            randLoop.randValues.pop_back();
            randLoop.randValues.insert(randLoop.randValues.begin(), generate());
        }
        return randLoop;
    }

    uint16_t weibull_dist() {
        // Rejection sample into [0, 1] normalized range to avoid hard clipping pileups at 10V.
        constexpr int kMaxAttempts = 64;
        float y = 0.0f;
        for (int i = 0; i < kMaxAttempts; i++) {
            y = GetRand<float>(_shape, _scale);
            if (y >= 0.0f && y <= 1.0f) {
                return clamp_u16_float(y * 65535.0f);
            }
        }
        // Fallback after bounded retries.
        if (y < 0.0f) {
            y = 0.0f;
        } else if (y > 1.0f) {
            y = 1.0f;
        }
        return clamp_u16_float(y * 65535.0f);
    }

  private:
    float _shape;
    float _scale;
    std::mt19937 mt_{0};
};

} // namespace R8
