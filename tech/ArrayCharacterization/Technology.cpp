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


#include "Technology.h"
#include "constant.h"
#include <math.h>

Technology::Technology() {
	// TODO Auto-generated constructor stub
	initialized = false;
}

Technology::~Technology() {
	// TODO Auto-generated destructor stub
}

void Technology::Initialize(int _featureSizeInNano, DeviceRoadmap _deviceRoadmap, bool _is_FeFET) {
	if (initialized)
		cout << "Warning: Already initialized!" << endl;


	// based on neurosim
	double caplist [7] = {103.816,97.549,100.497,81.859,72.572, 79.74, 66.94}; // 69.369
	double currentlist [7] = {595.045, 599.237, 562.048, 578.494, 641.463, 526.868, 460.979}; //  556.448
	double currentlist_off [7] = {0.0001,0.000127, 0.000147, 0.000138, 0.000158, 0.0000733, 0.000169}; //0.000569
	double eff_res_mul [7] = {2.09, 2.09, 2.05, 2.10, 2.14, 1.98, 2.05};
	double gm [7] = {1415.34, 1803.50, 1785.37, 1820.90, 2018.04, 1968.85, 2401.75};
	double vth_list [7] = {0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1}; // dummy values, since we don't need them
	double cap_draintotallist [7] = {2.499e-17, 2.668e-17, 2.224e-17, 2.076e-17, 1.791e-17, 1.543e-17, 1.409e-17};


	featureSizeInNano = _featureSizeInNano;
	featureSize = _featureSizeInNano * 1e-9;
	deviceRoadmap = _deviceRoadmap;
        is_FeFET = _is_FeFET;

	if (_featureSizeInNano >= 200) { //TO-DO : only for test
		if (_deviceRoadmap == HP) {
			vdd = 1.5;
			vth = 300e-3;
			vpp = vdd + vth;
			phyGateLength = 0.1e-6;
			capIdealGate = 8e-10;
			capFringe = 2.5e-10;
			capJunction = 1.00e-3;
			capOx = 1e-2;
			effectiveElectronMobility = 320e-4;
			effectiveHoleMobility = 80e-4;
			pnSizeRatio = 2.45;						/* from CACTI */
			effectiveResistanceMultiplier = 1.54;	/* from CACTI */
			gm_oncurrent = 0.0; // dummy value only used for advanced nodes
			currentOnNmos[0] = 750;
			currentOnNmos[10] = 750;
			currentOnNmos[20] = 750;
			currentOnNmos[30] = 750;
			currentOnNmos[40] = 750;
			currentOnNmos[50] = 750;
			currentOnNmos[60] = 750;
			currentOnNmos[70] = 750;
			currentOnNmos[80] = 750;
			currentOnNmos[90] = 750;
			currentOnNmos[100] = 750;
			currentOnPmos[0] = 350;
			currentOnPmos[10] = 350;
			currentOnPmos[20] = 350;
			currentOnPmos[30] = 350;
			currentOnPmos[40] = 350;
			currentOnPmos[50] = 350;
			currentOnPmos[60] = 350;
			currentOnPmos[70] = 350;
			currentOnPmos[80] = 350;
			currentOnPmos[90] = 350;
			currentOnPmos[100] = 350;
			currentOffNmos[0] = 8e-3;
			currentOffNmos[10] = 8e-3;
			currentOffNmos[20] = 8e-3;
			currentOffNmos[30] = 8e-3;
			currentOffNmos[40] = 8e-3;
			currentOffNmos[50] = 8e-3;
			currentOffNmos[60] = 8e-3;
			currentOffNmos[70] = 8e-3;
			currentOffNmos[80] = 8e-3;
			currentOffNmos[90] = 8e-3;
			currentOffNmos[100] = 8e-3;
			currentOffPmos[0] = 1.6e-2;
			currentOffPmos[10] = 1.6e-2;
			currentOffPmos[20] = 1.6e-2;
			currentOffPmos[30] = 1.6e-2;
			currentOffPmos[40] = 1.6e-2;
			currentOffPmos[50] = 1.6e-2;
			currentOffPmos[60] = 1.6e-2;
			currentOffPmos[70] = 1.6e-2;
			currentOffPmos[80] = 1.6e-2;
			currentOffPmos[90] = 1.6e-2;
			currentOffPmos[100] = 1.6e-2;
		} else if (_deviceRoadmap == LOP) {
			vdd = 1.5;
			vth = 600e-3;
			vpp = vdd + vth;
			phyGateLength = 0.16e-6;
			capIdealGate = 8e-10;
			capFringe = 2.5e-10;
			capJunction = 1.00e-3;
			capOx = 1e-2;
			effectiveElectronMobility = 320e-4;
			effectiveHoleMobility = 80e-4;
			pnSizeRatio = 2.45;						/* from CACTI */
			effectiveResistanceMultiplier = 1.54;	/* from CACTI */
			gm_oncurrent = 0.0; // dummy value only used for advanced nodes
			currentOnNmos[0] = 330;
			currentOnNmos[10] = 330;
			currentOnNmos[20] = 330;
			currentOnNmos[30] = 330;
			currentOnNmos[40] = 330;
			currentOnNmos[50] = 330;
			currentOnNmos[60] = 330;
			currentOnNmos[70] = 330;
			currentOnNmos[80] = 330;
			currentOnNmos[90] = 330;
			currentOnNmos[100] = 330;
			currentOnPmos[0] = 168;
			currentOnPmos[10] =  168;
			currentOnPmos[20] =  168;
			currentOnPmos[30] =  168;
			currentOnPmos[40] =  168;
			currentOnPmos[50] =  168;
			currentOnPmos[60] =  168;
			currentOnPmos[70] =  168;
			currentOnPmos[80] =  168;
			currentOnPmos[90] =  168;
			currentOnPmos[100] =  168;
			currentOffNmos[0] = 4.25e-6;
			currentOffNmos[10] = 4.25e-6;
			currentOffNmos[20] = 4.25e-6;
			currentOffNmos[30] = 4.25e-6;
			currentOffNmos[40] = 4.25e-6;
			currentOffNmos[50] = 4.25e-6;
			currentOffNmos[60] = 4.25e-6;
			currentOffNmos[70] = 4.25e-6;
			currentOffNmos[80] = 4.25e-6;
			currentOffNmos[90] = 4.25e-6;
			currentOffNmos[100] = 4.25e-6;
			currentOffPmos[0] = 8.5e-6;
			currentOffPmos[10] = 8.5e-6;
			currentOffPmos[20] = 8.5e-6;
			currentOffPmos[30] = 8.5e-6;
			currentOffPmos[40] = 8.5e-6;
			currentOffPmos[50] = 8.5e-6;
			currentOffPmos[60] = 8.5e-6;
			currentOffPmos[70] = 8.5e-6;
			currentOffPmos[80] = 8.5e-6;
			currentOffPmos[90] = 8.5e-6;
			currentOffPmos[100] = 8.5e-6;
		} else {
			vdd = 1.2;
			vth = 450e-3;
			vpp = vdd + vth;
			phyGateLength = 0.135e-6;
			capIdealGate = 8e-10;
			capFringe = 2.5e-10;
			capJunction = 1.00e-3;
			capOx = 1e-2;
			effectiveElectronMobility = 330e-4;
			effectiveHoleMobility = 90e-4;
			pnSizeRatio = 2.45;						/* from CACTI */
			effectiveResistanceMultiplier = 1.54;	/* from CACTI */
			gm_oncurrent = 0.0; // dummy value only used for advanced nodes
			currentOnNmos[0] = 490;
			currentOnNmos[10] = 490;
			currentOnNmos[20] = 490;
			currentOnNmos[30] = 490;
			currentOnNmos[40] = 490;
			currentOnNmos[50] = 490;
			currentOnNmos[60] = 490;
			currentOnNmos[70] = 490;
			currentOnNmos[80] = 490;
			currentOnNmos[90] = 490;
			currentOnNmos[100] = 490;
			currentOnPmos[0] = 230;
			currentOnPmos[10] = 230;
			currentOnPmos[20] = 230;
			currentOnPmos[30] = 230;
			currentOnPmos[40] = 230;
			currentOnPmos[50] = 230;
			currentOnPmos[60] = 230;
			currentOnPmos[70] = 230;
			currentOnPmos[80] = 230;
			currentOnPmos[90] = 230;
			currentOnPmos[100] = 230;
			currentOffNmos[0] = 4e-4;
			currentOffNmos[10] = 4e-4;
			currentOffNmos[20] = 4e-4;
			currentOffNmos[30] = 4e-4;
			currentOffNmos[40] = 4e-4;
			currentOffNmos[50] = 4e-4;
			currentOffNmos[60] = 4e-4;
			currentOffNmos[70] = 4e-4;
			currentOffNmos[80] = 4e-4;
			currentOffNmos[90] = 4e-4;
			currentOffNmos[100] = 4e-4;
			currentOffPmos[0] = 8e-4;
			currentOffPmos[10] = 8e-4;
			currentOffPmos[20] = 8e-4;
			currentOffPmos[30] = 8e-4;
			currentOffPmos[40] = 8e-4;
			currentOffPmos[50] = 8e-4;
			currentOffPmos[60] = 8e-4;
			currentOffPmos[70] = 8e-4;
			currentOffPmos[80] = 8e-4;
			currentOffPmos[90] = 8e-4;
			currentOffPmos[100] = 8e-4;
		}
	} else if (_featureSizeInNano >= 120) {
		if (_deviceRoadmap == HP) {
			/* PTM model: 130nm_HP.pm, from http://ptm.asu.edu/ */
			vdd = 1.3;
			vth = 128.4855e-3;
			vpp = vdd + vth;
			phyGateLength = 7.5e-8;
			capIdealGate = 6.058401e-10;
			capFringe = 6.119807e-10;
			capJunction = 1.00e-3;
			capOx = 1.64e-2;
			effectiveElectronMobility = 249.59e-4;
			effectiveHoleMobility = 59.52e-4;
			effectiveResistanceMultiplier = 1.54;	/* from CACTI */
			gm_oncurrent = 0.0; // dummy value only used for advanced nodes
			current_gmNmos=3.94E+02; 
			current_gmPmos=2.61E+02;
			currentOnNmos[0]   = 0.93e3; 
			currentOnNmos[10]  = 0.91e3; 
			currentOnNmos[20]  = 0.89e3; 
			currentOnNmos[30]  = 0.87e3; 
			currentOnNmos[40]  = 0.85e3; 
			currentOnNmos[50]  = 0.83e3; 
			currentOnNmos[60]  = 0.81e3; 
			currentOnNmos[70]  = 0.79e3; 
			currentOnNmos[80]  = 0.77e3; 
			currentOnNmos[90]  = 0.75e3; 
			currentOnNmos[100] = 0.74e3; 
			currentOnPmos[0]   = 0.43e3; 
			currentOnPmos[10]  = 0.41e3; 
			currentOnPmos[20]  = 0.38e3; 
			currentOnPmos[30]  = 0.36e3; 
			currentOnPmos[40]  = 0.34e3; 
			currentOnPmos[50]  = 0.32e3; 
			currentOnPmos[60]  = 0.30e3; 
			currentOnPmos[70]  = 0.28e3; 
			currentOnPmos[80]  = 0.26e3; 
			currentOnPmos[90]  = 0.25e3; 
			currentOnPmos[100] = 0.24e3; 
			currentOffNmos[0]  = 100.00e-3; 
			currentOffNmos[10] = 119.90e-3; 
			currentOffNmos[20] = 142.20e-3; 
			currentOffNmos[30] = 167.00e-3; 
			currentOffNmos[40] = 194.30e-3; 
			currentOffNmos[50] = 224.30e-3; 
			currentOffNmos[60] = 256.80e-3; 
			currentOffNmos[70] = 292.00e-3; 
			currentOffNmos[80] = 329.90e-3; 
			currentOffNmos[90] = 370.50e-3; 
			currentOffNmos[100]= 413.80e-3; 
			currentOffPmos[0]  = 100.20e-3; 
			currentOffPmos[10] = 113.60e-3; 
			currentOffPmos[20] = 127.90e-3; 
			currentOffPmos[30] = 143.10e-3; 
			currentOffPmos[40] = 159.10e-3; 
			currentOffPmos[50] = 175.80e-3; 
			currentOffPmos[60] = 193.40e-3; 
			currentOffPmos[70] = 211.70e-3; 
			currentOffPmos[80] = 230.80e-3; 
			currentOffPmos[90] = 250.70e-3; 
			currentOffPmos[100]= 271.20e-3;
			pnSizeRatio = currentOnNmos[0]/currentOnPmos[0];
		} else if (_deviceRoadmap == LOP) {
			/* PTM model: 130nm_LP.pm, from http://ptm.asu.edu/ */
			vdd = 1.3;
			vth = 466.0949e-3;
			vpp = vdd + vth;
			phyGateLength = 7.5e-8;
			capIdealGate = 1.8574e-9;
			capFringe = 9.530642e-10;
			cap_draintotal = capFringe/2;
			capJunction = 1.00e-3;
			capOx = 1.15e-2;
			effectiveElectronMobility = 284.97e-4;
			effectiveHoleMobility = 61.82e-4;
			effectiveResistanceMultiplier = 1.54;	/* from CACTI */
			gm_oncurrent = 0.0; // dummy value only used for advanced nodes
			current_gmNmos=3.87E+01;
			current_gmPmos=5.67E+01;			
			currentOnNmos[0]   = 300.70; 
			currentOnNmos[10]  = 273.40; 
			currentOnNmos[20]  = 249.40; 
			currentOnNmos[30]  = 228.40; 
			currentOnNmos[40]  = 209.90; 
			currentOnNmos[50]  = 193.50; 
			currentOnNmos[60]  = 179.00; 
			currentOnNmos[70]  = 166.00; 
			currentOnNmos[80]  = 154.40; 
			currentOnNmos[90]  = 144.00; 
			currentOnNmos[100] = 134.60; 
			currentOnPmos[0]   = 150.70;
			currentOnPmos[10]  = 136.20;
			currentOnPmos[20]  = 123.60;
			currentOnPmos[30]  = 112.70;
			currentOnPmos[40]  = 103.20;
			currentOnPmos[50]  = 94.88 ;
			currentOnPmos[60]  = 87.54 ;
			currentOnPmos[70]  = 81.04 ;
			currentOnPmos[80]  = 75.25 ;
			currentOnPmos[90]  = 70.08 ;
			currentOnPmos[100] = 65.44 ;
			currentOffNmos[0]  = 100.20e-6;
			currentOffNmos[10] = 135.90e-6;
			currentOffNmos[20] = 181.20e-6;
			currentOffNmos[30] = 237.80e-6;
			currentOffNmos[40] = 307.30e-6;
			currentOffNmos[50] = 391.90e-6;
			currentOffNmos[60] = 493.30e-6;
			currentOffNmos[70] = 613.70e-6;
			currentOffNmos[80] = 755.30e-6;
			currentOffNmos[90] = 920.20e-6;
			currentOffNmos[100]= 1111.0e-6;
			currentOffPmos[0]  = 100.20e-6; 
			currentOffPmos[10] = 132.80e-6; 
			currentOffPmos[20] = 173.00e-6; 
			currentOffPmos[30] = 221.90e-6; 
			currentOffPmos[40] = 280.70e-6; 
			currentOffPmos[50] = 350.40e-6; 
			currentOffPmos[60] = 432.20e-6; 
			currentOffPmos[70] = 527.20e-6; 
			currentOffPmos[80] = 636.80e-6; 
			currentOffPmos[90] = 761.90e-6; 
			currentOffPmos[100]= 903.80e-6;
			pnSizeRatio = currentOnNmos[0]/currentOnPmos[0];
		} else {
            cout << "Unknown device roadmap!" << endl;
            exit(1);
		}
	} else if (_featureSizeInNano >= 90) {
		if (_deviceRoadmap == HP) {
			/* PTM model: 90nm_HP.pm, from http://ptm.asu.edu/ */
			vdd = 1.2;
			vth = 146.0217e-3;
			vpp = vdd + vth;
			phyGateLength = 5.5e-8;
			capIdealGate = 5.694423e-10;
			capFringe = 5.652302e-10;
			capJunction = 1.00e-3;
			capOx = 1.73e-2;
			effectiveElectronMobility = 243.43e-4;
			effectiveHoleMobility = 58.32e-4;
			effectiveResistanceMultiplier = 1.54;	/* from CACTI */
			gm_oncurrent = 0.0; // dummy value only used for advanced nodes
			current_gmNmos=4.95E+02;
			current_gmPmos=3.16E+02;			
			currentOnNmos[0]   = 1.07e3;
			currentOnNmos[10]  = 1.05e3;
			currentOnNmos[20]  = 1.03e3;
			currentOnNmos[30]  = 1.01e3;
			currentOnNmos[40]  = 0.99e3;
			currentOnNmos[50]  = 0.97e3;
			currentOnNmos[60]  = 0.95e3;
			currentOnNmos[70]  = 0.93e3;
			currentOnNmos[80]  = 0.90e3;
			currentOnNmos[90]  = 0.88e3;
			currentOnNmos[100] = 0.86e3;
			currentOnPmos[0]   = 0.54e3; 
			currentOnPmos[10]  = 0.50e3; 
			currentOnPmos[20]  = 0.47e3; 
			currentOnPmos[30]  = 0.44e3; 
			currentOnPmos[40]  = 0.41e3; 
			currentOnPmos[50]  = 0.39e3; 
			currentOnPmos[60]  = 0.37e3; 
			currentOnPmos[70]  = 0.34e3; 
			currentOnPmos[80]  = 0.32e3; 
			currentOnPmos[90]  = 0.31e3; 
			currentOnPmos[100] = 0.29e3; 
			currentOffNmos[0]  = 100.8e-3;	
			currentOffNmos[10] = 120.8e-3;	
			currentOffNmos[20] = 143.4e-3;	
			currentOffNmos[30] = 168.6e-3;	
			currentOffNmos[40] = 196.6e-3;	
			currentOffNmos[50] = 227.4e-3;	
			currentOffNmos[60] = 261.1e-3;	
			currentOffNmos[70] = 297.7e-3;	
			currentOffNmos[80] = 337.3e-3;	
			currentOffNmos[90] = 379.8e-3;	
			currentOffNmos[100]= 425.4e-3;	
			currentOffPmos[0]  = 100.00e-3;
			currentOffPmos[10] = 114.00e-3;
			currentOffPmos[20] = 128.90e-3;
			currentOffPmos[30] = 144.80e-3;
			currentOffPmos[40] = 161.60e-3;
			currentOffPmos[50] = 179.30e-3;
			currentOffPmos[60] = 197.90e-3;
			currentOffPmos[70] = 217.40e-3;
			currentOffPmos[80] = 237.90e-3;
			currentOffPmos[90] = 259.10e-3;
			currentOffPmos[100]= 281.30e-3;
			pnSizeRatio = currentOnNmos[0]/currentOnPmos[0];
		} else if (_deviceRoadmap == LOP) {
			/* PTM model: 90nm_LP.pm, from http://ptm.asu.edu/ */
			vdd = 1.2;
			vth = 501.3229e-3;
			vpp = vdd + vth;
			phyGateLength = 5.5e-8;
			capIdealGate = 1.5413e-9;
			capFringe = 9.601334e-10;
			capJunction = 1.00e-3;
			capOx = 1.19e-2;
			effectiveElectronMobility = 277.94e-4;
			effectiveHoleMobility = 60.64e-4;
			effectiveResistanceMultiplier = 1.77;	/* from CACTI */
			gm_oncurrent = 0.0; // dummy value only used for advanced nodes
			current_gmNmos=4.38E+01;
			current_gmPmos=5.99E+01;			
			currentOnNmos[0]   = 346.30 ;
			currentOnNmos[10]  = 314.50 ;
			currentOnNmos[20]  = 286.80 ;
			currentOnNmos[30]  = 262.50 ;
			currentOnNmos[40]  = 241.20 ;
			currentOnNmos[50]  = 222.30 ;
			currentOnNmos[60]  = 205.60 ;
			currentOnNmos[70]  = 190.80 ;
			currentOnNmos[80]  = 177.50 ;
			currentOnNmos[90]  = 165.60 ;
			currentOnNmos[100] = 155.00 ;
			currentOnPmos[0]   = 200.30 ;
			currentOnPmos[10]  = 179.50 ;
			currentOnPmos[20]  = 161.90 ;
			currentOnPmos[30]  = 146.90 ;
			currentOnPmos[40]  = 133.90 ;
			currentOnPmos[50]  = 122.60 ;
			currentOnPmos[60]  = 112.80 ;
			currentOnPmos[70]  = 104.10 ;
			currentOnPmos[80]  = 96.47  ;
			currentOnPmos[90]  = 89.68  ;
			currentOnPmos[100] = 83.62  ;
			currentOffNmos[0]  = 100.00e-6;
			currentOffNmos[10] = 135.70e-6;
			currentOffNmos[20] = 181.10e-6;
			currentOffNmos[30] = 238.00e-6;
			currentOffNmos[40] = 308.50e-6;
			currentOffNmos[50] = 394.60e-6;
			currentOffNmos[60] = 498.50e-6;
			currentOffNmos[70] = 622.60e-6;
			currentOffNmos[80] = 769.30e-6;
			currentOffNmos[90] = 941.20e-6;
			currentOffNmos[100]= 1141.0e-6;
			currentOffPmos[0]  = 100.30e-6;
			currentOffPmos[10] = 133.20e-6;
			currentOffPmos[20] = 174.20e-6;
			currentOffPmos[30] = 224.40e-6;
			currentOffPmos[40] = 285.10e-6;
			currentOffPmos[50] = 357.60e-6;
			currentOffPmos[60] = 443.40e-6;
			currentOffPmos[70] = 543.70e-6;
			currentOffPmos[80] = 660.00e-6;
			currentOffPmos[90] = 793.80e-6;
			currentOffPmos[100]= 946.40e-6;
			pnSizeRatio = currentOnNmos[0]/currentOnPmos[0];
		} else {
            cout << "Unknown device roadmap!" << endl;
            exit(1);
		}
	} else if (_featureSizeInNano >= 65) {
		if (_deviceRoadmap == HP) {
			/* PTM model: 65nm_HP.pm, from http://ptm.asu.edu/ */
			vdd = 1.1;
			vth = 166.3941e-3;
			vpp = vdd + vth;
			phyGateLength = 3.5e-8;
			capIdealGate = 4.868295e-10;
			capFringe = 5.270361e-10;
			cap_draintotal = capFringe/2;
			capJunction = 1.00e-3;
			capOx = 1.88e-2;
			effectiveElectronMobility = 445.74e-4;
			effectiveHoleMobility = 113.330e-4;
			effectiveResistanceMultiplier = 1.54;	/* from CACTI */
			gm_oncurrent = 0.0; // dummy value only used for advanced nodes
			current_gmNmos=5.72E+02;
			current_gmPmos=3.99E+02;			
			currentOnNmos[0]   = 1.12e3;
			currentOnNmos[10]  = 1.10e3;
			currentOnNmos[20]  = 1.08e3;
			currentOnNmos[30]  = 1.06e3;
			currentOnNmos[40]  = 1.04e3;
			currentOnNmos[50]  = 1.02e3;
			currentOnNmos[60]  = 1.00e3;
			currentOnNmos[70]  = 0.98e3;
			currentOnNmos[80]  = 0.95e3;
			currentOnNmos[90]  = 0.93e3;
			currentOnNmos[100] = 0.91e3;
			currentOnPmos[0]   = 0.70e3; 
			currentOnPmos[10]  = 0.66e3; 
			currentOnPmos[20]  = 0.62e3; 
			currentOnPmos[30]  = 0.58e3; 
			currentOnPmos[40]  = 0.55e3; 
			currentOnPmos[50]  = 0.52e3; 
			currentOnPmos[60]  = 0.49e3; 
			currentOnPmos[70]  = 0.46e3; 
			currentOnPmos[80]  = 0.44e3; 
			currentOnPmos[90]  = 0.41e3; 
			currentOnPmos[100] = 0.39e3; 
			currentOffNmos[0]  = 100.00e-3;	
			currentOffNmos[10] = 119.70e-3;	
			currentOffNmos[20] = 141.90e-3;	
			currentOffNmos[30] = 166.80e-3;	
			currentOffNmos[40] = 194.40e-3;	
			currentOffNmos[50] = 224.80e-3;	
			currentOffNmos[60] = 258.10e-3;	
			currentOffNmos[70] = 294.40e-3;	
			currentOffNmos[80] = 333.60e-3;	
			currentOffNmos[90] = 375.90e-3;	
			currentOffNmos[100]= 421.20e-3;	
			currentOffPmos[0]  = 100.10e-3;
			currentOffPmos[10] = 115.20e-3;
			currentOffPmos[20] = 131.50e-3;
			currentOffPmos[30] = 149.00e-3;
			currentOffPmos[40] = 167.60e-3;
			currentOffPmos[50] = 187.40e-3;
			currentOffPmos[60] = 208.40e-3;
			currentOffPmos[70] = 230.50e-3;
			currentOffPmos[80] = 253.70e-3;
			currentOffPmos[90] = 278.10e-3;
			currentOffPmos[100]= 303.60e-3;
			pnSizeRatio = currentOnNmos[0]/currentOnPmos[0];
		} else if (_deviceRoadmap == LOP) {
			/* PTM model: 65nm_LP.pm, from http://ptm.asu.edu/ */
			vdd = 1.1;
			vth = 501.6636e-3;
			vpp = vdd + vth;
			phyGateLength = 3.5e-8;
			capIdealGate = 1.1926e-9;
			capFringe = 9.62148e-10;
			capJunction = 1.00e-3;
			capOx = 1.37e-2;
			effectiveElectronMobility = 457.86e-4;
			effectiveHoleMobility = 102.64e-4;
			effectiveResistanceMultiplier = 1.77;	/* from CACTI */
			gm_oncurrent = 0.0; // dummy value only used for advanced nodes
			current_gmNmos=5.90E+01;
			current_gmPmos=6.75E+01;			
			currentOnNmos[0]   = 400.00 ;
			currentOnNmos[10]  = 363.90 ;
			currentOnNmos[20]  = 332.30 ;
			currentOnNmos[30]  = 304.70 ;
			currentOnNmos[40]  = 280.40 ;
			currentOnNmos[50]  = 258.90 ;
			currentOnNmos[60]  = 239.90 ;
			currentOnNmos[70]  = 223.00 ;
			currentOnNmos[80]  = 207.90 ;
			currentOnNmos[90]  = 194.30 ;
			currentOnNmos[100] = 182.10 ;
			currentOnPmos[0]   = 238.70 ;
			currentOnPmos[10]  = 216.10 ;
			currentOnPmos[20]  = 196.60 ;
			currentOnPmos[30]  = 179.70 ;
			currentOnPmos[40]  = 164.90 ;
			currentOnPmos[50]  = 152.00 ;
			currentOnPmos[60]  = 140.50 ;
			currentOnPmos[70]  = 130.40 ;
			currentOnPmos[80]  = 121.40 ;
			currentOnPmos[90]  = 113.30 ;
			currentOnPmos[100] = 106.10 ;
			currentOffNmos[0]  = 100.20e-6;
			currentOffNmos[10] = 137.50e-6;
			currentOffNmos[20] = 185.80e-6;
			currentOffNmos[30] = 247.20e-6;
			currentOffNmos[40] = 324.20e-6;
			currentOffNmos[50] = 419.30e-6;
			currentOffNmos[60] = 535.40e-6;
			currentOffNmos[70] = 675.70e-6;
			currentOffNmos[80] = 843.100e-6;
			currentOffNmos[90] = 1041.00e-6;
			currentOffNmos[100]= 1273.00e-6;
			currentOffPmos[0]  = 100.20e-6;
			currentOffPmos[10] = 135.40e-6;
			currentOffPmos[20] = 179.70e-6;
			currentOffPmos[30] = 234.90e-6;
			currentOffPmos[40] = 302.50e-6;
			currentOffPmos[50] = 384.30e-6;
			currentOffPmos[60] = 482.20e-6;
			currentOffPmos[70] = 598.00e-6;
			currentOffPmos[80] = 733.90e-6;
			currentOffPmos[90] = 891.60e-6;
			currentOffPmos[100]= 1073.00e-6;
			pnSizeRatio = currentOnNmos[0]/currentOnPmos[0];
		} else if (_deviceRoadmap == IGZO) { // Largernode dummy for interpolation only
			/* IGZO TFT - virtual source model derived */
			vdd = 1.4;
			vpp = 1.4;
			vth = 0.667;
			phyGateLength = 0.044e-6;
			capIdealGate = 8.93e-16;
			capFringe = 6.51e-17;
			capOx = 2.03e-2;
			effectiveElectronMobility = 10.0e-4; // μ in cm^2/Vs
			effectiveHoleMobility = 0.0;
			pnSizeRatio = 1.0;
			effectiveResistanceMultiplier = 5.0;
			gm_oncurrent = 0.0; // dummy value only used for advanced nodes
		
			// NMOS ON-current, Unit: μA/μm 
			currentOnNmos[0] = 155.6;
			currentOnNmos[10] = 153.0;
			currentOnNmos[20] = 150.1;
			currentOnNmos[30] = 147.6;
			currentOnNmos[40] = 145.2;
			currentOnNmos[50] = 142.7;
			currentOnNmos[60] = 140.2;
			currentOnNmos[70] = 137.6;
			currentOnNmos[80] = 135.1;
			currentOnNmos[90] = 132.5;
			currentOnNmos[100] = 130.0;
		
			// PMOS ON-current (IGZO has no PMOS)
			for (int i = 0; i <= 100; i += 10) {
				currentOnPmos[i] = 0.0;
			}
		
			// NMOS off current, Unit: A/m
			currentOffNmos[0] = 6.79e-7;   
			currentOffNmos[10] = 1.358e-6;  
			currentOffNmos[20] = 2.716e-6;  
			currentOffNmos[30] = 5.432e-6;  
			currentOffNmos[40] = 1.0864e-5; 
			currentOffNmos[50] = 2.1728e-5; 
			currentOffNmos[60] = 4.3456e-5; 
			currentOffNmos[70] = 8.6912e-5; 
			currentOffNmos[80] = 1.73824e-4;
			currentOffNmos[90] = 3.47648e-4;
			currentOffNmos[100] = 6.95296e-4;
		
			// PMOS off current (N/A in IGZO)
			for (int i = 0; i <= 100; i += 10) {
				currentOffPmos[i] = 0.0;
			}
		} else {
            cout << "Unknown device roadmap!" << endl;
            exit(1);
		}
	} else if (_featureSizeInNano >= 45) {
		if (_deviceRoadmap == HP) {
			/* PTM model: 45nm_HP.pm, from http://ptm.asu.edu/ */
			vdd = 1.0;
			vth = 171.0969e-3;
			vpp = vdd + vth;
			phyGateLength = 3.0e-8;
			capIdealGate = 4.091305e-10;
			capFringe = 4.957928e-10;
			capJunction = 1.00e-3;
			capOx = 3.77e-2;
			effectiveElectronMobility = 297.70e-4;
			effectiveHoleMobility = 95.27e-4;
			effectiveResistanceMultiplier = 1.54;	/* from CACTI */
			gm_oncurrent = 0.0; // dummy value only used for advanced nodes
			current_gmNmos=7.37E+02;
			current_gmPmos=6.30E+02;			
			currentOnNmos[0]   = 1.27e3;
			currentOnNmos[10]  = 1.24e3;
			currentOnNmos[20]  = 1.22e3;
			currentOnNmos[30]  = 1.19e3;
			currentOnNmos[40]  = 1.16e3;
			currentOnNmos[50]  = 1.13e3;
			currentOnNmos[60]  = 1.11e3;
			currentOnNmos[70]  = 1.08e3;
			currentOnNmos[80]  = 1.05e3;
			currentOnNmos[90]  = 1.02e3;
			currentOnNmos[100] = 1.00e3;
			currentOnPmos[0]   = 1.08e3; 
			currentOnPmos[10]  = 1.04e3; 
			currentOnPmos[20]  = 1.00e3; 
			currentOnPmos[30]  = 0.96e3; 
			currentOnPmos[40]  = 0.92e3; 
			currentOnPmos[50]  = 0.88e3; 
			currentOnPmos[60]  = 0.85e3; 
			currentOnPmos[70]  = 0.81e3; 
			currentOnPmos[80]  = 0.78e3; 
			currentOnPmos[90]  = 0.75e3; 
			currentOnPmos[100] = 0.72e3; 
			currentOffNmos[0]  = 100.00e-3;	
			currentOffNmos[10] = 120.70e-3;	
			currentOffNmos[20] = 144.10e-3;	
			currentOffNmos[30] = 170.50e-3;	
			currentOffNmos[40] = 199.80e-3;	
			currentOffNmos[50] = 232.30e-3;	
			currentOffNmos[60] = 268.00e-3;	
			currentOffNmos[70] = 307.10e-3;	
			currentOffNmos[80] = 349.50e-3;	
			currentOffNmos[90] = 395.40e-3;	
			currentOffNmos[100]= 444.80e-3;	
			currentOffPmos[0]  = 100.20e-3;
			currentOffPmos[10] = 118.70e-3;
			currentOffPmos[20] = 139.30e-3;
			currentOffPmos[30] = 162.00e-3;
			currentOffPmos[40] = 186.80e-3;
			currentOffPmos[50] = 213.90e-3;
			currentOffPmos[60] = 243.30e-3;
			currentOffPmos[70] = 274.90e-3;
			currentOffPmos[80] = 308.90e-3;
			currentOffPmos[90] = 345.20e-3;
			currentOffPmos[100]= 383.80e-3;
			pnSizeRatio = currentOnNmos[0]/currentOnPmos[0];
		} else if (_deviceRoadmap == LOP) {
			/* PTM model: 45nm_LP.pm, from http://ptm.asu.edu/ */
			vdd = 1;
			vth = 464.3718e-3;
			vpp = vdd + vth;
			phyGateLength = 3.0e-8;
			capIdealGate = 8.930709e-10;
			capFringe = 8.849901e-10;
			capJunction = 1.00e-3;
			capOx = 1.99e-2;
			effectiveElectronMobility = 456.14e-4;
			effectiveHoleMobility = 96.98e-4;
			effectiveResistanceMultiplier = 1.77;	/* from CACTI */
			gm_oncurrent = 0.0; // dummy value only used for advanced nodes
			current_gmNmos=1.32E+02;
			current_gmPmos=8.65E+01;			
			currentOnNmos[0]   = 500.20 ;
			currentOnNmos[10]  = 462.00 ;
			currentOnNmos[20]  = 427.80 ;
			currentOnNmos[30]  = 397.10 ;
			currentOnNmos[40]  = 369.40 ;
			currentOnNmos[50]  = 344.50 ;
			currentOnNmos[60]  = 322.10 ;
			currentOnNmos[70]  = 301.80 ;
			currentOnNmos[80]  = 283.40 ;
			currentOnNmos[90]  = 266.70 ;
			currentOnNmos[100] = 251.50 ;
			currentOnPmos[0]   = 300.00 ;
			currentOnPmos[10]  = 275.70 ;
			currentOnPmos[20]  = 254.20 ;
			currentOnPmos[30]  = 235.10 ;
			currentOnPmos[40]  = 218.10 ;
			currentOnPmos[50]  = 202.80 ;
			currentOnPmos[60]  = 189.20 ;
			currentOnPmos[70]  = 176.90 ;
			currentOnPmos[80]  = 165.80 ;
			currentOnPmos[90]  = 155.80 ;
			currentOnPmos[100] = 146.70 ;
			currentOffNmos[0]  = 100.00e-6;
			currentOffNmos[10] = 140.50e-6;
			currentOffNmos[20] = 193.90e-6;
			currentOffNmos[30] = 263.10e-6;
			currentOffNmos[40] = 351.40e-6;
			currentOffNmos[50] = 462.50e-6;
			currentOffNmos[60] = 600.30e-6;
			currentOffNmos[70] = 769.20e-6;
			currentOffNmos[80] = 973.900e-6;
			currentOffNmos[90] = 1219.00e-6;
			currentOffNmos[100]= 1511.00e-6;
			currentOffPmos[0]  = 100.20e-6;
			currentOffPmos[10] = 138.40e-6;
			currentOffPmos[20] = 187.60e-6;
			currentOffPmos[30] = 250.10e-6;
			currentOffPmos[40] = 328.10e-6;
			currentOffPmos[50] = 424.10e-6;
			currentOffPmos[60] = 540.90e-6;
			currentOffPmos[70] = 681.30e-6;
			currentOffPmos[80] = 848.30e-6;
			currentOffPmos[90] = 1045.00e-6;
			currentOffPmos[100]= 1275.00e-6;
			pnSizeRatio = currentOnNmos[0]/currentOnPmos[0];
		} else if (_deviceRoadmap == IGZO) {
			/* IGZO TFT - virtual source model derived */
			vdd = 1.4;
			vpp = 1.4;
			vth = 0.667;
			phyGateLength = 0.044e-6;
			capIdealGate = 8.93e-16;
			capFringe = 6.51e-17;
			capOx = 2.03e-2;
			effectiveElectronMobility = 10.0e-4; // μ in cm^2/Vs
			effectiveHoleMobility = 0.0;
			pnSizeRatio = 1.0;
			effectiveResistanceMultiplier = 5.0;
			gm_oncurrent = 0.0; // dummy value only used for advanced nodes
		
			// NMOS ON-current, Unit: μA/μm approximately scaled with temperature
			currentOnNmos[0] = 155.6;
			currentOnNmos[10] = 153.0;
			currentOnNmos[20] = 150.1;
			currentOnNmos[30] = 147.6;
			currentOnNmos[40] = 145.2;
			currentOnNmos[50] = 142.7;
			currentOnNmos[60] = 140.2;
			currentOnNmos[70] = 137.6;
			currentOnNmos[80] = 135.1;
			currentOnNmos[90] = 132.5;
			currentOnNmos[100] = 130.0;
		
			// PMOS ON-current (IGZO has no PMOS)
			for (int i = 0; i <= 100; i += 10) {
				currentOnPmos[i] = 0.0;
			}
		
			// NMOS off current, Unit: A/m
			currentOffNmos[0] = 6.79e-7;   
			currentOffNmos[10] = 1.358e-6;  
			currentOffNmos[20] = 2.716e-6;  
			currentOffNmos[30] = 5.432e-6;  
			currentOffNmos[40] = 1.0864e-5; 
			currentOffNmos[50] = 2.1728e-5; 
			currentOffNmos[60] = 4.3456e-5; 
			currentOffNmos[70] = 8.6912e-5; 
			currentOffNmos[80] = 1.73824e-4;
			currentOffNmos[90] = 3.47648e-4;
			currentOffNmos[100] = 6.95296e-4;
		
			// PMOS off current (N/A in IGZO)
			for (int i = 0; i <= 100; i += 10) {
				currentOffPmos[i] = 0.0;
			}
		} else {
            cout << "Unknown device roadmap!" << endl;
            exit(1);
		}
	} else if (_featureSizeInNano >= 32) {	/* TO-DO: actually 36nm */
		if (_deviceRoadmap == HP) {
			/* PTM model: 32nm_HP.pm, from http://ptm.asu.edu/ */
			vdd = 0.9;
			vth = 194.4951e-3;
			vpp = vdd + vth;
			phyGateLength = 2.8e-8;
			capIdealGate = 3.767721e-10;
			capFringe = 4.713762e-10;
			capJunction = 1.00e-3;
			capOx = 4.59e-2;
			effectiveElectronMobility = 257.73e-4;
			effectiveHoleMobility = 89.92e-4;
			effectiveResistanceMultiplier = 1.54;	/* from CACTI */
			gm_oncurrent = 0.0; // dummy value only used for advanced nodes
			current_gmNmos=9.29E+02;
			current_gmPmos=6.73E+02;			
			currentOnNmos[0]   = 1.41e3;
			currentOnNmos[10]  = 1.38e3;
			currentOnNmos[20]  = 1.35e3;
			currentOnNmos[30]  = 1.31e3;
			currentOnNmos[40]  = 1.28e3;
			currentOnNmos[50]  = 1.25e3;
			currentOnNmos[60]  = 1.21e3;
			currentOnNmos[70]  = 1.18e3;
			currentOnNmos[80]  = 1.15e3;
			currentOnNmos[90]  = 1.12e3;
			currentOnNmos[100] = 1.08e3;
			currentOnPmos[0]   = 1.22e3; 
			currentOnPmos[10]  = 1.17e3; 
			currentOnPmos[20]  = 1.12e3; 
			currentOnPmos[30]  = 1.07e3; 
			currentOnPmos[40]  = 1.02e3; 
			currentOnPmos[50]  = 0.98e3; 
			currentOnPmos[60]  = 0.94e3; 
			currentOnPmos[70]  = 0.89e3; 
			currentOnPmos[80]  = 0.86e3; 
			currentOnPmos[90]  = 0.82e3; 
			currentOnPmos[100] = 0.78e3; 
			currentOffNmos[0]  = 100.30e-3;	
			currentOffNmos[10] = 120.40e-3;	
			currentOffNmos[20] = 143.10e-3;	
			currentOffNmos[30] = 168.60e-3;	
			currentOffNmos[40] = 197.00e-3;	
			currentOffNmos[50] = 228.40e-3;	
			currentOffNmos[60] = 262.90e-3;	
			currentOffNmos[70] = 300.60e-3;	
			currentOffNmos[80] = 341.70e-3;	
			currentOffNmos[90] = 386.10e-3;	
			currentOffNmos[100]= 433.90e-3;	
			currentOffPmos[0]  = 100.10e-3;
			currentOffPmos[10] = 119.00e-3;
			currentOffPmos[20] = 140.00e-3;
			currentOffPmos[30] = 163.30e-3;
			currentOffPmos[40] = 188.80e-3;
			currentOffPmos[50] = 216.70e-3;
			currentOffPmos[60] = 247.00e-3;
			currentOffPmos[70] = 279.70e-3;
			currentOffPmos[80] = 314.90e-3;
			currentOffPmos[90] = 352.60e-3;
			currentOffPmos[100]= 392.80e-3;
			pnSizeRatio = currentOnNmos[0]/currentOnPmos[0];
		} else if (_deviceRoadmap == LOP) {
			/* PTM model: 32nm_LP.pm, from http://ptm.asu.edu/ */
			vdd = 0.9;
			vth = 442.034e-3;
			vpp = vdd + vth;
			phyGateLength = 2.8e-8;
			capIdealGate = 8.375279e-10;
			capFringe = 6.856677e-10;
			capJunction = 1.00e-3;
			capOx = 2.19e-2;
			effectiveElectronMobility = 395.20e-4;
			effectiveHoleMobility = 88.67e-4;
			effectiveResistanceMultiplier = 1.77;	/* from CACTI */
			gm_oncurrent = 0.0; // dummy value only used for advanced nodes
			current_gmNmos=2.56E+02;
			current_gmPmos=1.19E+02;			
			currentOnNmos[0]   =600.20;
			currentOnNmos[10]  =562.80;
			currentOnNmos[20]  =528.20;
			currentOnNmos[30]  =496.20;
			currentOnNmos[40]  =466.80;
			currentOnNmos[50]  =439.70;
			currentOnNmos[60]  =414.80;
			currentOnNmos[70]  =391.90;
			currentOnNmos[80]  =370.70;
			currentOnNmos[90]  =351.30;
			currentOnNmos[100] =333.30;
			currentOnPmos[0]   = 400.00;
			currentOnPmos[10]  = 368.40;
			currentOnPmos[20]  = 340.30;
			currentOnPmos[30]  = 315.30;
			currentOnPmos[40]  = 292.90;
			currentOnPmos[50]  = 272.80;
			currentOnPmos[60]  = 254.80;
			currentOnPmos[70]  = 238.50;
			currentOnPmos[80]  = 223.80;
			currentOnPmos[90]  = 210.50;
			currentOnPmos[100] = 198.40;
			currentOffNmos[0]  = 100.10e-6;
			currentOffNmos[10] = 143.60e-6;
			currentOffNmos[20] = 202.10e-6;
			currentOffNmos[30] = 279.30e-6;
			currentOffNmos[40] = 379.50e-6;
			currentOffNmos[50] = 507.50e-6;
			currentOffNmos[60] = 668.80e-6;
			currentOffNmos[70] = 869.20e-6;
			currentOffNmos[80] = 1115.00e-6;
			currentOffNmos[90] = 1415.00e-6;
			currentOffNmos[100]= 1774.00e-6;
			currentOffPmos[0]  = 100.10e-6;
			currentOffPmos[10] = 140.70e-6;
			currentOffPmos[20] = 194.00e-6;
			currentOffPmos[30] = 262.50e-6;
			currentOffPmos[40] = 349.30e-6;
			currentOffPmos[50] = 457.70e-6;
			currentOffPmos[60] = 591.20e-6;
			currentOffPmos[70] = 753.70e-6;
			currentOffPmos[80] = 949.30e-6;
			currentOffPmos[90] = 1182.00e-6;
			currentOffPmos[100]= 1457.00e-6;
			pnSizeRatio = currentOnNmos[0]/currentOnPmos[0];
		} else if (_deviceRoadmap == CNT) { // Larger node dummy for interpolation
			/* CNFET @ 20nm gate length */
			vdd = 0.7;
			vth = 0.18;
			vpp = vdd + vth;
			phyGateLength = 0.020e-6;
			capIdealGate = 1.18e-15;
			capFringe = 4.18e-17;
			capOx = 5.90e-2;
			effectiveElectronMobility = 78700e-4;
			effectiveHoleMobility = 78700e-4;
			pnSizeRatio = 1.0;
			effectiveResistanceMultiplier = 1.3;
			gm_oncurrent = 0.0;

			currentOnNmos[0]   = 3361.1;
			currentOnNmos[10]  = 3300.0;
			currentOnNmos[20]  = 3240.0;
			currentOnNmos[30]  = 3180.0;
			currentOnNmos[40]  = 3120.0;
			currentOnNmos[50]  = 3060.0;
			currentOnNmos[60]  = 3000.0;
			currentOnNmos[70]  = 2940.0;
			currentOnNmos[80]  = 2880.0;
			currentOnNmos[90]  = 2820.0;
			currentOnNmos[100] = 2760.0;

			/* PMOS ON-current (same as NMOS if symmetrical) */
			currentOnPmos[0]   = currentOnNmos[0];
			currentOnPmos[10]  = currentOnNmos[10];
			currentOnPmos[20]  = currentOnNmos[20];
			currentOnPmos[30]  = currentOnNmos[30];
			currentOnPmos[40]  = currentOnNmos[40];
			currentOnPmos[50]  = currentOnNmos[50];
			currentOnPmos[60]  = currentOnNmos[60];
			currentOnPmos[70]  = currentOnNmos[70];
			currentOnPmos[80]  = currentOnNmos[80];
			currentOnPmos[90]  = currentOnNmos[90];
			currentOnPmos[100] = currentOnNmos[100];

			currentOffNmos[0]   = 8.94e-2;
			currentOffNmos[10]  = 1.00e-1;
			currentOffNmos[20]  = 1.12e-1;
			currentOffNmos[30]  = 1.26e-1;
			currentOffNmos[40]  = 1.41e-1;
			currentOffNmos[50]  = 1.57e-1;
			currentOffNmos[60]  = 1.75e-1;
			currentOffNmos[70]  = 1.94e-1;
			currentOffNmos[80]  = 2.15e-1;
			currentOffNmos[90]  = 2.37e-1;
			currentOffNmos[100] = 2.60e-1;

			/* PMOS OFF-current (same as NMOS if symmetrical) */
			currentOffPmos[0]   = currentOffNmos[0];
			currentOffPmos[10]  = currentOffNmos[10];
			currentOffPmos[20]  = currentOffNmos[20];
			currentOffPmos[30]  = currentOffNmos[30];
			currentOffPmos[40]  = currentOffNmos[40];
			currentOffPmos[50]  = currentOffNmos[50];
			currentOffPmos[60]  = currentOffNmos[60];
			currentOffPmos[70]  = currentOffNmos[70];
			currentOffPmos[80]  = currentOffNmos[80];
			currentOffPmos[90]  = currentOffNmos[90];
			currentOffPmos[100] = currentOffNmos[100];
		} else {
            cout << "Unknown device roadmap!" << endl;
            exit(1);
		}
	} else if (_featureSizeInNano >= 22) {
		if (_deviceRoadmap == HP) {
			/* PTM model: 22nm.pm, from http://ptm.asu.edu/ */
			vdd = 0.85;
			vth = 208.9006e-3;
			vpp = vdd + vth;
			phyGateLength = 2.6e-8;
			capIdealGate = 3.287e-10;
			capFringe = 4.532e-10;
			capOx = 3.83e-2;
			effectiveElectronMobility = 397.26e-4;
			effectiveHoleMobility = 83.60e-4;
			effectiveResistanceMultiplier = 1.54;	/* from CACTI */
			gm_oncurrent = 0.0; // dummy value only used for advanced nodes
			current_gmNmos=1.08E+03;
			current_gmPmos=6.98E+02;			
			currentOnNmos[0]   = 1.50e3;
			currentOnNmos[10]  = 1.47e3;
			currentOnNmos[20]  = 1.43e3;
			currentOnNmos[30]  = 1.39e3;
			currentOnNmos[40]  = 1.35e3;
			currentOnNmos[50]  = 1.31e3;
			currentOnNmos[60]  = 1.28e3;
			currentOnNmos[70]  = 1.24e3;
			currentOnNmos[80]  = 1.20e3;
			currentOnNmos[90]  = 1.17e3;
			currentOnNmos[100] = 1.13e3;
			currentOnPmos[0]   = 1.32e3;
			currentOnPmos[10]  = 1.25e3;
			currentOnPmos[20]  = 1.19e3;
			currentOnPmos[30]  = 1.13e3;
			currentOnPmos[40]  = 1.07e3;
			currentOnPmos[50]  = 1.02e3;
			currentOnPmos[60]  = 0.97e3;
			currentOnPmos[70]  = 0.92e3;
			currentOnPmos[80]  = 0.88e3;
			currentOnPmos[90]  = 0.84e3;
			currentOnPmos[100] = 0.80e3;
			currentOffNmos[0]  = 100.20e-3;	
			currentOffNmos[10] = 120.40e-3;	
			currentOffNmos[20] = 143.50e-3;	
			currentOffNmos[30] = 169.50e-3;	
			currentOffNmos[40] = 198.70e-3;	
			currentOffNmos[50] = 231.20e-3;	
			currentOffNmos[60] = 267.00e-3;	
			currentOffNmos[70] = 306.30e-3;	
			currentOffNmos[80] = 349.30e-3;	
			currentOffNmos[90] = 396.00e-3;	
			currentOffNmos[100]= 446.60e-3;	
			currentOffPmos[0]  = 100.20e-3;
			currentOffPmos[10] = 119.40e-3;
			currentOffPmos[20] = 140.80e-3;
			currentOffPmos[30] = 164.60e-3;
			currentOffPmos[40] = 190.90e-3;
			currentOffPmos[50] = 219.50e-3;
			currentOffPmos[60] = 250.70e-3;
			currentOffPmos[70] = 284.50e-3;
			currentOffPmos[80] = 320.90e-3;
			currentOffPmos[90] = 359.80e-3;
			currentOffPmos[100]= 401.50e-3;
			pnSizeRatio = currentOnNmos[0]/currentOnPmos[0];
		} else if (_deviceRoadmap == LOP) {
			/* PTM model: 22nm_LP.pm, from http://ptm.asu.edu/ */
			vdd = 0.85;
			vth = 419.915e-3;
			vpp = vdd + vth;
			phyGateLength = 2.6e-8; 	
			capIdealGate = 5.245e-10;
			capFringe = 8.004e-10;
			capOx = 2.65e-2;
			effectiveElectronMobility = 731.29e-4;
			effectiveHoleMobility = 111.22e-4;
			effectiveResistanceMultiplier = 1.77;	/* from CACTI */
			gm_oncurrent = 0.0; // dummy value only used for advanced nodes
			current_gmNmos=4.56E+02;
			current_gmPmos=1.85E+02;			
			currentOnNmos[0]   = 791.90;
			currentOnNmos[10]  = 756.40;
			currentOnNmos[20]  = 722.20;
			currentOnNmos[30]  = 689.40;
			currentOnNmos[40]  = 658.10;
			currentOnNmos[50]  = 628.30;
			currentOnNmos[60]  = 600.00;
			currentOnNmos[70]  = 573.30;
			currentOnNmos[80]  = 548.00;
			currentOnNmos[90]  = 524.20;
			currentOnNmos[100] = 501.70;
			currentOnPmos[0]   = 600.20;
			currentOnPmos[10]  = 561.30;
			currentOnPmos[20]  = 525.50;
			currentOnPmos[30]  = 492.50;
			currentOnPmos[40]  = 462.20;
			currentOnPmos[50]  = 434.30;
			currentOnPmos[60]  = 408.70;
			currentOnPmos[70]  = 385.10;
			currentOnPmos[80]  = 363.40;
			currentOnPmos[90]  = 343.30;
			currentOnPmos[100] = 324.80;
			currentOffNmos[0]  = 100.00e-6;
			currentOffNmos[10] = 147.30e-6;
			currentOffNmos[20] = 212.10e-6;
			currentOffNmos[30] = 299.60e-6;
			currentOffNmos[40] = 415.30e-6;
			currentOffNmos[50] = 565.80e-6;
			currentOffNmos[60] = 758.90e-6;
			currentOffNmos[70] = 1003.00e-6;
			currentOffNmos[80] = 1307.00e-6;
			currentOffNmos[90] = 1682.00e-6;
			currentOffNmos[100]= 2139.00e-6;
			currentOffPmos[0]  = 100.00e-6;
			currentOffPmos[10] = 147.30e-6;
			currentOffPmos[20] = 212.10e-6;
			currentOffPmos[30] = 299.60e-6;
			currentOffPmos[40] = 415.30e-6;
			currentOffPmos[50] = 565.80e-6;
			currentOffPmos[60] = 758.90e-6;
			currentOffPmos[70] = 1003.00e-6;
			currentOffPmos[80] = 1307.00e-6;
			currentOffPmos[90] = 1682.00e-6;
			currentOffPmos[100]= 2139.00e-6;
			pnSizeRatio = currentOnNmos[0]/currentOnPmos[0];
		} else if (_deviceRoadmap == CNT) {
			/* CNFET @ 20nm gate length */
			vdd = 0.7;
			vth = 0.18;
			vpp = vdd + vth;
			phyGateLength = 0.020e-6;
			capIdealGate = 1.18e-15;
			capFringe = 4.18e-17;
			capOx = 5.90e-2;
			effectiveElectronMobility = 78700e-4;
			effectiveHoleMobility = 78700e-4;
			pnSizeRatio = 1.0;
			effectiveResistanceMultiplier = 1.3;
			gm_oncurrent = 0.0;

			currentOnNmos[0]   = 3361.1;
			currentOnNmos[10]  = 3300.0;
			currentOnNmos[20]  = 3240.0;
			currentOnNmos[30]  = 3180.0;
			currentOnNmos[40]  = 3120.0;
			currentOnNmos[50]  = 3060.0;
			currentOnNmos[60]  = 3000.0;
			currentOnNmos[70]  = 2940.0;
			currentOnNmos[80]  = 2880.0;
			currentOnNmos[90]  = 2820.0;
			currentOnNmos[100] = 2760.0;

			/* PMOS ON-current (same as NMOS if symmetrical) */
			currentOnPmos[0]   = currentOnNmos[0];
			currentOnPmos[10]  = currentOnNmos[10];
			currentOnPmos[20]  = currentOnNmos[20];
			currentOnPmos[30]  = currentOnNmos[30];
			currentOnPmos[40]  = currentOnNmos[40];
			currentOnPmos[50]  = currentOnNmos[50];
			currentOnPmos[60]  = currentOnNmos[60];
			currentOnPmos[70]  = currentOnNmos[70];
			currentOnPmos[80]  = currentOnNmos[80];
			currentOnPmos[90]  = currentOnNmos[90];
			currentOnPmos[100] = currentOnNmos[100];

			currentOffNmos[0]   = 8.94e-2;
			currentOffNmos[10]  = 1.00e-1;
			currentOffNmos[20]  = 1.12e-1;
			currentOffNmos[30]  = 1.26e-1;
			currentOffNmos[40]  = 1.41e-1;
			currentOffNmos[50]  = 1.57e-1;
			currentOffNmos[60]  = 1.75e-1;
			currentOffNmos[70]  = 1.94e-1;
			currentOffNmos[80]  = 2.15e-1;
			currentOffNmos[90]  = 2.37e-1;
			currentOffNmos[100] = 2.60e-1;

			/* PMOS OFF-current (same as NMOS if symmetrical) */
			currentOffPmos[0]   = currentOffNmos[0];
			currentOffPmos[10]  = currentOffNmos[10];
			currentOffPmos[20]  = currentOffNmos[20];
			currentOffPmos[30]  = currentOffNmos[30];
			currentOffPmos[40]  = currentOffNmos[40];
			currentOffPmos[50]  = currentOffNmos[50];
			currentOffPmos[60]  = currentOffNmos[60];
			currentOffPmos[70]  = currentOffNmos[70];
			currentOffPmos[80]  = currentOffNmos[80];
			currentOffPmos[90]  = currentOffNmos[90];
			currentOffPmos[100] = currentOffNmos[100];
		} else {
            cout << "Unknown device roadmap!" << endl;
            exit(1);
        }
	} else if (featureSizeInNano >= 14) {
		if (deviceRoadmap != LOP) {
			
			cout<<"Only LOP for 14 nm and beyond is supported"<<endl;
			exit(-1);

		} else if (_deviceRoadmap == LOP){
			// device specifications follow IRDS 2016
			vdd = 0.8;
			vth = vth_list[0];
			vpp = vdd + vth;
			heightFin = 4.2e-8;
			widthFin = 8.0e-9;
			PitchFin = 4.8e-8;

			max_fin_num =4;
			effective_width=widthFin+heightFin*2;

			phyGateLength = 2.6e-8; // changed to 2.6e-8 following IRDS 2016
			capIdealGate = caplist[0] * 1E-18 / (effective_width);
			cap_draintotal = cap_draintotallist[0] / (effective_width);
			capFringe = 0;
			effectiveResistanceMultiplier = eff_res_mul[0];	/* from CACTI */
			current_gmNmos= gm[0];
			current_gmPmos= gm[0];	
			gm_oncurrent = gm[0];  // gm at on current


			currentOnNmos[0]  = currentlist[0];
			currentOnNmos[10] = 853;
			currentOnNmos[20] = 814;
			currentOnNmos[30] = 777;
			currentOnNmos[40] = 742;
			currentOnNmos[50] = 708;
			currentOnNmos[60] = 677;
			currentOnNmos[70] = 646;
			currentOnNmos[80] = 618;
			currentOnNmos[90] = 591;
			currentOnNmos[100] =565;
			currentOnPmos[0]  = currentOnNmos[0];
			currentOnPmos[10] = 767;
			currentOnPmos[20] = 718;
			currentOnPmos[30] = 672;
			currentOnPmos[40] = 631;
			currentOnPmos[50] = 593;
			currentOnPmos[60] = 558;
			currentOnPmos[70] = 526;
			currentOnPmos[80] = 496;
			currentOnPmos[90] = 469;
			currentOnPmos[100] =443;
			currentOffNmos[0]  = 0.0001;
			currentOffNmos[10] = 184.4553e-6;
			currentOffNmos[20] = 328.7707e-6;
			currentOffNmos[30] = 566.8658e-6;
			currentOffNmos[40] = 948.1816e-6;
			currentOffNmos[50] = 1.5425e-3;
			currentOffNmos[60] = 2.4460e-3;
			currentOffNmos[70] = 3.7885e-3;
			currentOffNmos[80] = 5.7416e-3;
			currentOffNmos[90] = 8.5281e-3;
			currentOffNmos[100] =1.24327e-2;;
			currentOffPmos[0]  = 102.3333e-6;
			currentOffPmos[10] = 203.4774e-6;
			currentOffPmos[20] = 389.0187e-6;
			currentOffPmos[30] = 717.5912e-6;
			currentOffPmos[40] = 1.2810e-3;
			currentOffPmos[50] = 2.2192e-3;
			currentOffPmos[60] = 3.7395e-3;
			currentOffPmos[70] = 6.1428e-3;
			currentOffPmos[80] = 9.8554e-3;
			currentOffPmos[90] = 1.54702e-2;
			currentOffPmos[100] =2.37959e-2;
			pnSizeRatio = (int)(currentOnNmos[0]/currentOnPmos[0]);
		}
	} else if (featureSizeInNano >= 10) {
		if (deviceRoadmap != LOP) {

			cout<<"Only LOP for 14 nm and beyond is supported"<<endl;
			exit(-1);

		} else if (_deviceRoadmap == LOP) {

			// device specifications follow IRDS 2017
			vdd = 0.75;
			vth = vth_list[1];
			vpp = vdd + vth;
			heightFin = 4.5e-8;	
			widthFin = 8.0e-9;	
			PitchFin = 3.6e-8;			

			max_fin_num =3;
			effective_width=widthFin+heightFin*2;

			phyGateLength = 2.2e-8;	
			capIdealGate = caplist[1] * 1E-18 / (effective_width);
			cap_draintotal = cap_draintotallist[1]/ (effective_width);
			capFringe = 0;
			effectiveResistanceMultiplier = eff_res_mul[1];	/* from CACTI */
			current_gmNmos= gm[1];
			current_gmPmos= gm[1];			
			gm_oncurrent = gm[1];  // gm at on current


			currentOnNmos[0]  = currentlist[1];
			currentOnNmos[10] = 824;
			currentOnNmos[20] = 787;
			currentOnNmos[30] = 751;
			currentOnNmos[40] = 717;
			currentOnNmos[50] = 684;
			currentOnNmos[60] = 654;
			currentOnNmos[70] = 624;
			currentOnNmos[80] = 597;
			currentOnNmos[90] = 571;
			currentOnNmos[100] =546;
			currentOnPmos[0]  = currentOnNmos[0];  
			currentOnPmos[10] = 725;
			currentOnPmos[20] = 678;
			currentOnPmos[30] = 636;
			currentOnPmos[40] = 597;
			currentOnPmos[50] = 561;
			currentOnPmos[60] = 527;
			currentOnPmos[70] = 497;
			currentOnPmos[80] = 469;
			currentOnPmos[90] = 443;
			currentOnPmos[100] =419;
			currentOffNmos[0]  = 0.000127;
			currentOffNmos[10] = 184.4892e-6;
			currentOffNmos[20] = 329.1615e-6;
			currentOffNmos[30] = 568.0731e-6;
			currentOffNmos[40] = 951.0401e-6;
			currentOffNmos[50] = 1.5484e-3;
			currentOffNmos[60] = 2.4574e-3;
			currentOffNmos[70] = 3.8090e-3;
			currentOffNmos[80] = 5.7767e-3;
			currentOffNmos[90] = 8.5862e-3;
			currentOffNmos[100] =1.2525e-2;
			currentOffPmos[0]  = 100.5839e-6;
			currentOffPmos[10] = 200.2609e-6;
			currentOffPmos[20] = 383.3239e-6;
			currentOffPmos[30] = 707.8499e-6;
			currentOffPmos[40] = 1.2649e-3;
			currentOffPmos[50] = 2.1932e-3;
			currentOffPmos[60] = 3.6987e-3;
			currentOffPmos[70] = 6.0804e-3;
			currentOffPmos[80] = 9.7622e-3;
			currentOffPmos[90] = 1.53340e-2;
			currentOffPmos[100] =2.36007e-2;
			pnSizeRatio = (int)(currentOnNmos[0]/currentOnPmos[0]);
		}
	} else if (featureSizeInNano >= 7) {
		if (deviceRoadmap != LOP) {

			cout<<"Only LOP for 14 nm and beyond is supported"<<endl;
			exit(-1);

		} else if (_deviceRoadmap == LOP){

			// based on IRDS 2017
			vdd = 0.7;
			vth = vth_list[2];
			vpp = vdd + vth;
			heightFin = 5.0e-8;
			widthFin = 7e-9;
			PitchFin = 3.0e-8;			

			max_fin_num = 2;
			effective_width=107e-9;	

			phyGateLength = 2.2e-8;
			capIdealGate = caplist[2] * 1E-18 / (effective_width);//8.49489e-10;
			cap_draintotal = cap_draintotallist[2]/ (effective_width);
			capFringe = 0;
			effectiveResistanceMultiplier = eff_res_mul[2];	/* from CACTI */
			current_gmNmos= gm[2];
			current_gmPmos= gm[2];
			gm_oncurrent = gm[2];  // gm at on current


			currentOnNmos[0]  = currentlist[2];
			currentOnNmos[10] = 786; 
			currentOnNmos[20] = 750; 
			currentOnNmos[30] = 716; 
			currentOnNmos[40] = 684; 
			currentOnNmos[50] = 653; 
			currentOnNmos[60] = 624; 
			currentOnNmos[70] = 595; 
			currentOnNmos[80] = 569; 
			currentOnNmos[90] = 545;
			currentOnNmos[100]= 521; 
			currentOnPmos[0]  = currentOnNmos[0];  
			currentOnPmos[10] = 689;
			currentOnPmos[20] = 645;
			currentOnPmos[30] = 605;
			currentOnPmos[40] = 567;
			currentOnPmos[50] = 533;
			currentOnPmos[60] = 501;
			currentOnPmos[70] = 473;
			currentOnPmos[80] = 446;
			currentOnPmos[90] = 421;
			currentOnPmos[100] =398;
			currentOffNmos[0]  = 0.000147;
			currentOffNmos[10] = 1.85E-04;
			currentOffNmos[20] = 3.32E-04;
			currentOffNmos[30] = 5.74E-04;
			currentOffNmos[40] = 9.62E-04;
			currentOffNmos[50] = 1.5695e-3;
			currentOffNmos[60] = 2.4953e-3;
			currentOffNmos[70] = 3.8744e-3 ;
			currentOffNmos[80] = 5.8858e-3 ;
			currentOffNmos[90] = 8.7624e-3;
			currentOffNmos[100] =1.28025e-2;
			currentOffPmos[0]  = 100.9536e-6;
			currentOffPmos[10] = 201.3937e-6;
			currentOffPmos[20] = 386.2086e-6;
			currentOffPmos[30] = 714.4288e-6;
			currentOffPmos[40] = 1.2788e-3;
			currentOffPmos[50] = 2.2207e-3;
			currentOffPmos[60] = 3.7509e-3;
			currentOffPmos[70] = 6.1750e-3;
			currentOffPmos[80] = 9.9278e-3;
			currentOffPmos[90] = 1.56146e-2;
			currentOffPmos[100] =2.40633e-2;
			pnSizeRatio = (int)(currentOnNmos[0]/currentOnPmos[0]);
		}
	} 

	/* Technology update beyond 7 nm */ 
	else if (featureSizeInNano >= 5) {
		if (deviceRoadmap != LOP) {

			cout<<"Only LOP for 14 nm and beyond is supported"<<endl;
			exit(-1);

		} else if (_deviceRoadmap == LOP){

			// IRDS 2021
			vdd = 0.7;
			vth = vth_list[3];
			vpp = vdd + vth;

			widthFin=6.0e-9; 
			PitchFin=28.0e-9;	
			phyGateLength = 2.0e-8;

			// height is not needed as long as effective width is specified
			effective_width = 106.0*1e-9;	
			max_fin_num =2;		

			capIdealGate = caplist[3] * 1E-18 / (effective_width );
			cap_draintotal = cap_draintotallist[3]/ (effective_width);
			capFringe = 0;

			effectiveResistanceMultiplier = eff_res_mul[3];	/* from CACTI */
			current_gmNmos= gm[3];
			current_gmPmos= gm[3];
			gm_oncurrent = gm[3];  // gm at on current


			currentOnNmos[0]  = currentlist[3];
			currentOnNmos[10] = 786; 
			currentOnNmos[20] = 750; 
			currentOnNmos[30] = 716; 
			currentOnNmos[40] = 684; 
			currentOnNmos[50] = 653; 
			currentOnNmos[60] = 624; 
			currentOnNmos[70] = 595; 
			currentOnNmos[80] = 569; 
			currentOnNmos[90] = 545;
			currentOnNmos[100]= 521; 
			currentOnPmos[0]  = currentOnNmos[0]; 
			currentOnPmos[10] = 689;
			currentOnPmos[20] = 645;
			currentOnPmos[30] = 605;
			currentOnPmos[40] = 567;
			currentOnPmos[50] = 533;
			currentOnPmos[60] = 501;
			currentOnPmos[70] = 473;
			currentOnPmos[80] = 446;
			currentOnPmos[90] = 421;
			currentOnPmos[100] =398;
			currentOffNmos[0]  = 0.000138;
			currentOffNmos[10] = 1.85E-04;
			currentOffNmos[20] = 3.32E-04;
			currentOffNmos[30] = 5.74E-04;
			currentOffNmos[40] = 9.62E-04;
			currentOffNmos[50] = 1.5695e-3;
			currentOffNmos[60] = 2.4953e-3;
			currentOffNmos[70] = 3.8744e-3 ;
			currentOffNmos[80] = 5.8858e-3 ;
			currentOffNmos[90] = 8.7624e-3;
			currentOffNmos[100] =1.28025e-2;
			currentOffPmos[0]  = 100.9536e-6;
			currentOffPmos[10] = 201.3937e-6;
			currentOffPmos[20] = 386.2086e-6;
			currentOffPmos[30] = 714.4288e-6;
			currentOffPmos[40] = 1.2788e-3;
			currentOffPmos[50] = 2.2207e-3;
			currentOffPmos[60] = 3.7509e-3;
			currentOffPmos[70] = 6.1750e-3;
			currentOffPmos[80] = 9.9278e-3;
			currentOffPmos[90] = 1.56146e-2;
			currentOffPmos[100] =2.40633e-2;
			pnSizeRatio = (int)(currentOnNmos[0]/currentOnPmos[0]);
		}
	} else if (featureSizeInNano >= 3) {
		if (deviceRoadmap != LOP) {

			cout<<"Only LOP for 14 nm and beyond is supported"<<endl;
			exit(-1);

		} else if (_deviceRoadmap == LOP){

			// IRDS 2022
			vdd = 0.7;
			vth = vth_list[4];
			vpp = vdd + vth;
			widthFin=5.0e-9;  	
			PitchFin=24.0e-9;	
			phyGateLength = 1.8e-8;

			// height is not needed as long as effective width is specified
			effective_width = 101.0*1e-9;
			max_fin_num =2;	
			
			capIdealGate = caplist[4] * 1E-18 / (effective_width);   //6.44E-10; //8.91E-10;
			cap_draintotal = cap_draintotallist[4]/ (effective_width);
			capFringe = 0;

			effectiveResistanceMultiplier = eff_res_mul[4];	/* from CACTI */
			current_gmNmos= gm[4];
			current_gmPmos= gm[4];	
			gm_oncurrent = gm[4];  // gm at on current

			currentOnNmos[0]  = currentlist[4];
			currentOnNmos[10] = 786; 
			currentOnNmos[20] = 750; 
			currentOnNmos[30] = 716; 
			currentOnNmos[40] = 684; 
			currentOnNmos[50] = 653; 
			currentOnNmos[60] = 624; 
			currentOnNmos[70] = 595; 
			currentOnNmos[80] = 569; 
			currentOnNmos[90] = 545;
			currentOnNmos[100]= 521; 
			currentOnPmos[0]  = currentOnNmos[0]; 
			currentOnPmos[10] = 689;
			currentOnPmos[20] = 645;
			currentOnPmos[30] = 605;
			currentOnPmos[40] = 567;
			currentOnPmos[50] = 533;
			currentOnPmos[60] = 501;
			currentOnPmos[70] = 473;
			currentOnPmos[80] = 446;
			currentOnPmos[90] = 421;
			currentOnPmos[100] =398;
			currentOffNmos[0]  = 0.000158;
			currentOffNmos[10] = 1.85E-04;
			currentOffNmos[20] = 3.32E-04;
			currentOffNmos[30] = 5.74E-04;
			currentOffNmos[40] = 9.62E-04;
			currentOffNmos[50] = 1.5695e-3;
			currentOffNmos[60] = 2.4953e-3;
			currentOffNmos[70] = 3.8744e-3 ;
			currentOffNmos[80] = 5.8858e-3 ;
			currentOffNmos[90] = 8.7624e-3;
			currentOffNmos[100] =1.28025e-2;
			currentOffPmos[0]  = 100.9536e-6;
			currentOffPmos[10] = 201.3937e-6;
			currentOffPmos[20] = 386.2086e-6;
			currentOffPmos[30] = 714.4288e-6;
			currentOffPmos[40] = 1.2788e-3;
			currentOffPmos[50] = 2.2207e-3;
			currentOffPmos[60] = 3.7509e-3;
			currentOffPmos[70] = 6.1750e-3;
			currentOffPmos[80] = 9.9278e-3;
			currentOffPmos[90] = 1.56146e-2;
			currentOffPmos[100] =2.40633e-2;
			pnSizeRatio = (int)(currentOnNmos[0]/currentOnPmos[0]);
		}
	} 
	else if (featureSizeInNano >= 2) {
		if (deviceRoadmap != LOP) {

			cout<<"Only LOP for 14 nm and beyond is supported"<<endl;
			exit(-1);

		} else if (_deviceRoadmap == LOP){ // 2 nm

			// IRDS 2022
			vdd = 0.65;
			vth = vth_list[5];
			vpp = vdd + vth;
			PitchFin= 26e-9;
			phyGateLength = 1.4e-8;

			// GAA-specific parameters
			max_fin_per_GAA=1;
			max_sheet_num=3;
			thickness_sheet=6*1e-9;
			width_sheet=15*1e-9;	

			widthFin=width_sheet; // for drain height calculation 	
			effective_width=(thickness_sheet+width_sheet)*2;

			capIdealGate = caplist[5] * 1E-18 /  (effective_width*max_sheet_num) ; 
			cap_draintotal = cap_draintotallist[5]/ (effective_width);
			capFringe = 0;

			effectiveResistanceMultiplier = eff_res_mul[5];	/* from CACTI */
			current_gmNmos= gm[5];
			current_gmPmos= gm[5];	
			gm_oncurrent = gm[5];  // gm at on current


			currentOnNmos[0]  = currentlist[5];
			currentOnNmos[10] = 786; 
			currentOnNmos[20] = 750; 
			currentOnNmos[30] = 716; 
			currentOnNmos[40] = 684; 
			currentOnNmos[50] = 653; 
			currentOnNmos[60] = 624; 
			currentOnNmos[70] = 595; 
			currentOnNmos[80] = 569; 
			currentOnNmos[90] = 545;
			currentOnNmos[100]= 521; 
			currentOnPmos[0]  = currentOnNmos[0]; 
			currentOnPmos[10] = 689;
			currentOnPmos[20] = 645;
			currentOnPmos[30] = 605;
			currentOnPmos[40] = 567;
			currentOnPmos[50] = 533;
			currentOnPmos[60] = 501;
			currentOnPmos[70] = 473;
			currentOnPmos[80] = 446;
			currentOnPmos[90] = 421;
			currentOnPmos[100] =398;
			currentOffNmos[0]  = 0.0000733;
			currentOffNmos[10] = 1.85E-04;
			currentOffNmos[20] = 3.32E-04;
			currentOffNmos[30] = 5.74E-04;
			currentOffNmos[40] = 9.62E-04;
			currentOffNmos[50] = 1.5695e-3;
			currentOffNmos[60] = 2.4953e-3;
			currentOffNmos[70] = 3.8744e-3 ;
			currentOffNmos[80] = 5.8858e-3 ;
			currentOffNmos[90] = 8.7624e-3;
			currentOffNmos[100] =1.28025e-2;
			currentOffPmos[0]  = 100.9536e-6;
			currentOffPmos[10] = 201.3937e-6;
			currentOffPmos[20] = 386.2086e-6;
			currentOffPmos[30] = 714.4288e-6;
			currentOffPmos[40] = 1.2788e-3;
			currentOffPmos[50] = 2.2207e-3;
			currentOffPmos[60] = 3.7509e-3;
			currentOffPmos[70] = 6.1750e-3;
			currentOffPmos[80] = 9.9278e-3;
			currentOffPmos[90] = 1.56146e-2;
			currentOffPmos[100] =2.40633e-2;
			pnSizeRatio = (int)(currentOnNmos[0]/currentOnPmos[0]);
		}
	} 		
	else if (featureSizeInNano >= 1) {
		if (deviceRoadmap != LOP) {

			cout<<"Only LOP for 14 nm and beyond is supported"<<endl;
			exit(-1);

		} else if (_deviceRoadmap == LOP) {

			// IRDS 2022
			vdd = 0.6;
			vth = vth_list[6];
			vpp = vdd + vth;
			PitchFin= 24e-9;
			phyGateLength = 1.2e-8;

			// IRDS 2022 - GAA specfic parameters
			max_fin_per_GAA=1;
			max_sheet_num=4;
			thickness_sheet=6*1e-9;
			width_sheet=10*1e-9;	
				
			widthFin= width_sheet; // for drain height calculation 
			effective_width=(thickness_sheet+width_sheet)*2;
			
			capIdealGate = caplist[6] * 1E-18 /  (effective_width*max_sheet_num) ;
			cap_draintotal = cap_draintotallist[6]/ (effective_width);
			capFringe = 0;

			effectiveResistanceMultiplier = eff_res_mul[6];	/* from CACTI */
			current_gmNmos= gm[6];
			current_gmPmos= gm[6];	
			gm_oncurrent = gm[6];  // gm at on current	

			currentOnNmos[0]  = currentlist[6];
			currentOnNmos[10] = 786; 
			currentOnNmos[20] = 750; 
			currentOnNmos[30] = 716; 
			currentOnNmos[40] = 684; 
			currentOnNmos[50] = 653; 
			currentOnNmos[60] = 624; 
			currentOnNmos[70] = 595; 
			currentOnNmos[80] = 569; 
			currentOnNmos[90] = 545;
			currentOnNmos[100]= 521; 
			currentOnPmos[0]  = currentOnNmos[0]; 
			currentOnPmos[10] = 689;
			currentOnPmos[20] = 645;
			currentOnPmos[30] = 605;
			currentOnPmos[40] = 567;
			currentOnPmos[50] = 533;
			currentOnPmos[60] = 501;
			currentOnPmos[70] = 473;
			currentOnPmos[80] = 446;
			currentOnPmos[90] = 421;
			currentOnPmos[100] =398;
			currentOffNmos[0]  = 0.000169;
			currentOffNmos[10] = 1.85E-04;
			currentOffNmos[20] = 3.32E-04;
			currentOffNmos[30] = 5.74E-04;
			currentOffNmos[40] = 9.62E-04;
			currentOffNmos[50] = 1.5695e-3;
			currentOffNmos[60] = 2.4953e-3;
			currentOffNmos[70] = 3.8744e-3 ;
			currentOffNmos[80] = 5.8858e-3 ;
			currentOffNmos[90] = 8.7624e-3;
			currentOffNmos[100] =1.28025e-2;
			currentOffPmos[0]  = 100.9536e-6;
			currentOffPmos[10] = 201.3937e-6;
			currentOffPmos[20] = 386.2086e-6;
			currentOffPmos[30] = 714.4288e-6;
			currentOffPmos[40] = 1.2788e-3;
			currentOffPmos[50] = 2.2207e-3;
			currentOffPmos[60] = 3.7509e-3;
			currentOffPmos[70] = 6.1750e-3;
			currentOffPmos[80] = 9.9278e-3;
			currentOffPmos[90] = 1.56146e-2;
			currentOffPmos[100] =2.40633e-2;
			pnSizeRatio = (int)(currentOnNmos[0]/currentOnPmos[0]);
		}
	}
	else 
	{
            cout << "Unknown device roadmap!" << endl;
            exit(1);
    }
        // If FeFET, we scale ideal gate capacitance by standard factor across all tech nodes
        if (is_FeFET == true){
	    	//cout << "Modify capIdealGate for FeFET:" << endl;
            capIdealGateFeFET = capIdealGate * 1.73;
        }

	capOverlap = capIdealGate * 0.2;
	//capSidewall = 2.5e-10;	/* Unit: F/m, this value is from CACTI, PTM model shows the value is 5e-10 */
	double cjd = 1e-3;             /* Bottom junction capacitance, Unit: F/m^2*/
	double cjswd = 2.5e-10;           /* Isolation-edge sidewall junction capacitance, Unit: F/m */
	double cjswgd = 0.5e-10;          /* Gate-edge sidewall junction capacitance, Unit: F/m */
	double mjd = 0.5;             /* Bottom junction capacitance grating coefficient */
	double mjswd = 0.33;           /* Isolation-edge sidewall junction capacitance grading coefficient */
	double mjswgd = 0.33;          /* Gate-edge sidewall junction capacitance grading coefficient */
	buildInPotential = 0.9;			/* This value is from BSIM4 */
	capJunction = cjd / pow(1 + vdd / buildInPotential, mjd);
	capSidewall = cjswd / pow(1 + vdd / buildInPotential, mjswd);
	capDrainToChannel = cjswgd / pow(1 + vdd / buildInPotential, mjswgd);

	// junction capacitance for 14 nm and beyond; 

	if (featureSizeInNano == 14 ) capJunction= 0.0120;
	else if (featureSizeInNano == 10 ) capJunction= 0.0134;
	else if (featureSizeInNano== 7 ) capJunction= 0.0137;
	else if (featureSizeInNano == 5 ) capJunction= 0.0119;
	else if (featureSizeInNano == 3 ) capJunction= 0.0128;
	else if (featureSizeInNano == 2 ) capJunction= 0.0091;
	else if (featureSizeInNano == 1 ) capJunction= 0.0102;
	else capJunction = cjd / pow(1 + vdd / buildInPotential, mjd);

	vdsatNmos = phyGateLength * 1e5 /* Silicon saturatio velocity, Unit: m/s */ / effectiveElectronMobility;
	vdsatPmos = phyGateLength * 1e5 /* Silicon saturatio velocity, Unit: m/s */ / effectiveHoleMobility;

	/* Properties not used so far */
	capPolywire = 0.0;	/* TO-DO: we need to find the values */

	/* Interpolate */
	for (int i = 1; i < 100; i++) {
		if (i % 10) {
			double a = currentOnNmos[i / 10 * 10];
			double b = currentOnNmos[i / 10 * 10 + 10];
			currentOnNmos[i] = a + (b-a) * (i % 10) / 10;
		}
	}

	for (int i = 1; i < 100; i++) {
		if (i % 10) {
			double a = currentOnPmos[i / 10 * 10];
			double b = currentOnPmos[i / 10 * 10 + 10];
			currentOnPmos[i] = a + (b-a) * (i % 10) / 10;
		}
	}

	for (int i = 1; i < 100; i++) {
		if (i % 10) {
			double a = currentOffNmos[i / 10 * 10];
			double b = currentOffNmos[i / 10 * 10 + 10];
			currentOffNmos[i] = a + (b-a) * (i % 10) / 10;
		}
	}

	for (int i = 1; i < 100; i++) {
		if (i % 10) {
			double a = currentOffPmos[i / 10 * 10];
			double b = currentOffPmos[i / 10 * 10 + 10];
			currentOffPmos[i] = a + (b-a) * (i % 10) / 10;
		}
	}

	initialized = true;
}
void Technology::PrintProperty() {
	cout << "Fabrication Process Technology Node:" << endl;
	cout << "TO-DO" << endl;
}

