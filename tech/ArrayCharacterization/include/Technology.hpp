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


#ifndef MSE_TECHNOLOGY_H
#define MSE_TECHNOLOGY_H

#include "typedef.hpp"

#include <iostream>

class Technology {
public:
	/* Functions */
	void PrintProperty();
	void Initialize(int _featureSizeInNano, DeviceRoadmap _deviceRoadmap, bool _is_FeFET);
	void InterpolateWith(Technology rhs, double _alpha);

	/* Properties */
	bool initialized = false;	/* Initialization flag */
	int featureSizeInNano; /*Process feature size, Unit: nm */
        bool is_FeFET; /*flag for whether gate cap needs to be scaled */
	double featureSize;	/* Process feature size, Unit: m */
	DeviceRoadmap deviceRoadmap;	/* HP, LSTP, or LOP */
	double vdd;			/* Supply voltage, Unit: V */
    double vpp;             /* Wordline overdrive voltage. */
	double vth;				/* Threshold voltage, Unit: V */


	double heightFin;	/* Fin height, Unit: m */
	double widthFin;	/* Fin width, Unit: m */
	double PitchFin;	/* Fin pitch, Unit: m */


	double vdsatNmos;		/* Velocity saturation voltage, Unit: V */
	double vdsatPmos;		/* Velocity saturation voltage, Unit: V */
	double phyGateLength;	/* Physical gate length, Unit: m */
	double capIdealGate;	/* Ideal gate capacitance, Unit: F/m */
	double capIdealGateFeFET;	/* Ideal gate capacitance just for FeFET memory cells, Unit: F/m */
	double capFringe;		/* Fringe capacitance, Unit: F/m */
	double capJunction;		/* Junction bottom capacitance, Cj0, Unit: F/m^2 */
	double capOverlap;		/* Overlap capacitance, Cover in MASTAR, Unit: F/m */
	double capSidewall;		/* Junction sidewall capacitance, Cjsw, Unit: F/m */
	double capDrainToChannel;	/* Junction drain to channel capacitance, Cjswg, Unit: F/m */
	double capOx;			/* Cox_elec in MASTAR, Unit: F/m^2 */
	double buildInPotential; /* Bottom junction built-in potential(PB in BSIM4 model), Unit: V */
	double effectiveElectronMobility;		 /* ueff for NMOS in MASTAR, Unit: m^2/V/s */
	double effectiveHoleMobility;            /* ueff for PMOS in MASTAR, Unit: m^2/V/s */
	double pnSizeRatio;	/* PMOS to NMOS size ratio */
	double effectiveResistanceMultiplier;	/* Extra resistance due to vdsat */
	double currentOnNmos[101];		/* NMOS saturation current, Unit: A/m */
	double currentOnPmos[101];		/* PMOS saturation current, Unit: A/m */
	double currentOffNmos[101];	/* NMOS off current (from 300K to 400K), Unit: A/m */
	double currentOffPmos[101]; /* PMOS off current (from 300K to 400K), Unit: A/m */

	double capPolywire;	/* Poly wire capacitance, Unit: F/m */

    double current_gmNmos;		/* NMOS current at 0.7*vdd for gm calculation, Unit: A/m/V*/ 
    double current_gmPmos;		/* PMOS current at 0.7*vdd for gm calculation, Unit: A/m/V*/ 
	double max_sheet_num=0;
	double thickness_sheet=0;
	double width_sheet=0;
	double effective_width=0;
	double max_fin_num=0;
	double max_fin_per_GAA=0;
	double gm_oncurrent = 0;  // gm at on current
	double cap_draintotal = 0;
};

#endif /* MSE_TECHNOLOGY_H */
