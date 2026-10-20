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

double CalculateGateCap(double width, Technology tech) {
	return (tech.capIdealGate + tech.capOverlap + 3 * tech.capFringe) * width
			+ tech.phyGateLength * tech.capPolywire;
}

double CalculateFBRAMGateCap(double width, double thicknessFactor, Technology tech) {
	return (tech.capIdealGate / thicknessFactor + tech.capOverlap + 3 * tech.capFringe) * width
			+ tech.phyGateLength * tech.capPolywire;
}

// To calculate FeFET cell gate cap, use scaled capIdealGateFeFET value
double CalculateFeFETGateCap(double width, Technology tech) {
	return (tech.capIdealGateFeFET + tech.capOverlap + 3 * tech.capFringe) * width
			+ tech.phyGateLength * tech.capPolywire;
}

double CalculateFBRAMDrainCap(double width, Technology tech) {
	return (3 * tech.capSidewall + tech.capDrainToChannel) * width;
}

double CalculateGateArea(
		GateType type, int numInput,
		double widthNMOS, double widthPMOS,
		double heightTransistorRegion, Technology tech,
		double* height, double* width) {
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
		maxWidthPMOS = ratio * (heightTransistorRegion - mse::constant::minimumGapBetweenPandNDiffusions.value() * tech.featureSize);
		maxWidthNMOS = maxWidthPMOS / ratio * (1 - ratio);
	}

	if (widthPMOS > 0) {
		if (widthPMOS < maxWidthPMOS) { /* No folding */
			unitWidthRegionP = tech.featureSize;
			heightRegionP = widthPMOS;
		} else {	/* Folding */
			int numFoldedPMOS = (int)(ceil(widthPMOS / (maxWidthPMOS - 3 * tech.featureSize)));	/* 3F for folding overhead */
			unitWidthRegionP = numFoldedPMOS * tech.featureSize + (numFoldedPMOS-1) * tech.featureSize * mse::constant::minimumGapBetweenPoly.value();
			heightRegionP = maxWidthPMOS;
		}
	} else {
		unitWidthRegionP = 0;
		heightRegionP = 0;
	}

	if (widthNMOS > 0) {
		if (widthNMOS < maxWidthNMOS) { /* No folding */
			unitWidthRegionN = tech.featureSize;
			heightRegionN = widthNMOS;
		} else {	/* Folding */
			int numFoldedNMOS = (int)(ceil(widthNMOS / (maxWidthNMOS - 3 * tech.featureSize)));	/* 3F for folding overhead */
			unitWidthRegionN = numFoldedNMOS * tech.featureSize + (numFoldedNMOS-1) * tech.featureSize * mse::constant::minimumGapBetweenPoly.value();
			heightRegionN = maxWidthNMOS;
		}
	} else {
		unitWidthRegionN = 0;
		heightRegionN = 0;
	}

	switch (type) {
	case GateType::INV:
		widthRegionP = 2 * tech.featureSize * (mse::constant::contactSize.value() + mse::constant::minimumGapBetweenContactAndPoly.value() * 2) + unitWidthRegionP;
		widthRegionN = 2 * tech.featureSize * (mse::constant::contactSize.value() + mse::constant::minimumGapBetweenContactAndPoly.value() * 2) + unitWidthRegionN;
		break;
	case GateType::NOR:
		widthRegionP = 2 * tech.featureSize * (mse::constant::contactSize.value() + mse::constant::minimumGapBetweenContactAndPoly.value() * 2)
						+ unitWidthRegionP * numInput + (numInput - 1) * tech.featureSize * mse::constant::minimumGapBetweenPoly.value();
		widthRegionN = 2 * tech.featureSize * (mse::constant::contactSize.value() + mse::constant::minimumGapBetweenContactAndPoly.value() * 2)
						+ unitWidthRegionN * numInput
						+ (numInput - 1) * tech.featureSize * (mse::constant::contactSize.value() + mse::constant::minimumGapBetweenContactAndPoly.value() * 2);
		break;
	case GateType::NAND:
		widthRegionN = 2 * tech.featureSize * (mse::constant::contactSize.value() + mse::constant::minimumGapBetweenContactAndPoly.value() * 2)
						+ unitWidthRegionN * numInput + (numInput - 1) * tech.featureSize * mse::constant::minimumGapBetweenPoly.value();
		widthRegionP = 2 * tech.featureSize * (mse::constant::contactSize.value() + mse::constant::minimumGapBetweenContactAndPoly.value() * 2)
						+ unitWidthRegionP * numInput
						+ (numInput - 1) * tech.featureSize * (mse::constant::contactSize.value() + mse::constant::minimumGapBetweenContactAndPoly.value() * 2);
		break;
	default:
		widthRegionN = widthRegionP = 0;
	}

	*width = std::max(widthRegionN, widthRegionP);
	if (widthPMOS > 0 && widthNMOS > 0) {	/* it is a gate */
		*height = heightRegionN + heightRegionP + tech.featureSize * mse::constant::minimumGapBetweenPandNDiffusions.value()
					+ 2 * tech.featureSize * mse::constant::minimumPowerRailWidth.value();
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
	/* TO-DO: most parts of this function is the same of CalculateGateArea,
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
		maxWidthPMOS = ratio * (heightTransistorRegion - mse::constant::minimumGapBetweenPandNDiffusions.value() * tech.featureSize);
		maxWidthNMOS = maxWidthPMOS / ratio * (1 - ratio);
	}

	if (widthPMOS > 0) {
		if (widthPMOS < maxWidthPMOS) { /* No folding */
			unitWidthDrainP = 0;
			heightDrainP = widthPMOS;
		} else {	/* Folding */
			if (maxWidthPMOS < 3 * tech.featureSize) {
				std::cout << "Error: Unable to do PMOS folding because PMOS size limitation is less than 3F!" << std::endl;
				exit(-1);
			}
			numFoldedPMOS = (int)(ceil(widthPMOS / (maxWidthPMOS - 3 * tech.featureSize)));	/* 3F for folding overhead */
			unitWidthDrainP = (numFoldedPMOS-1) * tech.featureSize * mse::constant::minimumGapBetweenPoly.value();
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
			if (maxWidthNMOS < 3 * tech.featureSize) {
				std::cout << "Error: Unable to do NMOS folding because NMOS size limitation is less than 3F!" << std::endl;
				exit(-1);
			}
			numFoldedNMOS = (int)(ceil(widthNMOS / (maxWidthNMOS - 3 * tech.featureSize)));	/* 3F for folding overhead */
			unitWidthDrainN = (numFoldedNMOS-1) * tech.featureSize * mse::constant::minimumGapBetweenPoly.value();
			heightDrainN = maxWidthNMOS;
		}
	} else {
		unitWidthDrainN = 0;
		heightDrainN = 0;
	}

	switch (type) {
	case GateType::INV:
		if (widthPMOS > 0)
			widthDrainP = tech.featureSize * (mse::constant::contactSize.value() + mse::constant::minimumGapBetweenContactAndPoly.value() * 2) + unitWidthDrainP;
		if (widthNMOS > 0)
			widthDrainN = tech.featureSize * (mse::constant::contactSize.value() + mse::constant::minimumGapBetweenContactAndPoly.value() * 2) + unitWidthDrainN;
		break;
	case GateType::NOR:
		/* PMOS is in series, worst case capacitance is below */
		if (widthPMOS > 0)
			widthDrainP = tech.featureSize * (mse::constant::contactSize.value() + mse::constant::minimumGapBetweenContactAndPoly.value() * 2)
						+ unitWidthDrainP * numInput + (numInput - 1) * tech.featureSize * mse::constant::minimumGapBetweenPoly.value();
		/* NMOS is parallel, capacitance is multiplied as below */
		if (widthNMOS > 0)
			widthDrainN = (tech.featureSize * (mse::constant::contactSize.value() + mse::constant::minimumGapBetweenContactAndPoly.value() * 2)
						+ unitWidthDrainN) * numInput;
		break;
	case GateType::NAND:
		/* NMOS is in series, worst case capacitance is below */
		if (widthNMOS > 0)
			widthDrainN = tech.featureSize * (mse::constant::contactSize.value() + mse::constant::minimumGapBetweenContactAndPoly.value() * 2)
						+ unitWidthDrainN * numInput + (numInput - 1) * tech.featureSize * mse::constant::minimumGapBetweenPoly.value();
		/* PMOS is parallel, capacitance is multiplied as below */
		if (widthPMOS > 0)
			widthDrainP = (tech.featureSize * (mse::constant::contactSize.value() + mse::constant::minimumGapBetweenContactAndPoly.value() * 2)
						+ unitWidthDrainP) * numInput;
		break;
	default:
		widthDrainN = widthDrainP = 0;
	}

	/* Junction capacitance */
	double capDrainBottomN = widthDrainN * heightDrainN * tech.capJunction;
	double capDrainBottomP = widthDrainP * heightDrainP * tech.capJunction;

	/* Sidewall capacitance */
	double capDrainSidewallN, capDrainSidewallP;
	if (numFoldedNMOS % 2 == 0)
		capDrainSidewallN = 2 * widthDrainN * tech.capSidewall;
	else
		capDrainSidewallN = (2 * widthDrainN + heightDrainN) * tech.capSidewall;
	if (numFoldedPMOS % 2 == 0)
		capDrainSidewallP = 2 * widthDrainP * tech.capSidewall;
	else
		capDrainSidewallP = (2* widthDrainP + heightDrainP) * tech.capSidewall;

	/* Drain to channel capacitance */
	double capDrainToChannelN = numFoldedNMOS * heightDrainN * tech.capDrainToChannel;
	double capDrainToChannelP = numFoldedPMOS * heightDrainP * tech.capDrainToChannel;

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
	double* leakN = tech.currentOffNmos;
	double* leakP = tech.currentOffPmos;
	double leakageN, leakageP;
	switch (type) {
	case GateType::INV:
		leakageN = widthNMOS * leakN[tempIndex];
		leakageP = widthPMOS * leakP[tempIndex];
		return std::max(leakageN, leakageP);
	case GateType::NOR:
		leakageN = widthNMOS * leakN[tempIndex] * numInput;
		if (numInput == 2) {
			return mse::constant::averageLeakRatioNor2 * leakageN;
		}
		else {
			return mse::constant::averageLeakRatioNor3 * leakageN;
		}
	case GateType::NAND:
		leakageP = widthPMOS * leakP[tempIndex] * numInput;
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
	if (type == CmosType::NMOS)
		r = tech.effectiveResistanceMultiplier * tech.vdd / (tech.currentOnNmos[tempIndex] * width);
	else
		r = tech.effectiveResistanceMultiplier * tech.vdd / (tech.currentOnPmos[tempIndex] * width);
	return r;
}

