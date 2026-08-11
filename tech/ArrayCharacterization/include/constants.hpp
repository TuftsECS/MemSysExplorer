#ifndef MSE_CONSTANTS_HPP
#define MSE_CONSTANTS_HPP

#include "unit.hpp"

namespace mse::constant {

using namespace mse::unit;

inline constexpr Feature minimumGapBetweenPandNDiffusions = 2_feat;
inline constexpr Feature minimumGapBetweenSameDiffusions = 1.5_feat;
inline constexpr Feature minimumGapBetweenPoly = 1.5_feat;
inline constexpr Feature minimumGapBetweenContactAndPoly = 0.75_feat;
inline constexpr Feature contactSize = 1_feat;
inline constexpr Feature minimumPowerRailWidth = 2_feat;
inline constexpr double averageLeakRatioNand2 = 0.48;
inline constexpr double averageLeakRatioNand3 = 0.31;
inline constexpr double averageLeakRatioNor2 = 0.95;
inline constexpr double averageLeakRatioNor3 = 0.62;

}

#endif
