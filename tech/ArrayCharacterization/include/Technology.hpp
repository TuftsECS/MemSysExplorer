/*******************************************************************************
* Copyright (c) 2012-2013, The Microsystems Design Labratory (MDL)
* Department of Computer Science and Engineering, The Pennsylvania State University
* Exascale Computing Lab, Hewlett-Packard Company
* All rights reserved.
*
* This source code is part of NVSim - An area, timing and power model for both
* volatile (e.g., SRAM, DRAM) and non-volatile memory (e.g., PCRAM, STT-RAM, ReRAM,
* SLC NAND Flash). The source code is free and you can redistribute and/or modify it
* by providing that the following conditions are met:
*
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* Author list:
*   Cong Xu     ( Email: czx102 at psu dot edu
*                     Website: http://www.cse.psu.edu/~czx102/ )
*   Xiangyu Dong    ( Email: xydong at cse dot psu dot edu
*                     Website: http://www.cse.psu.edu/~xydong/ )
*******************************************************************************/


#ifndef MSE_TECHNOLOGY_HPP
#define MSE_TECHNOLOGY_HPP

#include "typedef.hpp"
#include "unit.hpp"

#include <array>
#include <iostream>

class Technology {
public:
    using MeterPerFeature = mse::unit::Unit<mse::unit::MeterAtom<>, mse::unit::FeatureAtom<-1>>;
    using FaradPerMeter = mse::unit::Unit<mse::unit::FaradAtom<>, mse::unit::MeterAtom<-1>>;
    using FaradPerSquareMeter = mse::unit::Unit<mse::unit::FaradAtom<>, mse::unit::MeterAtom<-2>>;
    using AmperePerMeter = mse::unit::Unit<mse::unit::AmpereAtom<>, mse::unit::MeterAtom<-1>>;
    using SiemensPerMeter = mse::unit::Unit<mse::unit::SiemensAtom<>, mse::unit::MeterAtom<-1>>;
    using Mobility = mse::unit::Unit<mse::unit::MeterAtom<2>, mse::unit::VoltAtom<-1>, mse::unit::SecondAtom<-1>>;
    // Indexed by temperature from 300K to 400K
    using TemperatureTable = std::array<AmperePerMeter, 101>;

    // Functions
    void PrintProperty();
    void Initialize(int _featureSizeInNano, DeviceRoadmap _deviceRoadmap, bool _isFeFET);
    void InterpolateWith(Technology rhs, double _alpha);

    // Properties
    bool initialized = false; // Initialization flag
    int featureSizeInNano; // Process feature size in nm
    bool isFeFET; // Whether gate cap needs to be scaled
    MeterPerFeature featureSize; // Process feature size
    DeviceRoadmap deviceRoadmap; // HP, LSTP, or LOP
    mse::unit::Volt vdd; // Supply voltage
    mse::unit::Volt vpp; // Wordline overdrive voltage
    mse::unit::Volt vth; // Threshold voltage

    mse::unit::Meter heightFin;
    mse::unit::Meter widthFin;
    mse::unit::Meter finPitch;

    mse::unit::Volt vdsatNmos; // Velocity saturation voltage
    mse::unit::Volt vdsatPmos; // Velocity saturation voltage
    mse::unit::Meter phyGateLength; // Physical gate length
    FaradPerMeter capIdealGate; // Ideal gate capacitance
    FaradPerMeter capIdealGateFeFET; // Ideal gate capacitance just for FeFET memory cells
    FaradPerMeter capFringe; // Fringe capacitance
    FaradPerSquareMeter capJunction; // Junction bottom capacitance, Cj0
    FaradPerMeter capOverlap; // Overlap capacitance, Cover in MASTAR
    FaradPerMeter capSidewall; // Junction sidewall capacitance, Cjsw
    FaradPerMeter capDrainToChannel; // Junction drain to channel capacitance, Cjswg
    FaradPerSquareMeter capOx; // Cox_elec in MASTAR
    mse::unit::Volt buildInPotential; // Bottom junction built-in potential (PB in BSIM4 model)
    Mobility effectiveElectronMobility; // ueff for NMOS in MASTAR
    Mobility effectiveHoleMobility; // ueff for PMOS in MASTAR
    mse::unit::Number pnSizeRatio; // PMOS to NMOS size ratio
    mse::unit::Number effectiveResistanceMultiplier; // Extra resistance due to vdsat
    TemperatureTable currentOnNmos; // NMOS saturation current
    TemperatureTable currentOnPmos; // PMOS saturation current
    TemperatureTable currentOffNmos; // NMOS off current
    TemperatureTable currentOffPmos; // PMOS off current

    FaradPerMeter capPolywire; // Poly wire capacitance

    SiemensPerMeter transconductanceNmos; // NMOS current per volt at 0.7 vdd for gm calculation
    SiemensPerMeter transconductancePmos; // PMOS current per volt at 0.7 vdd for gm calculation
    int maximumSheetCount = 0;
    mse::unit::Meter sheetThickness;
    mse::unit::Meter sheetWidth;
    mse::unit::Meter effectiveWidth;
    int maximumFinCount = 0;
    int maximumFinsPerGAA = 0;
    SiemensPerMeter onCurrentTransconductance; // gm at on current
    FaradPerMeter capDrainTotal;
};

#endif
