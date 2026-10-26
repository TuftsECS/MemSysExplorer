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
*   Cong Xu	    ( Email: czx102 at psu dot edu 
*                     Website: http://www.cse.psu.edu/~czx102/ )
*   Xiangyu Dong    ( Email: xydong at cse dot psu dot edu
*                     Website: http://www.cse.psu.edu/~xydong/ )
*******************************************************************************/


#include "formula.hpp"
#include "constant.hpp"

#include <stdlib.h>
#include <algorithm>

bool IsPlanar(const Technology& tech) {
	return tech.featureSizeInNano >= 22;
}

static bool IsGAA(const Technology& tech) {
	return tech.featureSizeInNano < 3;
}

static int CalculateNumFins(double width, const Technology& tech) {
	if (width <= 0)
		return 0;
	return (int)ceil(width / (2 * tech.featureSize.value()) - 1e-9);
}

static double EffectiveWidthPerFin(const Technology& tech) {
	if (IsGAA(tech))
		return tech.effectiveWidth.value() * tech.maximumSheetCount / tech.maximumFinsPerGAA;
	return tech.effectiveWidth.value();
}

double CalculateEffectiveWidth(double width, const Technology& tech) {
	if (IsPlanar(tech))
		return width;
	return CalculateNumFins(width, tech) * EffectiveWidthPerFin(tech);
}

double CalculateLayoutWidth(double widthEff, const Technology& tech) {
	if (IsPlanar(tech))
		return widthEff;
	return ceil(widthEff / EffectiveWidthPerFin(tech) - 1e-9) * 2 * tech.featureSize.value();
}

double CalculateGateCap(double width, Technology tech) {
	return (tech.capIdealGate.value() + tech.capOverlap.value() + 3 * tech.capFringe.value()) * CalculateEffectiveWidth(width, tech)
			+ tech.phyGateLength.value() * tech.capPolywire.value();
}

double CalculateFBRAMGateCap(double width, double thicknessFactor, Technology tech) {
	return (tech.capIdealGate.value() / thicknessFactor + tech.capOverlap.value() + 3 * tech.capFringe.value()) * width
			+ tech.phyGateLength.value() * tech.capPolywire.value();
}

// To calculate FeFET cell gate cap, use scaled capIdealGateFeFET value
double CalculateFeFETGateCap(double width, Technology tech) {
	return (tech.capIdealGateFeFET.value() + tech.capOverlap.value() + 3 * tech.capFringe.value()) * CalculateEffectiveWidth(width, tech)
			+ tech.phyGateLength.value() * tech.capPolywire.value();
}

double CalculateFBRAMDrainCap(double width, Technology tech) {
	return (3 * tech.capSidewall.value() + tech.capDrainToChannel.value()) * width;
}

/* FinFET / GAA gate layout, after NeuroSim 1.4's CalculateGateArea (as in NS-Cache) */
double CalculateFinFETGateArea(
		GateType type, int numInput,
		double widthNMOS, double widthPMOS,
		double heightTransistorRegion, const Technology& tech,
		double* height, double* width) {
	double F = tech.featureSize.value();
	double cellHeight = mse::constant::maxTransistorHeightFinfet.value();
	double cpp = mse::constant::polyWidthFinfet.value() + mse::constant::minimumGapBetweenGatePolyFinfet.value();
	double gapPN = 0, outer = 0;
	for (const mse::constant::FinfetLayoutRules& rules : mse::constant::finfetLayoutRules) {
		if (rules.featureSizeInNano == tech.featureSizeInNano) {
			cellHeight = rules.cellHeight.value();
			cpp = rules.contactedPolyPitch.value();
			gapPN = rules.gapBetweenPandNDiffusions.value();
			outer = rules.outerRegionHeight.value();
		}
	}
	bool standard = fabs(heightTransistorRegion - mse::constant::maxTransistorHeight.value() * F) < 1e-3 * F || heightTransistorRegion > 1e-3;
	double region = standard ? cellHeight * F : heightTransistorRegion;
	int maxFin = IsGAA(tech) ? tech.maximumFinsPerGAA : tech.maximumFinCount;
	if (!standard && gapPN > 0)
		maxFin = (int)floor(((region - (gapPN + outer) * F) + tech.finPitch.value()) / 2.0 / (tech.widthFin.value() + tech.finPitch.value()));
	maxFin = std::max(maxFin, 1);

	int numFinN = CalculateNumFins(widthNMOS, tech);
	int numFinP = CalculateNumFins(widthPMOS, tech);
	if (numFinN + numFinP == 0) {
		*height = *width = 0;
		return 0;
	}
	/* split the 2 x maxFin fin tracks between P and N by width */
	int tracks = 2 * maxFin, maxFinP = tracks, maxFinN = tracks;
	if (numFinP == 0) {
		maxFinP = 0;
	} else if (numFinN == 0) {
		maxFinN = 0;
	} else {
		double ratio = widthPMOS / (widthPMOS + widthNMOS);
		if (ratio > 0.5) {
			maxFinN = (int)(tracks * (1 - ratio));
			maxFinP = tracks - maxFinN;
		} else {
			maxFinP = (int)(tracks * ratio);
			maxFinN = tracks - maxFinP;
		}
		if (maxFinP == 0) { maxFinP = 1; maxFinN = tracks - 1; }
		if (maxFinN == 0) { maxFinN = 1; maxFinP = tracks - 1; }
	}
	int numFoldP = numFinP > 0 ? (numFinP + maxFinP - 1) / maxFinP : 0;
	int numFoldN = numFinN > 0 ? (numFinN + maxFinN - 1) / maxFinN : 0;
	double unitWidthP = numFoldP > 0 ? (numFoldP + 1) * cpp * F : 0;
	double unitWidthN = numFoldN > 0 ? (numFoldN + 1) * cpp * F : 0;

	double widthRegionP = 0, widthRegionN = 0;
	if (type == GateType::INV) {
		widthRegionP = unitWidthP;
		widthRegionN = unitWidthN;
	} else if (type == GateType::NOR || type == GateType::NAND) {
		/* unfolded stacks share their inner contacts */
		bool share = numFoldP <= 1 && numFoldN <= 1;
		widthRegionP = unitWidthP * numInput - ((share && unitWidthP > 0) ? (numInput - 1) * cpp * F : 0);
		widthRegionN = unitWidthN * numInput - ((share && unitWidthN > 0) ? (numInput - 1) * cpp * F : 0);
	}
	*width = std::max(widthRegionN, widthRegionP);
	*height = region;
	return (*width)*(*height);
}

double CalculateGateArea(
		GateType type, int numInput,
		double widthNMOS, double widthPMOS,
		double heightTransistorRegion, Technology tech,
		double* height, double* width) {
	if (!IsPlanar(tech))
		return CalculateFinFETGateArea(type, numInput, widthNMOS, widthPMOS, heightTransistorRegion, tech, height, width);

	double	ratio = widthPMOS / (widthPMOS + widthNMOS);

	double maxWidthPMOS, maxWidthNMOS;
	double unitWidthRegionP, unitWidthRegionN;
	double widthRegionP, widthRegionN;
	double heightRegionP, heightRegionN;

	if (ratio == 0) {	/* no PMOS */
		maxWidthPMOS = 0;
		maxWidthNMOS = heightTransistorRegion;
	} else if (ratio == 1) {	/* no NMOS */
		maxWidthPMOS = heightTransistorRegion;
		maxWidthNMOS = 0;
	} else {
		maxWidthPMOS = ratio * (heightTransistorRegion - mse::constant::minimumGapBetweenPandNDiffusions.value() * tech.featureSize.value());
		maxWidthNMOS = maxWidthPMOS / ratio * (1 - ratio);
	}

	if (widthPMOS > 0) {
		if (widthPMOS < maxWidthPMOS) { /* No folding */
			unitWidthRegionP = tech.featureSize.value();
			heightRegionP = widthPMOS;
		} else {	/* Folding */
			int numFoldedPMOS = (int)(ceil(widthPMOS / (maxWidthPMOS - 3 * tech.featureSize.value())));	// 3F for folding overhead
			unitWidthRegionP = numFoldedPMOS * tech.featureSize.value() + (numFoldedPMOS-1) * tech.featureSize.value() * mse::constant::minimumGapBetweenPoly.value();
			heightRegionP = maxWidthPMOS;
		}
	} else {
		unitWidthRegionP = 0;
		heightRegionP = 0;
	}

	if (widthNMOS > 0) {
		if (widthNMOS < maxWidthNMOS) { /* No folding */
			unitWidthRegionN = tech.featureSize.value();
			heightRegionN = widthNMOS;
		} else {	/* Folding */
			int numFoldedNMOS = (int)(ceil(widthNMOS / (maxWidthNMOS - 3 * tech.featureSize.value())));	// 3F for folding overhead
			unitWidthRegionN = numFoldedNMOS * tech.featureSize.value() + (numFoldedNMOS-1) * tech.featureSize.value() * mse::constant::minimumGapBetweenPoly.value();
			heightRegionN = maxWidthNMOS;
		}
	} else {
		unitWidthRegionN = 0;
		heightRegionN = 0;
	}

	switch (type) {
		case GateType::INV:
			widthRegionP = 2 * tech.featureSize.value() * (mse::constant::contactSize.value() + mse::constant::minimumGapBetweenContactAndPoly.value() * 2) + unitWidthRegionP;
			widthRegionN = 2 * tech.featureSize.value() * (mse::constant::contactSize.value() + mse::constant::minimumGapBetweenContactAndPoly.value() * 2) + unitWidthRegionN;
			break;
		case GateType::NOR:
			widthRegionP = 2 * tech.featureSize.value() * (mse::constant::contactSize.value() + mse::constant::minimumGapBetweenContactAndPoly.value() * 2)
							+ unitWidthRegionP * numInput + (numInput - 1) * tech.featureSize.value() * mse::constant::minimumGapBetweenPoly.value();
			widthRegionN = 2 * tech.featureSize.value() * (mse::constant::contactSize.value() + mse::constant::minimumGapBetweenContactAndPoly.value() * 2)
							+ unitWidthRegionN * numInput
							+ (numInput - 1) * tech.featureSize.value() * (mse::constant::contactSize.value() + mse::constant::minimumGapBetweenContactAndPoly.value() * 2);
			break;
		case GateType::NAND:
			widthRegionN = 2 * tech.featureSize.value() * (mse::constant::contactSize.value() + mse::constant::minimumGapBetweenContactAndPoly.value() * 2)
							+ unitWidthRegionN * numInput + (numInput - 1) * tech.featureSize.value() * mse::constant::minimumGapBetweenPoly.value();
			widthRegionP = 2 * tech.featureSize.value() * (mse::constant::contactSize.value() + mse::constant::minimumGapBetweenContactAndPoly.value() * 2)
							+ unitWidthRegionP * numInput
							+ (numInput - 1) * tech.featureSize.value() * (mse::constant::contactSize.value() + mse::constant::minimumGapBetweenContactAndPoly.value() * 2);
			break;
		default:
			widthRegionN = widthRegionP = 0;
	}

	*width = std::max(widthRegionN, widthRegionP);
	if (widthPMOS > 0 && widthNMOS > 0) {	/* it is a gate */
		*height = heightRegionN + heightRegionP + tech.featureSize.value() * mse::constant::minimumGapBetweenPandNDiffusions.value()
					+ 2 * tech.featureSize.value() * mse::constant::minimumPowerRailWidth.value();
	} else {	/* it is a transistor */
		*height = heightRegionN + heightRegionP;	/* one of them is zero, and no power rail is added */
	}

	return (*width)*(*height);
}

void CalculateGateCapacitance(
		GateType type, int numInput,
		double widthNMOS, double widthPMOS,
		double heightTransistorRegion, Technology tech,
		double* capInput, double* capOutput) {
	if (!IsPlanar(tech)) {
		double drainN = CalculateEffectiveWidth(widthNMOS, tech) * tech.capDrainTotal.value();
		double drainP = CalculateEffectiveWidth(widthPMOS, tech) * tech.capDrainTotal.value();
		if (capOutput) {
			switch (type) {
			case GateType::INV:  *(capOutput) = drainN + drainP; break;
			case GateType::NOR:  *(capOutput) = drainN * numInput + drainP; break;
			case GateType::NAND: *(capOutput) = drainN + drainP * numInput; break;
			default:             *(capOutput) = 0;
			}
		}
		if (capInput)
			*(capInput) = CalculateGateCap(widthNMOS, tech) + CalculateGateCap(widthPMOS, tech);
		return;
	}

	/* TODO: most parts of this function is the same of CalculateGateArea,
	 * perhaps they will be combined in future
	 */
    double	ratio = widthPMOS / (widthPMOS + widthNMOS);

	double maxWidthPMOS = 0, maxWidthNMOS = 0;
	double unitWidthDrainP = 0, unitWidthDrainN = 0;
	double widthDrainP = 0, widthDrainN = 0;
	double heightDrainP = 0, heightDrainN = 0;
	int numFoldedPMOS = 1, numFoldedNMOS = 1;

	if (ratio == 0) {	/* no PMOS */
		maxWidthPMOS = 0;
		maxWidthNMOS = heightTransistorRegion;
	} else if (ratio == 1) {	/* no NMOS */
		maxWidthPMOS = heightTransistorRegion;
		maxWidthNMOS = 0;
	} else {
		maxWidthPMOS = ratio * (heightTransistorRegion - mse::constant::minimumGapBetweenPandNDiffusions.value() * tech.featureSize.value());
		maxWidthNMOS = maxWidthPMOS / ratio * (1 - ratio);
	}

	if (widthPMOS > 0) {
		if (widthPMOS < maxWidthPMOS) { /* No folding */
			unitWidthDrainP = 0;
			heightDrainP = widthPMOS;
		} else {	/* Folding */
			if (maxWidthPMOS < 3 * tech.featureSize.value()) {
				std::cout << "Error: Unable to do PMOS folding because PMOS size limitation is less than 3F!" << std::endl;
				exit(-1);
			}
			numFoldedPMOS = (int)(ceil(widthPMOS / (maxWidthPMOS - 3 * tech.featureSize.value())));	// 3F for folding overhead
			unitWidthDrainP = (numFoldedPMOS-1) * tech.featureSize.value() * mse::constant::minimumGapBetweenPoly.value();
			heightDrainP = maxWidthPMOS;
		}
	} else {
		unitWidthDrainP = 0;
		heightDrainP = 0;
	}

	if (widthNMOS > 0) {
		if (widthNMOS < maxWidthNMOS) { /* No folding */
			unitWidthDrainN = 0;
			heightDrainN = widthNMOS;
		} else {	/* Folding */
			if (maxWidthNMOS < 3 * tech.featureSize.value()) {
				std::cout << "Error: Unable to do NMOS folding because NMOS size limitation is less than 3F!" << std::endl;
				exit(-1);
			}
			numFoldedNMOS = (int)(ceil(widthNMOS / (maxWidthNMOS - 3 * tech.featureSize.value())));	// 3F for folding overhead
			unitWidthDrainN = (numFoldedNMOS-1) * tech.featureSize.value() * mse::constant::minimumGapBetweenPoly.value();
			heightDrainN = maxWidthNMOS;
		}
	} else {
		unitWidthDrainN = 0;
		heightDrainN = 0;
	}

	switch (type) {
		case GateType::INV:
			if (widthPMOS > 0)
				widthDrainP = tech.featureSize.value() * (mse::constant::contactSize.value() + mse::constant::minimumGapBetweenContactAndPoly.value() * 2) + unitWidthDrainP;
			if (widthNMOS > 0)
				widthDrainN = tech.featureSize.value() * (mse::constant::contactSize.value() + mse::constant::minimumGapBetweenContactAndPoly.value() * 2) + unitWidthDrainN;
			break;
		case GateType::NOR:
			/* PMOS is in series, worst case capacitance is below */
			if (widthPMOS > 0)
				widthDrainP = tech.featureSize.value() * (mse::constant::contactSize.value() + mse::constant::minimumGapBetweenContactAndPoly.value() * 2)
							+ unitWidthDrainP * numInput + (numInput - 1) * tech.featureSize.value() * mse::constant::minimumGapBetweenPoly.value();
			/* NMOS is parallel, capacitance is multiplied as below */
			if (widthNMOS > 0)
				widthDrainN = (tech.featureSize.value() * (mse::constant::contactSize.value() + mse::constant::minimumGapBetweenContactAndPoly.value() * 2)
							+ unitWidthDrainN) * numInput;
			break;
		case GateType::NAND:
			/* NMOS is in series, worst case capacitance is below */
			if (widthNMOS > 0)
				widthDrainN = tech.featureSize.value() * (mse::constant::contactSize.value() + mse::constant::minimumGapBetweenContactAndPoly.value() * 2)
							+ unitWidthDrainN * numInput + (numInput - 1) * tech.featureSize.value() * mse::constant::minimumGapBetweenPoly.value();
			/* PMOS is parallel, capacitance is multiplied as below */
			if (widthPMOS > 0)
				widthDrainP = (tech.featureSize.value() * (mse::constant::contactSize.value() + mse::constant::minimumGapBetweenContactAndPoly.value() * 2)
							+ unitWidthDrainP) * numInput;
			break;
		default:
			widthDrainN = widthDrainP = 0;
	}

	/* Junction capacitance */
	double capDrainBottomN = widthDrainN * heightDrainN * tech.capJunction.value();
	double capDrainBottomP = widthDrainP * heightDrainP * tech.capJunction.value();

	/* Sidewall capacitance */
	double capDrainSidewallN, capDrainSidewallP;
	if (numFoldedNMOS % 2 == 0)
		capDrainSidewallN = 2 * widthDrainN * tech.capSidewall.value();
	else
		capDrainSidewallN = (2 * widthDrainN + heightDrainN) * tech.capSidewall.value();
	if (numFoldedPMOS % 2 == 0)
		capDrainSidewallP = 2 * widthDrainP * tech.capSidewall.value();
	else
		capDrainSidewallP = (2* widthDrainP + heightDrainP) * tech.capSidewall.value();

	/* Drain to channel capacitance */
	double capDrainToChannelN = numFoldedNMOS * heightDrainN * tech.capDrainToChannel.value();
	double capDrainToChannelP = numFoldedPMOS * heightDrainP * tech.capDrainToChannel.value();

	if (capOutput)
		*(capOutput) = capDrainBottomN + capDrainBottomP + capDrainSidewallN + capDrainSidewallP + capDrainToChannelN + capDrainToChannelP;
	if (capInput)
		*(capInput) = CalculateGateCap(widthNMOS, tech) + CalculateGateCap(widthPMOS, tech);
}

double CalculateDrainCap(
		double width, CmosType type,
		double heightTransistorRegion, Technology tech) {
	double drainCap = 0;
	if (type == CmosType::NMOS) {
		CalculateGateCapacitance(GateType::INV, 1, width, 0, heightTransistorRegion, tech, nullptr, &drainCap);
	} else {
		CalculateGateCapacitance(GateType::INV, 1, 0, width, heightTransistorRegion, tech, nullptr, &drainCap);
    }
	return drainCap;
}

double CalculateGateLeakage(
		GateType type, int numInput,
		double widthNMOS, double widthPMOS,
		double temperature, Technology tech) {
	int tempIndex = (int)temperature - 300;
	if ((tempIndex > 100) || (tempIndex < 0)) {
		std::cout<<"Error: Temperature is out of range"<< std::endl;
		exit(-1);
	}
	const Technology::TemperatureTable& leakN = tech.currentOffNmos;
	const Technology::TemperatureTable& leakP = tech.currentOffPmos;
	double leakageN, leakageP;
	widthNMOS = CalculateEffectiveWidth(widthNMOS, tech);
	widthPMOS = CalculateEffectiveWidth(widthPMOS, tech);
	switch (type) {
		case GateType::INV:
			leakageN = widthNMOS * leakN[tempIndex].value();
			leakageP = widthPMOS * leakP[tempIndex].value();
			return std::max(leakageN, leakageP);
		case GateType::NOR:
			leakageN = widthNMOS * leakN[tempIndex].value() * numInput;
			if (numInput == 2) {
				return mse::constant::averageLeakRatioNor2 * leakageN;
			}
			else {
				return mse::constant::averageLeakRatioNor3 * leakageN;
			}
		case GateType::NAND:
			leakageP = widthPMOS * leakP[tempIndex].value() * numInput;
			if (numInput == 2) {
				return mse::constant::averageLeakRatioNand2 * leakageP;
			}
			else {
				return mse::constant::averageLeakRatioNand3 * leakageP;
			}
		default:
			return 0.0;
	}
}

double CalculateOnResistance(double width, CmosType type, double temperature, Technology tech) {
	double r;
	int tempIndex = (int)temperature - 300;
	if ((tempIndex > 100) || (tempIndex < 0)) {
		std::cout<<"Error: Temperature is out of range"<< std::endl;
		exit(-1);
	}
	double widthEff = CalculateEffectiveWidth(width, tech);
	if (type == CmosType::NMOS)
		r = tech.effectiveResistanceMultiplier * tech.vdd.value() / (tech.currentOnNmos[tempIndex].value() * widthEff);
	else
		r = tech.effectiveResistanceMultiplier * tech.vdd.value() / (tech.currentOnPmos[tempIndex].value() * widthEff);
	return r;
}

double CalculateTransconductance(double width, CmosType type, Technology tech) {
	double gm;
	double vsat;
	double widthEff = CalculateEffectiveWidth(width, tech);

	if (type == CmosType::NMOS) {
		if (tech.featureSizeInNano >= 22) {
			vsat = std::min(tech.vdsatNmos.value(), tech.vdd.value() - tech.vth.value());
 			gm = (tech.effectiveElectronMobility.value() * tech.capOx.value()) / 2 * width / tech.phyGateLength.value() * vsat;
        } else {
        gm = tech.onCurrentTransconductance.value()*widthEff;
        }

    } else {//type==PMOS
        if (tech.featureSizeInNano >= 22) {
			vsat = std::min(tech.vdsatPmos.value(), tech.vdd.value() - tech.vth.value());
 			gm = (tech.effectiveHoleMobility.value() * tech.capOx.value()) / 2 * width / tech.phyGateLength.value() * vsat;
        } else {
        gm = tech.onCurrentTransconductance.value()*widthEff;
        }
    }
	return gm;
}

double CalculateWireResistance(
		double resistivity, double wireWidth, double wireThickness,
		double barrierThickness, double dishingThickness, double alphaScatter) {
	return(alphaScatter * resistivity / (wireThickness - barrierThickness - dishingThickness)
			/ (wireWidth - 2 * barrierThickness));
}

double CalculateWireCapacitance(
		double permittivity, double wireWidth, double wireThickness, double wireSpacing,
		double ildThickness, double millerValue, double horizontalDielectric,
		double verticalDielectric, double fringeCap) {
	double verticalCap, sidewallCap;
	verticalCap = 2 * permittivity * verticalDielectric * wireWidth / ildThickness;
	sidewallCap = 2 * permittivity * millerValue * horizontalDielectric * wireThickness / wireSpacing;
	return (verticalCap + sidewallCap + fringeCap);
}
