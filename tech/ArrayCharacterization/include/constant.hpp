#ifndef MSE_CONSTANTS_HPP
#define MSE_CONSTANTS_HPP

#include "unit.hpp"

#include <array>

namespace mse::constant {

// TODO replace with log interface
inline constexpr bool debugWarningsEnabled = false;

using namespace mse::unit;

inline constexpr double invalidValue = 1e41;

inline constexpr SecondExp<-1> infiniteRamp{1e41};

inline constexpr Feature minimumGapBetweenPandNDiffusions{2};
inline constexpr Feature minimumGapBetweenSameDiffusions{1.5};
inline constexpr Feature minimumGapBetweenPoly{1.5};
inline constexpr Feature minimumGapBetweenContactAndPoly{0.75};
inline constexpr Feature minimumPowerRailWidth{2};
inline constexpr Feature contactSize{1};

inline constexpr double averageLeakRatioNand2 = 0.48;
inline constexpr double averageLeakRatioNand3 = 0.31;
inline constexpr double averageLeakRatioNor2 = 0.95;
inline constexpr double averageLeakRatioNor3 = 0.62;

inline constexpr int comparatorInvChainLength = 4;
inline constexpr int maxInvChainLength = 20;

inline constexpr Feature maxNmosSize{100};
inline constexpr Feature minNmosSize{1.5};
inline constexpr Feature maxTransistorHeight{20};

// FinFET and GAA layout rules from NeuroSim 1.4
inline constexpr Feature maxTransistorHeightFinfet{34};
inline constexpr Feature polyWidthFinfet{1.4};
inline constexpr Feature minimumGapBetweenGatePolyFinfet{3.9};

struct FinfetLayoutRules {
    int featureSizeInNano;
    Feature cellHeight; // Standard cell height
    Feature contactedPolyPitch;
    Feature gapBetweenPandNDiffusions; // Gap between the P and N fins
    Feature outerRegionHeight; // Cell height left after the P and N gap and the fin region
};

inline constexpr std::array<FinfetLayoutRules, 7> finfetLayoutRules = {{
    {14, Feature(41.142), Feature(5.571), Feature(9.71), Feature(10.285)}, // Samsung, the gap is 3 fin pitches less a fin width
    {10, Feature(33.0), Feature(6.4), Feature(10.0), Feature(8.2)}, // TSMC
    {7, Feature(34.285), Feature(8.1428), Feature(11.85), Feature(12.28)}, // TSMC
    {5, Feature(36.0), Feature(10.2), Feature(12.8), Feature(19.4)}, // IRDS 2021
    {3, Feature(48.0), Feature(16.0), Feature(15.0), Feature(12.666)}, // IRDS 2022
    {2, Feature(57.0), Feature(22.5), Feature(20.0), Feature(22.0)}, // IRDS 2022
    {1, Feature(80.0), Feature(40.0), Feature(15.0), Feature(40.0)} // IRDS 2022
}};

// The 3T read device's gate is the storage node, so it never sees the on current's Vgs = vdd
inline constexpr double readDeviceOnCurrentRatio = 0.5;

inline constexpr double optimalStageEffort = 4;
inline constexpr double areaOptimizationConstraint = 1;

inline constexpr Feature senseAmpWidthP{7.5};
inline constexpr Feature senseAmpWidthN{3.75};
inline constexpr Feature senseAmpWidthIsolation{12.5};
inline constexpr Feature senseAmpWidthEnable{5};
inline constexpr Feature senseAmpWidthMux{9};


inline constexpr Volt minBitlineSenseVoltage{0.08};
inline constexpr FeatureExp<2> ivConverterArea{50000};

inline constexpr Feature stitchingOverhead{7.5};

inline constexpr Second dramRefreshPeriod{64_ms};

inline constexpr double shaperEfficiencyConservative = 0.2;
inline constexpr double shaperEfficiencyAggressive = 1;

inline constexpr Unit<OhmAtom<1>, MeterAtom<1>> copperResistivity{2.2e-8};
inline constexpr KelvinExp<-1> copperResistivityTemperatureCoefficient{0.0039};
inline constexpr Unit<FaradAtom<1>, MeterAtom<-1>> vacuumPermittivity{8.85e-12};

inline constexpr Unit<AmpereAtom<1>, MeterAtom<-2>> tunnelCurrentFlow{10};
inline constexpr Volt floatingGateThresholdDelta{5};

inline constexpr double resistanceAdjustment = 8.6;
inline constexpr Volt voltageSwing{0.1};

inline constexpr double bankAspectRatioConstraint = 3;

inline constexpr double bitlineLeakageTolerance = 1;
inline constexpr double irDropTolerance = 0.2;

inline constexpr int totalAddressBit = 48;

}

#endif
