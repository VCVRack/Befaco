// Random8
// Copyright (c) 2024 Befaco / VanTa
// Open-source software
// Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported
// See LICENSE.txt for the complete license text

#pragma once
#include <iostream>
#include <stdio.h>
#include <cmath>
#include "../r8_random_style_base.h"
namespace R8{
    //example implementation of a random core
    class RoscRandom : public Random {
        public:
        uint32_t prngState = 0;
        float bias = 0.5f;
        uint8_t lastBit = 0;

        RoscRandom() {}
        ~RoscRandom() override {}

        void setup(RandomInfo _info) override {
            info = _info; // setRandomInfo(_info);
            // deterministic-per-seed init, but evolves thereafter
            prngState = 0x9E3779B9u ^ (static_cast<uint32_t>(info.seed) * 0x85EBCA6Bu + 0xC2B2AE35u);
            bias = 0.45f + (static_cast<float>(info.seed) / 255.0f) * 0.10f; // 0.45 .. 0.55
            lastBit = static_cast<uint8_t>(info.seed & 1u);
            CreateSequence();
            setCurrentValueFromIndex(0);
        }

        uint16_t generate() override { return rnd_whitened(info.seed); }

        RandomLoop CreateSequence() override {
            for (int i = 0; i < randLoop.size; i++)
            {
                randLoop.randValues.pop_back();
                randLoop.randValues.insert(randLoop.randValues.begin(), generate());
            }
            return randLoop;
        }


        protected:

            uint16_t rnd(int seed = 0) {
                (void)seed;
                // xorshift32
                prngState ^= prngState << 13;
                prngState ^= prngState >> 17;
                prngState ^= prngState << 5;
                return static_cast<uint16_t>(prngState & 0xffffu);
            }

            // Von Neumann extractor: From the input stream, his extractor took bits, two at a time (first and second, then third and fourth, and so on).
            // If the two bits matched, no output was generated. If the bits differed, the value of the first bit was output. 
            // https://people.ece.cornell.edu/land/courses/ece4760/RP2040/C_SDK_random/index_random.html
            uint8_t rosc_raw_bit() {
                // emulate slightly biased + correlated hardware bit stream
                const float corr = 0.12f;
                const float p = bias + (lastBit ? corr : -corr);
                const float threshold = std::fmin(0.98f, std::fmax(0.02f, p));
                const float u = static_cast<float>(rnd()) / 65535.0f;
                uint8_t b = (u < threshold) ? 1u : 0u;
                lastBit = b;

                // slow bias drift
                const float drift = (static_cast<float>(rnd()) / 65535.0f - 0.5f) * 0.004f;
                bias = std::fmin(0.60f, std::fmax(0.40f, bias + drift));
                return b;
            }

            uint16_t rnd_whitened(int seed = 0) {
                (void)seed;
                uint16_t out = 0;
                int k = 0;
                int guard = 0;
                while (k < 16 && guard < 4096) {
                    const uint8_t b1 = rosc_raw_bit();
                    const uint8_t b2 = rosc_raw_bit();
                    guard++;
                    if (b1 == b2) {
                        continue;
                    }
                    out = static_cast<uint16_t>((out << 1) | b1);
                    k++;
                }
                return out;
            }


            void set_seed_random_from_rosc() {
                info.seed = static_cast<uint8_t>(rand() & 0xff);
            }
        };
}