double CalculateTransconductance(double width, CmosType type, Technology tech) {
	double gm;
	double vsat;
	double widthEff = 0;

	if (tech.featureSize >= 22 * 1e-9) {
		widthEff = width;
	} else if ((tech.featureSize <= 14 * 1e-9) && (tech.featureSize >= 3 * 1e-9)) {
		width = width/(2 * tech.featureSize);
		widthEff = int(ceil(width))*(2*tech.heightFin + tech.widthFin)/tech.PitchFin;
	} else {
		width = width/(2 * tech.featureSize);
		widthEff = int(ceil(width))*tech.effective_width*tech.max_sheet_num/tech.max_fin_per_GAA;       
    }

	if (type == CmosType::NMOS) {
		if (tech.featureSize >= 22 * 1e-9) {
			vsat = std::min(tech.vdsatNmos, tech.vdd - tech.vth);
 			gm = (tech.effectiveElectronMobility * tech.capOx) / 2 * width / tech.phyGateLength * vsat;
        } else {
        gm = tech.gm_oncurrent*widthEff;
        }

    } else {//type==PMOS
        if (tech.featureSize >= 22 * 1e-9) {
			vsat = std::min(tech.vdsatPmos, tech.vdd - tech.vth);
 			gm = (tech.effectiveHoleMobility * tech.capOx) / 2 * width / tech.phyGateLength * vsat;
        } else {
        gm = tech.gm_oncurrent*widthEff;
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
