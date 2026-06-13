// Random8
// Copyright (c) 2024 Befaco / VanTa
// Open-source software
// Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported
// See LICENSE.txt for the complete license text

#pragma once
#include <iostream>
//#include <stdlib.h>
#include <cmath>

#include "../r8_random_style_base.h"
namespace R8{
    
    class VCO : public Random {
        public:
        static constexpr float FREQUENCY_HZ = 200.0f; // 5ms period, matches hardware intent
        static constexpr uint16_t TABLE_SIZE = 5000;  // 1 sample per microsecond in one 5ms cycle
        uint16_t wavetable[TABLE_SIZE];
        uint8_t wave_type = 0; // 0 is sine, 1 is sawtooth ...
        float phase = 0.0f;    // normalized [0, 1)

        VCO() {}
        ~VCO() override {}

        void setup(RandomInfo _info) override {
            info = _info; // setRandomInfo(_info);
            for (int i = 0; i < TABLE_SIZE; i++) {
                if (wave_type == 0)
                {
                    wavetable[i] = static_cast<uint16_t>(32767.0f * std::sin((float)i * 6.283185307f / (float)TABLE_SIZE) + 32767.0f);
                } else if (wave_type == 1) {
                    wavetable[i] = static_cast<uint16_t>(SIXTEEN_BIT_MAX * ((float)i / (float)TABLE_SIZE));
                }
            }
            phase = (static_cast<float>(info.seed) / 255.0f);
            CreateSequence();
            setCurrentValueFromIndex(0);
        }

        void advance(float deltaSeconds) override {
            phase += FREQUENCY_HZ * deltaSeconds;
            phase -= std::floor(phase);
        }

        uint16_t generate() override {
          uint16_t idx = static_cast<uint16_t>(phase * (TABLE_SIZE - 1));
          if (idx >= TABLE_SIZE) {
              idx = TABLE_SIZE - 1;
          }
          return wavetable[idx];
        }

        RandomLoop CreateSequence() override {
            for (int i = 0; i < randLoop.size; i++) {
                const uint16_t idx = static_cast<uint16_t>((i / static_cast<float>(randLoop.size)) * (TABLE_SIZE - 1));
                randLoop.randValues.pop_back();
                randLoop.randValues.insert(randLoop.randValues.begin(), wavetable[idx]);
            }
            return randLoop;
        }
            
    };
}
