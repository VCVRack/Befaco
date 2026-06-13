// Random8
// Copyright (c) 2024 Befaco / VanTa
// Open-source software
// Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported
// See LICENSE.txt for the complete license text

#pragma once
#include <iostream>
#include <stdio.h>
#include <random>

#include "../r8_random_style_base.h"
namespace R8{
    //example implementation of a random core
    class NormalDistribution : public Random {
        public:

        NormalDistribution() {
            // rd = new std::random_device{};
            // gen = gen { rd(); };
        }
        ~NormalDistribution() override {}

        void setup(RandomInfo _info) override {
            info = _info; // setRandomInfo(_info);
            generator.seed(info.seed);
            //distribution(0.0, 1.0);
            CreateSequence();
            setCurrentValueFromIndex(0);
        }

        // uint16_t generate() override { return remap(distribution(generator), -3, 3, 0, 65535); }
        uint16_t generate() override { return normal_dist(); }

        RandomLoop CreateSequence() override {
            for (int i = 0; i < randLoop.size; i++)
            {
                randLoop.randValues.pop_back();
                randLoop.randValues.insert(randLoop.randValues.begin(), generate());
            }
            return randLoop;
        }

        uint16_t normal_dist(){
            static std::mt19937 gen(info.seed);
            static std::normal_distribution<double> dist(32767.5, 8191); //16383.75
            return dist(gen);
        }

        private:
            // std::random_device rd;
            // std::mt19937 gen;
            std::default_random_engine generator;          // (info.seed);
            std::normal_distribution<double> distribution; //(0.0, 1.0);
            
    };
}