void Technology::InterpolateWith(Technology rhs, double _alpha) {
	if (featureSizeInNano != rhs.featureSizeInNano) {
		vdd = (1 - _alpha) * vdd + _alpha * rhs.vdd;
		vth = (1 - _alpha) * vth + _alpha * rhs.vth;
		phyGateLength = (1 - _alpha) * phyGateLength + _alpha * rhs.phyGateLength;
		capIdealGate = (1 - _alpha) * capIdealGate + _alpha * rhs.capIdealGate;
                if (is_FeFET){
		    capIdealGateFeFET = (1 - _alpha) * capIdealGateFeFET + _alpha * rhs.capIdealGate;
                }
		capFringe = (1 - _alpha) * capFringe + _alpha * rhs.capFringe;
		capJunction = (1 - _alpha) * capJunction + _alpha * rhs.capJunction;
		capOx = (1 - _alpha) * capOx + _alpha * rhs.capOx;
		effectiveElectronMobility = (1 - _alpha) * effectiveElectronMobility + _alpha * rhs.effectiveElectronMobility;
		effectiveHoleMobility = (1 - _alpha) * effectiveHoleMobility + _alpha * rhs.effectiveHoleMobility;
		pnSizeRatio = (1 - _alpha) * pnSizeRatio + _alpha * rhs.pnSizeRatio;
		effectiveResistanceMultiplier = (1 - _alpha) * effectiveResistanceMultiplier + _alpha * rhs.effectiveResistanceMultiplier;
		for (int i = 0; i <= 100; i++){
			currentOnNmos[i] = (1 - _alpha) * currentOnNmos[i] + _alpha * rhs.currentOnNmos[i];
			currentOnPmos[i] = (1 - _alpha) * currentOnPmos[i] + _alpha * rhs.currentOnPmos[i];
			currentOffNmos[i] = pow(currentOffNmos[i], 1 - _alpha) * pow(rhs.currentOffNmos[i], _alpha);
			currentOffPmos[i] = pow(currentOffPmos[i], 1 - _alpha) * pow(rhs.currentOffPmos[i], _alpha);
		}
		//capSidewall = 2.5e-10;	/* Unit: F/m, this value is from CACTI, PTM model shows the value is 5e-10 */
		double cjd = 1e-3;             /* Bottom junction capacitance, Unit: F/m^2*/
		double cjswd = 2.5e-10;           /* Isolation-edge sidewall junction capacitance, Unit: F/m */
		double cjswgd = 0.5e-10;          /* Gate-edge sidewall junction capacitance, Unit: F/m */
		double mjd = 0.5;             /* Bottom junction capacitance grating coefficient */
		double mjswd = 0.33;           /* Isolation-edge sidewall junction capacitance grading coefficient */
		double mjswgd = 0.33;          /* Gate-edge sidewall junction capacitance grading coefficient */
		buildInPotential = 0.9;			/* This value is from BSIM4 */
		capJunction = cjd / pow(1 + vdd / buildInPotential, mjd);
		capSidewall = cjswd / pow(1 + vdd / buildInPotential, mjswd);
		capDrainToChannel = cjswgd / pow(1 + vdd / buildInPotential, mjswgd);

		vdsatNmos = phyGateLength * 1e5 /* Silicon saturatio velocity, Unit: m/s */ / effectiveElectronMobility;
		vdsatPmos = phyGateLength * 1e5 /* Silicon saturatio velocity, Unit: m/s */ / effectiveHoleMobility;
	}
}