// Random8
// Copyright (c) 2024 Befaco / VanTa
// Open-source software
// Licensed under Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported
// See LICENSE.txt for the complete license text

#pragma once
#include <memory>

#include "R_Calib_Style.h"
#include "R_Color_Styles.h"
#include "R_EM_Style.h"
#include "R_Low-High_Style.h"
#include "R_Perlin_Styles.h"
#include "R_Rosc_Style.h"
#include "R_Standard_Style.h"
#include "R_Statistical_Styles.h"
#include "R_VCO_Style.h"

// include new cores here as a new case in the switch below,
// and after that add the cores to be used
// in the order to be accessed into:
// RandomStyle enum in r8_structs.h file
// and change the NUM_AVAILABLE_STYLES define
namespace R8 {

// created and moved by r8_channel
inline std::unique_ptr<Random> assign_style(RandomInfo info) {
    switch (info.style) {
    case RandomStyle::Standard: return std::unique_ptr<StandardRandom>(new StandardRandom);
    case RandomStyle::Normal: return std::unique_ptr<NormalDist>(new NormalDist(16383.75f, 8191.0f));
    case RandomStyle::Pink: return std::unique_ptr<PinkStyle>(new PinkStyle);
    case RandomStyle::Rosc: return std::unique_ptr<RoscRandom>(new RoscRandom);
    case RandomStyle::EM: return std::unique_ptr<EMCore>(new EMCore);
    case RandomStyle::FBM: return std::unique_ptr<FBMStyle>(new FBMStyle);
    case RandomStyle::Low_High: return std::unique_ptr<LowHigh>(new LowHigh);
    case RandomStyle::VCO_Pseudo: return std::unique_ptr<VCO>(new VCO);
    case RandomStyle::Perlin: return std::unique_ptr<PerlinStyle>(new PerlinStyle);
    case RandomStyle::Expo: return std::unique_ptr<GammaDist>(new GammaDist(1.0f, 2.0f));
    case RandomStyle::Lorentz: return std::unique_ptr<LorentzDist>(new LorentzDist);
    case RandomStyle::Mers: return std::unique_ptr<Mersenne>(new Mersenne);
    case RandomStyle::Gamma: return std::unique_ptr<GammaDist>(new GammaDist(2.0f, 2.0f));
    case RandomStyle::Weibull: return std::unique_ptr<WeibullDist>(new WeibullDist(1.0f, 1.5f));
    case RandomStyle::Calibration: return std::unique_ptr<CalibRandomStyle>(new CalibRandomStyle);
    case RandomStyle::Binomial: return std::unique_ptr<BinomialDist>(new BinomialDist);

    default: break;
    }
    return nullptr;
}

} // namespace R8
