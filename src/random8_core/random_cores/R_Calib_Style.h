// Random8
// Copyright (c) 2024 Befaco / VanTa
// Open-source software
// Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported
// See LICENSE.txt for the complete license text

#pragma once
#include <iostream>
//#include <stdlib.h>

#include "../r8_random_style_base.h"
#include "../quantizer.h"
namespace R8{
    //example implementation of a random style
    class CalibRandomStyle : public Random {
        public:

        CalibRandomStyle() {}
        ~CalibRandomStyle() override {}

        uint16_t count;

        void setup(RandomInfo _info) override {
            // Random::setup(_info); // calls base class' function
            info = _info; // setRandomInfo(_info);
            CreateSequence();
            setCurrentValueFromIndex(0);
        }

        uint16_t generate() override {
            ++count;
            count = count % (12 * befaco::QUANT_OCTAVE_RANGE); // SIXTEEN_BIT_MAX;
            return count * (befaco::QUANT_OCTAVE_RANGE / 12);
        }

        RandomLoop CreateSequence() override {
            for (int i = 0; i < randLoop.size; i++)
            {
                randLoop.randValues.pop_back();
                randLoop.randValues.insert(randLoop.randValues.begin(), generate());
            }
            return randLoop;
        }
            
    };
}
