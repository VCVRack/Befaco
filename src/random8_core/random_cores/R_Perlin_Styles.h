// Random8
// Copyright (c) 2024 Befaco / VanTa
// Open-source software
// Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported
// See LICENSE.txt for the complete license text

#pragma once
#include <iostream>
#include "perlin.h"

#include "../r8_random_style_base.h"
namespace R8{

    class PerlinStyle : public Random {
        public:

        uint16_t positionX;

        PerlinStyle() {}
        ~PerlinStyle() override {}


        void setup(RandomInfo _info) override {
            info = _info; // setRandomInfo(_info);
            positionX = info.seed;
            CreateSequence();
            setCurrentValueFromIndex(0);
        }

        uint16_t generate() override {
            positionX = (positionX + 1) % SIXTEEN_BIT_MAX;
            float pos = (float)positionX / (float)SIXTEEN_BIT_MAX;
            return (uint16_t)(perlin1d(pos, 920.666, 2) * SIXTEEN_BIT_MAX);
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

    class FBMStyle : public Random {
        public:

        uint16_t positionX;

        FBMStyle() {}
        ~FBMStyle() override {}


        void setup(RandomInfo _info) override {
            info = _info; // setRandomInfo(_info);
            positionX = info.seed;
            CreateSequence();
            setCurrentValueFromIndex(0);
        }

        uint16_t generate() override {
            positionX = (positionX + 1) % SIXTEEN_BIT_MAX;
            float pos = (float)positionX / (float)SIXTEEN_BIT_MAX;
            return (uint16_t)(perlin1d(pos, 720.1459, 10) * SIXTEEN_BIT_MAX);
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
