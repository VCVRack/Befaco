// Random8
// Copyright (c) 2024 Befaco / VanTa
// Open-source software
// Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported
// See LICENSE.txt for the complete license text

#pragma once
#include <iostream>
//#include <stdlib.h>

#include "../r8_random_style_base.h"
namespace R8{
    
    class LowHigh : public Random {
        public:
        uint8_t step = 0;

        LowHigh() {}
        ~LowHigh() override {}

        void setup(RandomInfo _info) override {
            info = _info; // setRandomInfo(_info);
            CreateSequence();
            setCurrentValueFromIndex(0);
        }

        uint16_t generate() override {
          step++;
          step = step % 2;
          // Match intended hardware behavior: alternate between low and high halves
          // of the range (roughly 0-5V then 5-10V), while still covering full 0-10V.
          if (step == 0) {
              // Low half [0, 32767]
              return static_cast<uint16_t>(rand() & 0x7FFF);
          }
          // High half [32768, 65535]
          return static_cast<uint16_t>(0x8000u | (rand() & 0x7FFF));
        }

        RandomLoop CreateSequence() override {
            //TODO: check values clamping here
            for (int i = 0; i < randLoop.size; i++)
            {
                randLoop.randValues.pop_back();
                randLoop.randValues.insert(randLoop.randValues.begin(), generate());
            }
            return randLoop;
        }
            
    };
}
