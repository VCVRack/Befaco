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
    
    class StandardRandom : public Random {
        public:

        StandardRandom() {}
        ~StandardRandom() override {}

        void setup(RandomInfo _info) override {
            info = _info; // setRandomInfo(_info);
            CreateSequence();
            setCurrentValueFromIndex(0);
        }

        uint16_t generate() override { return rand(); }

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
