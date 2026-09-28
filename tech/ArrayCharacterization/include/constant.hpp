#ifndef MSE_CONSTANTS_HPP
#define MSE_CONSTANTS_HPP

#include "unit.hpp"

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

inline constexpr double optimalStageEffort = 4;
inline constexpr double areaOptimizationConstraint = 1;

inline constexpr Feature senseAmpWidthP{7.5};
inline constexpr Feature senseAmpWidthN{3.75};
inline constexpr Feature senseAmpWidthIsolation{12.5};
inline constexpr Feature senseAmpWidthEnable{5};
inline constexpr Feature senseAmpWidthMux{9};

inline constexpr Feature senseAmpAltWidthTop{40};
inline constexpr Feature senseAmpAltWidthP{2};
inline constexpr Feature senseAmpAltWidthN{2};
inline constexpr Feature senseAmpAltWidthBottom{20};

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
