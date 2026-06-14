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
    
    class EMCore : public Random {
        public:

        EMCore() {}
        ~EMCore() override {}

        void setup(RandomInfo _info) override {
            info = _info; // setRandomInfo(_info);

            // 12-bit conversion, assume max value == ADC_VREF == 3.3 V
            //conversion_factor = 3.3f / (1 << 12);

            // adc_init();
            // adc_run(true); //free running conversion

            // // Make sure GPIO is high-impedance, no pullups etc
            // adc_gpio_init(26);
            // adc_select_input(0); //0-3 are GPIO 26-29

            CreateSequence();
            setCurrentValueFromIndex(0);
        }

        uint16_t generate() override { return read_EM_antenna(info.seed); }

        RandomLoop CreateSequence() override {
            for (int i = 0; i < randLoop.size; i++)
            {
                randLoop.randValues.pop_back();
                randLoop.randValues.insert(randLoop.randValues.begin(), generate());
            }
            return randLoop;
        }

        uint16_t read_EM_antenna(int seed = 0) {
            (void)seed;
            return static_cast<uint16_t>(rand());
        }

        private:
        
        //float conversion_factor;
            
    };
}


//gpio_deinit();
