// Random8
// Copyright (c) 2024 Befaco / VanTa
// Open-source software
// Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported
// See LICENSE.txt for the complete license text
// Technique by Larry "RidgeRat" Trammell 3/2006
// http://home.earthlink.net/~ltrammell/tech/pinkalg.htm
// implementation and optimization by David Lowenfels

#pragma once
#include <iostream>
#include <stdio.h>
#include <random>

#include "../r8_random_style_base.h"
namespace R8{

    // idea for gaussian white:
    // #define PI 3.1415926536f;
    // float R1 = (float) rand() / (float) RAND_MAX;
    // float R2 = (float) rand() / (float) RAND_MAX;
    // float X = (float) sqrt( -2.0f * log( R1 )) * cos( 2.0f * PI * R2 );

    #define PINK_NOISE_NUM_STAGES 3

    class PinkStyle : public Random {
        public:

        PinkStyle() {
            PinkNoise();
            // clear();
        }
        ~PinkStyle() override {}

        void setup(RandomInfo _info) override {
            info = _info; // setRandomInfo(_info);
            PinkNoise();
            CreateSequence();
            setCurrentValueFromIndex(0);

        }

        // uint16_t generate() override { return (uint16_t)(generatePink() * (double)65535); }
        uint16_t generate() override { return (uint16_t)(generatePink()); }

        RandomLoop CreateSequence() override {
            for (int i = 0; i < randLoop.size; i++)
            {
                randLoop.randValues.pop_back();
                randLoop.randValues.insert(randLoop.randValues.begin(), generate());
            }
            return randLoop;
        }

        private:
        // each row effectively holds an independent random number generator
        std::vector<float> pinkRows;
        // running sum for noise output
        float pinkRunSum;
        // the column index, incremented each sample
        int pinkIndex;
        // the row mask, which ensures that the index of the pinkRows vector is never exceeded
        int pinkIndexMask;
        // used to normalize the noise at the output
        float pinkNorm;

        // constructor, overload to initialize with 12 rows, which worked out to be a
        // good number when testing in Octave
        void PinkNoise(int numRows = 12) {
            pinkIndex = 0;
            // mask the index so it does not spill outside of the pinkRows vector range
            pinkIndexMask = (1 << numRows) - 1;
            // initialize normalization variable
            pinkNorm = 1.0 / (float)(numRows + 1);
            // in testing, I found it was better to initialize the rows with noise
            // this avoids a climb up to some max value during the first run through the rows
            for (int i = 0; i < numRows; i++)
                pinkRows.push_back(rand());
            pinkRunSum = rand();
        }

        float generatePink() {
            float newRandom, sum;

            // increment and mask index
            pinkIndex = (pinkIndex + 1) & pinkIndexMask;

            // ensure pink index is not zero, if it is, do not update any of the random vals
            if (pinkIndex != 0) {
                // determine the number of trailing zeros in pinkIndex
                int numZeros = 0;
                int n = pinkIndex;
                while ((n & 1) == 0) {
                    // bit shift until you run out of trailing zeros
                    n = n >> 1;
                    numZeros++;
                // }
                // McCARTNEY-VOSS ALGORITHM
                // subtract previous value from running sum
                pinkRunSum -= pinkRows[numZeros];
                // generate a new random number
                newRandom = rand();
                // add the new random number
                pinkRunSum += newRandom;
                // replace the row value at index numZeros with the new random value
                pinkRows[numZeros] = newRandom;
                }
            }

            // add extra white noise value
            sum = pinkRunSum + rand();

            // scale and return value
            return (sum * pinkNorm);
        }

        // https://github.com/bdejong/musicdsp/blob/master/source/Synthesis/220-trammell-pink-noise-c-class.rst
        // float state[ PINK_NOISE_NUM_STAGES ];
        // static const float A[ PINK_NOISE_NUM_STAGES ];
        // static const float P[ PINK_NOISE_NUM_STAGES ];
        //
        // void clear() {
        //     for( size_t i=0; i< PINK_NOISE_NUM_STAGES; i++ )
        //         state[ i ] = 0.0;
        // }
        //
        // float tick_pink() {
        //     static const float RMI2 = 2.0 / float(RAND_MAX);// + 1.0; // +1 is for range [0,1]??, otherwise [-1,1]
        //     static const float offset = A[0] + A[1] + A[2];
        //
        //     // unrolled loop
        //     float temp = float( rand() );
        //     state[0] = P[0] * (state[0] - temp) + temp;
        //     temp = float( rand() );
        //     state[1] = P[1] * (state[1] - temp) + temp;
        //     temp = float( rand() );
        //     state[2] = P[2] * (state[2] - temp) + temp;
        //     return ( A[0]*state[0] + A[1]*state[1] + A[2]*state[2] )*RMI2 - offset;
        // }
            
    };

    // inline constexpr float PinkStyle::A[] = {0.02109238f, 0.07113478f, 0.68873558f}; // rescaled by (1+P)/(1-P)
    // inline constexpr float PinkStyle::P[] = {0.3190f, 0.7756f, 0.9613f};


    class VossPink : public Random //https://www.firstpr.com.au/dsp/pink-noise/
    {
    private:
        int max_key;
        int key;
        unsigned int white_values[5];
        unsigned int range = 128;

      public:
        VossPink() {}
        ~VossPink() override {}

        void setup(RandomInfo _info) override {
            info = _info;
            max_key = 0x1f; // Five bits set
            key = 0;
            for (int i = 0; i < 5; i++){
              white_values[i] = rand() % (range / 5);
            }
            CreateSequence();
            setCurrentValueFromIndex(0);
        }

        int GetNextValue()
        {
            int last_key = key;
            unsigned int sum;

            key++;
            if (key > max_key)
                key = 0;
            // Exclusive-Or previous value with current value. This gives
            // a list of bits that have changed.
            int diff = last_key ^ key;
            sum = 0;
            for (int i = 0; i < 5; i++)
            {
                // If bit changed get new random number for corresponding
                // white_value
                if (diff & (1 << i))
                    white_values[i] = rand() % (range/5);
                sum += white_values[i];
            }
            return sum;
        }

        uint16_t generate() override {
            const int maxWhiteValue = (range / 5) - 1;
            const int maxSum = maxWhiteValue * 5;
            if (maxSum <= 0) {
                return 0;
            }
            const int sum = GetNextValue();
            const float normalized = static_cast<float>(sum) / static_cast<float>(maxSum);
            return static_cast<uint16_t>(std::round(normalized * static_cast<float>(SIXTEEN_BIT_MAX)));
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


    
} // namespace R8
