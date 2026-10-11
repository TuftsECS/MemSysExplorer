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


#include "Technology.hpp"
#include "constant.hpp"

#include <math.h>

void Technology::Initialize(int _featureSizeInNano, DeviceRoadmap _deviceRoadmap, bool _isFeFET) {
	using namespace mse::unit;

	if (initialized)
		std::cout << "Warning: Already initialized!" << std::endl;


	// based on neurosim
	double caplist [7] = {103.816,97.549,100.497,81.859,72.572, 79.74, 66.94}; // 69.369
	double currentlist [7] = {595.045, 599.237, 562.048, 578.494, 641.463, 526.868, 460.979}; //  556.448
	double effectiveResistanceMultipliers [7] = {2.09, 2.09, 2.05, 2.10, 2.14, 1.98, 2.05};
	double gm [7] = {1415.34, 1803.50, 1785.37, 1820.90, 2018.04, 1968.85, 2401.75};
	double thresholdVoltages [7] = {0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1}; // Dummy values, since we don't need them
	double drainTotalCapacitances [7] = {2.499e-17, 2.668e-17, 2.224e-17, 2.076e-17, 1.791e-17, 1.543e-17, 1.409e-17};


	featureSizeInNano = _featureSizeInNano;
	featureSize = MeterPerFeature(_featureSizeInNano * 1e-9);
	deviceRoadmap = _deviceRoadmap;
        isFeFET = _isFeFET;

	if (_featureSizeInNano >= 200) { //TODO : only for test
		if (_deviceRoadmap == HP) {
			vdd = 1.5_V;
			vth = 300e-3_V;
			vpp = vdd + vth;
			phyGateLength = 0.1e-6_m;
			capIdealGate = FaradPerMeter(8e-10);
			capFringe = FaradPerMeter(2.5e-10);
			capJunction = FaradPerSquareMeter(1.00e-3);
			capOx = FaradPerSquareMeter(1e-2);
			effectiveElectronMobility = Mobility(320e-4);
			effectiveHoleMobility = Mobility(80e-4);
			pnSizeRatio = 2.45;						/* from CACTI */
			effectiveResistanceMultiplier = 1.54;	/* from CACTI */
			currentOnNmos[0] = AmperePerMeter(750);
			currentOnNmos[10] = AmperePerMeter(750);
			currentOnNmos[20] = AmperePerMeter(750);
			currentOnNmos[30] = AmperePerMeter(750);
			currentOnNmos[40] = AmperePerMeter(750);
			currentOnNmos[50] = AmperePerMeter(750);
			currentOnNmos[60] = AmperePerMeter(750);
			currentOnNmos[70] = AmperePerMeter(750);
			currentOnNmos[80] = AmperePerMeter(750);
			currentOnNmos[90] = AmperePerMeter(750);
			currentOnNmos[100] = AmperePerMeter(750);
			currentOnPmos[0] = AmperePerMeter(350);
			currentOnPmos[10] = AmperePerMeter(350);
			currentOnPmos[20] = AmperePerMeter(350);
			currentOnPmos[30] = AmperePerMeter(350);
			currentOnPmos[40] = AmperePerMeter(350);
			currentOnPmos[50] = AmperePerMeter(350);
			currentOnPmos[60] = AmperePerMeter(350);
			currentOnPmos[70] = AmperePerMeter(350);
			currentOnPmos[80] = AmperePerMeter(350);
			currentOnPmos[90] = AmperePerMeter(350);
			currentOnPmos[100] = AmperePerMeter(350);
			currentOffNmos[0] = AmperePerMeter(8e-3);
			currentOffNmos[10] = AmperePerMeter(8e-3);
			currentOffNmos[20] = AmperePerMeter(8e-3);
			currentOffNmos[30] = AmperePerMeter(8e-3);
			currentOffNmos[40] = AmperePerMeter(8e-3);
			currentOffNmos[50] = AmperePerMeter(8e-3);
			currentOffNmos[60] = AmperePerMeter(8e-3);
			currentOffNmos[70] = AmperePerMeter(8e-3);
			currentOffNmos[80] = AmperePerMeter(8e-3);
			currentOffNmos[90] = AmperePerMeter(8e-3);
			currentOffNmos[100] = AmperePerMeter(8e-3);
			currentOffPmos[0] = AmperePerMeter(1.6e-2);
			currentOffPmos[10] = AmperePerMeter(1.6e-2);
			currentOffPmos[20] = AmperePerMeter(1.6e-2);
			currentOffPmos[30] = AmperePerMeter(1.6e-2);
			currentOffPmos[40] = AmperePerMeter(1.6e-2);
			currentOffPmos[50] = AmperePerMeter(1.6e-2);
			currentOffPmos[60] = AmperePerMeter(1.6e-2);
			currentOffPmos[70] = AmperePerMeter(1.6e-2);
			currentOffPmos[80] = AmperePerMeter(1.6e-2);
			currentOffPmos[90] = AmperePerMeter(1.6e-2);
			currentOffPmos[100] = AmperePerMeter(1.6e-2);
		} else if (_deviceRoadmap == LOP) {
			vdd = 1.5_V;
			vth = 600e-3_V;
			vpp = vdd + vth;
			phyGateLength = 0.16e-6_m;
			capIdealGate = FaradPerMeter(8e-10);
			capFringe = FaradPerMeter(2.5e-10);
			capJunction = FaradPerSquareMeter(1.00e-3);
			capOx = FaradPerSquareMeter(1e-2);
			effectiveElectronMobility = Mobility(320e-4);
			effectiveHoleMobility = Mobility(80e-4);
			pnSizeRatio = 2.45;						/* from CACTI */
			effectiveResistanceMultiplier = 1.54;	/* from CACTI */
			currentOnNmos[0] = AmperePerMeter(330);
			currentOnNmos[10] = AmperePerMeter(330);
			currentOnNmos[20] = AmperePerMeter(330);
			currentOnNmos[30] = AmperePerMeter(330);
			currentOnNmos[40] = AmperePerMeter(330);
			currentOnNmos[50] = AmperePerMeter(330);
			currentOnNmos[60] = AmperePerMeter(330);
			currentOnNmos[70] = AmperePerMeter(330);
			currentOnNmos[80] = AmperePerMeter(330);
			currentOnNmos[90] = AmperePerMeter(330);
			currentOnNmos[100] = AmperePerMeter(330);
			currentOnPmos[0] = AmperePerMeter(168);
			currentOnPmos[10] = AmperePerMeter(168);
			currentOnPmos[20] = AmperePerMeter(168);
			currentOnPmos[30] = AmperePerMeter(168);
			currentOnPmos[40] = AmperePerMeter(168);
			currentOnPmos[50] = AmperePerMeter(168);
			currentOnPmos[60] = AmperePerMeter(168);
			currentOnPmos[70] = AmperePerMeter(168);
			currentOnPmos[80] = AmperePerMeter(168);
			currentOnPmos[90] = AmperePerMeter(168);
			currentOnPmos[100] = AmperePerMeter(168);
			currentOffNmos[0] = AmperePerMeter(4.25e-6);
			currentOffNmos[10] = AmperePerMeter(4.25e-6);
			currentOffNmos[20] = AmperePerMeter(4.25e-6);
			currentOffNmos[30] = AmperePerMeter(4.25e-6);
			currentOffNmos[40] = AmperePerMeter(4.25e-6);
			currentOffNmos[50] = AmperePerMeter(4.25e-6);
			currentOffNmos[60] = AmperePerMeter(4.25e-6);
			currentOffNmos[70] = AmperePerMeter(4.25e-6);
			currentOffNmos[80] = AmperePerMeter(4.25e-6);
			currentOffNmos[90] = AmperePerMeter(4.25e-6);
			currentOffNmos[100] = AmperePerMeter(4.25e-6);
			currentOffPmos[0] = AmperePerMeter(8.5e-6);
			currentOffPmos[10] = AmperePerMeter(8.5e-6);
			currentOffPmos[20] = AmperePerMeter(8.5e-6);
			currentOffPmos[30] = AmperePerMeter(8.5e-6);
			currentOffPmos[40] = AmperePerMeter(8.5e-6);
			currentOffPmos[50] = AmperePerMeter(8.5e-6);
			currentOffPmos[60] = AmperePerMeter(8.5e-6);
			currentOffPmos[70] = AmperePerMeter(8.5e-6);
			currentOffPmos[80] = AmperePerMeter(8.5e-6);
			currentOffPmos[90] = AmperePerMeter(8.5e-6);
			currentOffPmos[100] = AmperePerMeter(8.5e-6);
		} else {
			vdd = 1.2_V;
			vth = 450e-3_V;
			vpp = vdd + vth;
			phyGateLength = 0.135e-6_m;
			capIdealGate = FaradPerMeter(8e-10);
			capFringe = FaradPerMeter(2.5e-10);
			capJunction = FaradPerSquareMeter(1.00e-3);
			capOx = FaradPerSquareMeter(1e-2);
			effectiveElectronMobility = Mobility(330e-4);
			effectiveHoleMobility = Mobility(90e-4);
			pnSizeRatio = 2.45;						/* from CACTI */
			effectiveResistanceMultiplier = 1.54;	/* from CACTI */
			currentOnNmos[0] = AmperePerMeter(490);
			currentOnNmos[10] = AmperePerMeter(490);
			currentOnNmos[20] = AmperePerMeter(490);
			currentOnNmos[30] = AmperePerMeter(490);
			currentOnNmos[40] = AmperePerMeter(490);
			currentOnNmos[50] = AmperePerMeter(490);
			currentOnNmos[60] = AmperePerMeter(490);
			currentOnNmos[70] = AmperePerMeter(490);
			currentOnNmos[80] = AmperePerMeter(490);
			currentOnNmos[90] = AmperePerMeter(490);
			currentOnNmos[100] = AmperePerMeter(490);
			currentOnPmos[0] = AmperePerMeter(230);
			currentOnPmos[10] = AmperePerMeter(230);
			currentOnPmos[20] = AmperePerMeter(230);
			currentOnPmos[30] = AmperePerMeter(230);
			currentOnPmos[40] = AmperePerMeter(230);
			currentOnPmos[50] = AmperePerMeter(230);
			currentOnPmos[60] = AmperePerMeter(230);
			currentOnPmos[70] = AmperePerMeter(230);
			currentOnPmos[80] = AmperePerMeter(230);
			currentOnPmos[90] = AmperePerMeter(230);
			currentOnPmos[100] = AmperePerMeter(230);
			currentOffNmos[0] = AmperePerMeter(4e-4);
			currentOffNmos[10] = AmperePerMeter(4e-4);
			currentOffNmos[20] = AmperePerMeter(4e-4);
			currentOffNmos[30] = AmperePerMeter(4e-4);
			currentOffNmos[40] = AmperePerMeter(4e-4);
			currentOffNmos[50] = AmperePerMeter(4e-4);
			currentOffNmos[60] = AmperePerMeter(4e-4);
			currentOffNmos[70] = AmperePerMeter(4e-4);
			currentOffNmos[80] = AmperePerMeter(4e-4);
			currentOffNmos[90] = AmperePerMeter(4e-4);
			currentOffNmos[100] = AmperePerMeter(4e-4);
			currentOffPmos[0] = AmperePerMeter(8e-4);
			currentOffPmos[10] = AmperePerMeter(8e-4);
			currentOffPmos[20] = AmperePerMeter(8e-4);
			currentOffPmos[30] = AmperePerMeter(8e-4);
			currentOffPmos[40] = AmperePerMeter(8e-4);
			currentOffPmos[50] = AmperePerMeter(8e-4);
			currentOffPmos[60] = AmperePerMeter(8e-4);
			currentOffPmos[70] = AmperePerMeter(8e-4);
			currentOffPmos[80] = AmperePerMeter(8e-4);
			currentOffPmos[90] = AmperePerMeter(8e-4);
			currentOffPmos[100] = AmperePerMeter(8e-4);
		}
	} else if (_featureSizeInNano >= 120) {
		if (_deviceRoadmap == HP) {
			/* PTM model: 130nm_HP.pm, from http://ptm.asu.edu/ */
			vdd = 1.3_V;
			vth = 128.4855e-3_V;
			vpp = vdd + vth;
			phyGateLength = 7.5e-8_m;
			capIdealGate = FaradPerMeter(6.058401e-10);
			capFringe = FaradPerMeter(6.119807e-10);
			capJunction = FaradPerSquareMeter(1.00e-3);
			capOx = FaradPerSquareMeter(1.64e-2);
			effectiveElectronMobility = Mobility(249.59e-4);
			effectiveHoleMobility = Mobility(59.52e-4);
			effectiveResistanceMultiplier = 1.54;	/* from CACTI */
			transconductanceNmos = SiemensPerMeter(3.94E+02);
			transconductancePmos = SiemensPerMeter(2.61E+02);
			currentOnNmos[0] = AmperePerMeter(0.93e3);
			currentOnNmos[10] = AmperePerMeter(0.91e3);
			currentOnNmos[20] = AmperePerMeter(0.89e3);
			currentOnNmos[30] = AmperePerMeter(0.87e3);
			currentOnNmos[40] = AmperePerMeter(0.85e3);
			currentOnNmos[50] = AmperePerMeter(0.83e3);
			currentOnNmos[60] = AmperePerMeter(0.81e3);
			currentOnNmos[70] = AmperePerMeter(0.79e3);
			currentOnNmos[80] = AmperePerMeter(0.77e3);
			currentOnNmos[90] = AmperePerMeter(0.75e3);
			currentOnNmos[100] = AmperePerMeter(0.74e3);
			currentOnPmos[0] = AmperePerMeter(0.43e3);
			currentOnPmos[10] = AmperePerMeter(0.41e3);
			currentOnPmos[20] = AmperePerMeter(0.38e3);
			currentOnPmos[30] = AmperePerMeter(0.36e3);
			currentOnPmos[40] = AmperePerMeter(0.34e3);
			currentOnPmos[50] = AmperePerMeter(0.32e3);
			currentOnPmos[60] = AmperePerMeter(0.30e3);
			currentOnPmos[70] = AmperePerMeter(0.28e3);
			currentOnPmos[80] = AmperePerMeter(0.26e3);
			currentOnPmos[90] = AmperePerMeter(0.25e3);
			currentOnPmos[100] = AmperePerMeter(0.24e3);
			currentOffNmos[0] = AmperePerMeter(100.00e-3);
			currentOffNmos[10] = AmperePerMeter(119.90e-3);
			currentOffNmos[20] = AmperePerMeter(142.20e-3);
			currentOffNmos[30] = AmperePerMeter(167.00e-3);
			currentOffNmos[40] = AmperePerMeter(194.30e-3);
			currentOffNmos[50] = AmperePerMeter(224.30e-3);
			currentOffNmos[60] = AmperePerMeter(256.80e-3);
			currentOffNmos[70] = AmperePerMeter(292.00e-3);
			currentOffNmos[80] = AmperePerMeter(329.90e-3);
			currentOffNmos[90] = AmperePerMeter(370.50e-3);
			currentOffNmos[100] = AmperePerMeter(413.80e-3);
			currentOffPmos[0] = AmperePerMeter(100.20e-3);
			currentOffPmos[10] = AmperePerMeter(113.60e-3);
			currentOffPmos[20] = AmperePerMeter(127.90e-3);
			currentOffPmos[30] = AmperePerMeter(143.10e-3);
			currentOffPmos[40] = AmperePerMeter(159.10e-3);
			currentOffPmos[50] = AmperePerMeter(175.80e-3);
			currentOffPmos[60] = AmperePerMeter(193.40e-3);
			currentOffPmos[70] = AmperePerMeter(211.70e-3);
			currentOffPmos[80] = AmperePerMeter(230.80e-3);
			currentOffPmos[90] = AmperePerMeter(250.70e-3);
			currentOffPmos[100] = AmperePerMeter(271.20e-3);
			pnSizeRatio = currentOnNmos[0]/currentOnPmos[0];
		} else if (_deviceRoadmap == LOP) {
			/* PTM model: 130nm_LP.pm, from http://ptm.asu.edu/ */
			vdd = 1.3_V;
			vth = 466.0949e-3_V;
			vpp = vdd + vth;
			phyGateLength = 7.5e-8_m;
			capIdealGate = FaradPerMeter(1.8574e-9);
			capFringe = FaradPerMeter(9.530642e-10);
			capDrainTotal = capFringe/2;
			capJunction = FaradPerSquareMeter(1.00e-3);
			capOx = FaradPerSquareMeter(1.15e-2);
			effectiveElectronMobility = Mobility(284.97e-4);
			effectiveHoleMobility = Mobility(61.82e-4);
			effectiveResistanceMultiplier = 1.54;	/* from CACTI */
			transconductanceNmos = SiemensPerMeter(3.87E+01);
			transconductancePmos = SiemensPerMeter(5.67E+01);
			currentOnNmos[0] = AmperePerMeter(300.70);
			currentOnNmos[10] = AmperePerMeter(273.40);
			currentOnNmos[20] = AmperePerMeter(249.40);
			currentOnNmos[30] = AmperePerMeter(228.40);
			currentOnNmos[40] = AmperePerMeter(209.90);
			currentOnNmos[50] = AmperePerMeter(193.50);
			currentOnNmos[60] = AmperePerMeter(179.00);
			currentOnNmos[70] = AmperePerMeter(166.00);
			currentOnNmos[80] = AmperePerMeter(154.40);
			currentOnNmos[90] = AmperePerMeter(144.00);
			currentOnNmos[100] = AmperePerMeter(134.60);
			currentOnPmos[0] = AmperePerMeter(150.70);
			currentOnPmos[10] = AmperePerMeter(136.20);
			currentOnPmos[20] = AmperePerMeter(123.60);
			currentOnPmos[30] = AmperePerMeter(112.70);
			currentOnPmos[40] = AmperePerMeter(103.20);
			currentOnPmos[50] = AmperePerMeter(94.88);
			currentOnPmos[60] = AmperePerMeter(87.54);
			currentOnPmos[70] = AmperePerMeter(81.04);
			currentOnPmos[80] = AmperePerMeter(75.25);
			currentOnPmos[90] = AmperePerMeter(70.08);
			currentOnPmos[100] = AmperePerMeter(65.44);
			currentOffNmos[0] = AmperePerMeter(100.20e-6);
			currentOffNmos[10] = AmperePerMeter(135.90e-6);
			currentOffNmos[20] = AmperePerMeter(181.20e-6);
			currentOffNmos[30] = AmperePerMeter(237.80e-6);
			currentOffNmos[40] = AmperePerMeter(307.30e-6);
			currentOffNmos[50] = AmperePerMeter(391.90e-6);
			currentOffNmos[60] = AmperePerMeter(493.30e-6);
			currentOffNmos[70] = AmperePerMeter(613.70e-6);
			currentOffNmos[80] = AmperePerMeter(755.30e-6);
			currentOffNmos[90] = AmperePerMeter(920.20e-6);
			currentOffNmos[100] = AmperePerMeter(1111.0e-6);
			currentOffPmos[0] = AmperePerMeter(100.20e-6);
			currentOffPmos[10] = AmperePerMeter(132.80e-6);
			currentOffPmos[20] = AmperePerMeter(173.00e-6);
			currentOffPmos[30] = AmperePerMeter(221.90e-6);
			currentOffPmos[40] = AmperePerMeter(280.70e-6);
			currentOffPmos[50] = AmperePerMeter(350.40e-6);
			currentOffPmos[60] = AmperePerMeter(432.20e-6);
			currentOffPmos[70] = AmperePerMeter(527.20e-6);
			currentOffPmos[80] = AmperePerMeter(636.80e-6);
			currentOffPmos[90] = AmperePerMeter(761.90e-6);
			currentOffPmos[100] = AmperePerMeter(903.80e-6);
			pnSizeRatio = currentOnNmos[0]/currentOnPmos[0];
		} else {
            std::cout << "Unknown device roadmap!" << std::endl;
            exit(1);
		}
	} else if (_featureSizeInNano >= 90) {
		if (_deviceRoadmap == HP) {
			/* PTM model: 90nm_HP.pm, from http://ptm.asu.edu/ */
			vdd = 1.2_V;
			vth = 146.0217e-3_V;
			vpp = vdd + vth;
			phyGateLength = 5.5e-8_m;
			capIdealGate = FaradPerMeter(5.694423e-10);
			capFringe = FaradPerMeter(5.652302e-10);
			capJunction = FaradPerSquareMeter(1.00e-3);
			capOx = FaradPerSquareMeter(1.73e-2);
			effectiveElectronMobility = Mobility(243.43e-4);
			effectiveHoleMobility = Mobility(58.32e-4);
			effectiveResistanceMultiplier = 1.54;	/* from CACTI */
			transconductanceNmos = SiemensPerMeter(4.95E+02);
			transconductancePmos = SiemensPerMeter(3.16E+02);
			currentOnNmos[0] = AmperePerMeter(1.07e3);
			currentOnNmos[10] = AmperePerMeter(1.05e3);
			currentOnNmos[20] = AmperePerMeter(1.03e3);
			currentOnNmos[30] = AmperePerMeter(1.01e3);
			currentOnNmos[40] = AmperePerMeter(0.99e3);
			currentOnNmos[50] = AmperePerMeter(0.97e3);
			currentOnNmos[60] = AmperePerMeter(0.95e3);
			currentOnNmos[70] = AmperePerMeter(0.93e3);
			currentOnNmos[80] = AmperePerMeter(0.90e3);
			currentOnNmos[90] = AmperePerMeter(0.88e3);
			currentOnNmos[100] = AmperePerMeter(0.86e3);
			currentOnPmos[0] = AmperePerMeter(0.54e3);
			currentOnPmos[10] = AmperePerMeter(0.50e3);
			currentOnPmos[20] = AmperePerMeter(0.47e3);
			currentOnPmos[30] = AmperePerMeter(0.44e3);
			currentOnPmos[40] = AmperePerMeter(0.41e3);
			currentOnPmos[50] = AmperePerMeter(0.39e3);
			currentOnPmos[60] = AmperePerMeter(0.37e3);
			currentOnPmos[70] = AmperePerMeter(0.34e3);
			currentOnPmos[80] = AmperePerMeter(0.32e3);
			currentOnPmos[90] = AmperePerMeter(0.31e3);
			currentOnPmos[100] = AmperePerMeter(0.29e3);
			currentOffNmos[0] = AmperePerMeter(100.8e-3);
			currentOffNmos[10] = AmperePerMeter(120.8e-3);
			currentOffNmos[20] = AmperePerMeter(143.4e-3);
			currentOffNmos[30] = AmperePerMeter(168.6e-3);
			currentOffNmos[40] = AmperePerMeter(196.6e-3);
			currentOffNmos[50] = AmperePerMeter(227.4e-3);
			currentOffNmos[60] = AmperePerMeter(261.1e-3);
			currentOffNmos[70] = AmperePerMeter(297.7e-3);
			currentOffNmos[80] = AmperePerMeter(337.3e-3);
			currentOffNmos[90] = AmperePerMeter(379.8e-3);
			currentOffNmos[100] = AmperePerMeter(425.4e-3);
			currentOffPmos[0] = AmperePerMeter(100.00e-3);
			currentOffPmos[10] = AmperePerMeter(114.00e-3);
			currentOffPmos[20] = AmperePerMeter(128.90e-3);
			currentOffPmos[30] = AmperePerMeter(144.80e-3);
			currentOffPmos[40] = AmperePerMeter(161.60e-3);
			currentOffPmos[50] = AmperePerMeter(179.30e-3);
			currentOffPmos[60] = AmperePerMeter(197.90e-3);
			currentOffPmos[70] = AmperePerMeter(217.40e-3);
			currentOffPmos[80] = AmperePerMeter(237.90e-3);
			currentOffPmos[90] = AmperePerMeter(259.10e-3);
			currentOffPmos[100] = AmperePerMeter(281.30e-3);
			pnSizeRatio = currentOnNmos[0]/currentOnPmos[0];
		} else if (_deviceRoadmap == LOP) {
			/* PTM model: 90nm_LP.pm, from http://ptm.asu.edu/ */
			vdd = 1.2_V;
			vth = 501.3229e-3_V;
			vpp = vdd + vth;
			phyGateLength = 5.5e-8_m;
			capIdealGate = FaradPerMeter(1.5413e-9);
			capFringe = FaradPerMeter(9.601334e-10);
			capJunction = FaradPerSquareMeter(1.00e-3);
			capOx = FaradPerSquareMeter(1.19e-2);
			effectiveElectronMobility = Mobility(277.94e-4);
			effectiveHoleMobility = Mobility(60.64e-4);
			effectiveResistanceMultiplier = 1.77;	/* from CACTI */
			transconductanceNmos = SiemensPerMeter(4.38E+01);
			transconductancePmos = SiemensPerMeter(5.99E+01);
			currentOnNmos[0] = AmperePerMeter(346.30);
			currentOnNmos[10] = AmperePerMeter(314.50);
			currentOnNmos[20] = AmperePerMeter(286.80);
			currentOnNmos[30] = AmperePerMeter(262.50);
			currentOnNmos[40] = AmperePerMeter(241.20);
			currentOnNmos[50] = AmperePerMeter(222.30);
			currentOnNmos[60] = AmperePerMeter(205.60);
			currentOnNmos[70] = AmperePerMeter(190.80);
			currentOnNmos[80] = AmperePerMeter(177.50);
			currentOnNmos[90] = AmperePerMeter(165.60);
			currentOnNmos[100] = AmperePerMeter(155.00);
			currentOnPmos[0] = AmperePerMeter(200.30);
			currentOnPmos[10] = AmperePerMeter(179.50);
			currentOnPmos[20] = AmperePerMeter(161.90);
			currentOnPmos[30] = AmperePerMeter(146.90);
			currentOnPmos[40] = AmperePerMeter(133.90);
			currentOnPmos[50] = AmperePerMeter(122.60);
			currentOnPmos[60] = AmperePerMeter(112.80);
			currentOnPmos[70] = AmperePerMeter(104.10);
			currentOnPmos[80] = AmperePerMeter(96.47);
			currentOnPmos[90] = AmperePerMeter(89.68);
			currentOnPmos[100] = AmperePerMeter(83.62);
			currentOffNmos[0] = AmperePerMeter(100.00e-6);
			currentOffNmos[10] = AmperePerMeter(135.70e-6);
			currentOffNmos[20] = AmperePerMeter(181.10e-6);
			currentOffNmos[30] = AmperePerMeter(238.00e-6);
			currentOffNmos[40] = AmperePerMeter(308.50e-6);
			currentOffNmos[50] = AmperePerMeter(394.60e-6);
			currentOffNmos[60] = AmperePerMeter(498.50e-6);
			currentOffNmos[70] = AmperePerMeter(622.60e-6);
			currentOffNmos[80] = AmperePerMeter(769.30e-6);
			currentOffNmos[90] = AmperePerMeter(941.20e-6);
			currentOffNmos[100] = AmperePerMeter(1141.0e-6);
			currentOffPmos[0] = AmperePerMeter(100.30e-6);
			currentOffPmos[10] = AmperePerMeter(133.20e-6);
			currentOffPmos[20] = AmperePerMeter(174.20e-6);
			currentOffPmos[30] = AmperePerMeter(224.40e-6);
			currentOffPmos[40] = AmperePerMeter(285.10e-6);
			currentOffPmos[50] = AmperePerMeter(357.60e-6);
			currentOffPmos[60] = AmperePerMeter(443.40e-6);
			currentOffPmos[70] = AmperePerMeter(543.70e-6);
			currentOffPmos[80] = AmperePerMeter(660.00e-6);
			currentOffPmos[90] = AmperePerMeter(793.80e-6);
			currentOffPmos[100] = AmperePerMeter(946.40e-6);
			pnSizeRatio = currentOnNmos[0]/currentOnPmos[0];
		} else {
            std::cout << "Unknown device roadmap!" << std::endl;
            exit(1);
		}
	} else if (_featureSizeInNano >= 65) {
		if (_deviceRoadmap == HP) {
			/* PTM model: 65nm_HP.pm, from http://ptm.asu.edu/ */
			vdd = 1.1_V;
			vth = 166.3941e-3_V;
			vpp = vdd + vth;
			phyGateLength = 3.5e-8_m;
			capIdealGate = FaradPerMeter(4.868295e-10);
			capFringe = FaradPerMeter(5.270361e-10);
			capDrainTotal = capFringe/2;
			capJunction = FaradPerSquareMeter(1.00e-3);
			capOx = FaradPerSquareMeter(1.88e-2);
			effectiveElectronMobility = Mobility(445.74e-4);
			effectiveHoleMobility = Mobility(113.330e-4);
			effectiveResistanceMultiplier = 1.54;	/* from CACTI */
			transconductanceNmos = SiemensPerMeter(5.72E+02);
			transconductancePmos = SiemensPerMeter(3.99E+02);
			currentOnNmos[0] = AmperePerMeter(1.12e3);
			currentOnNmos[10] = AmperePerMeter(1.10e3);
			currentOnNmos[20] = AmperePerMeter(1.08e3);
			currentOnNmos[30] = AmperePerMeter(1.06e3);
			currentOnNmos[40] = AmperePerMeter(1.04e3);
			currentOnNmos[50] = AmperePerMeter(1.02e3);
			currentOnNmos[60] = AmperePerMeter(1.00e3);
			currentOnNmos[70] = AmperePerMeter(0.98e3);
			currentOnNmos[80] = AmperePerMeter(0.95e3);
			currentOnNmos[90] = AmperePerMeter(0.93e3);
			currentOnNmos[100] = AmperePerMeter(0.91e3);
			currentOnPmos[0] = AmperePerMeter(0.70e3);
			currentOnPmos[10] = AmperePerMeter(0.66e3);
			currentOnPmos[20] = AmperePerMeter(0.62e3);
			currentOnPmos[30] = AmperePerMeter(0.58e3);
			currentOnPmos[40] = AmperePerMeter(0.55e3);
			currentOnPmos[50] = AmperePerMeter(0.52e3);
			currentOnPmos[60] = AmperePerMeter(0.49e3);
			currentOnPmos[70] = AmperePerMeter(0.46e3);
			currentOnPmos[80] = AmperePerMeter(0.44e3);
			currentOnPmos[90] = AmperePerMeter(0.41e3);
			currentOnPmos[100] = AmperePerMeter(0.39e3);
			currentOffNmos[0] = AmperePerMeter(100.00e-3);
			currentOffNmos[10] = AmperePerMeter(119.70e-3);
			currentOffNmos[20] = AmperePerMeter(141.90e-3);
			currentOffNmos[30] = AmperePerMeter(166.80e-3);
			currentOffNmos[40] = AmperePerMeter(194.40e-3);
			currentOffNmos[50] = AmperePerMeter(224.80e-3);
			currentOffNmos[60] = AmperePerMeter(258.10e-3);
			currentOffNmos[70] = AmperePerMeter(294.40e-3);
			currentOffNmos[80] = AmperePerMeter(333.60e-3);
			currentOffNmos[90] = AmperePerMeter(375.90e-3);
			currentOffNmos[100] = AmperePerMeter(421.20e-3);
			currentOffPmos[0] = AmperePerMeter(100.10e-3);
			currentOffPmos[10] = AmperePerMeter(115.20e-3);
			currentOffPmos[20] = AmperePerMeter(131.50e-3);
			currentOffPmos[30] = AmperePerMeter(149.00e-3);
			currentOffPmos[40] = AmperePerMeter(167.60e-3);
			currentOffPmos[50] = AmperePerMeter(187.40e-3);
			currentOffPmos[60] = AmperePerMeter(208.40e-3);
			currentOffPmos[70] = AmperePerMeter(230.50e-3);
			currentOffPmos[80] = AmperePerMeter(253.70e-3);
			currentOffPmos[90] = AmperePerMeter(278.10e-3);
			currentOffPmos[100] = AmperePerMeter(303.60e-3);
			pnSizeRatio = currentOnNmos[0]/currentOnPmos[0];
		} else if (_deviceRoadmap == LOP) {
			/* PTM model: 65nm_LP.pm, from http://ptm.asu.edu/ */
			vdd = 1.1_V;
			vth = 501.6636e-3_V;
			vpp = vdd + vth;
			phyGateLength = 3.5e-8_m;
			capIdealGate = FaradPerMeter(1.1926e-9);
			capFringe = FaradPerMeter(9.62148e-10);
			capJunction = FaradPerSquareMeter(1.00e-3);
			capOx = FaradPerSquareMeter(1.37e-2);
			effectiveElectronMobility = Mobility(457.86e-4);
			effectiveHoleMobility = Mobility(102.64e-4);
			effectiveResistanceMultiplier = 1.77;	/* from CACTI */
			transconductanceNmos = SiemensPerMeter(5.90E+01);
			transconductancePmos = SiemensPerMeter(6.75E+01);
			currentOnNmos[0] = AmperePerMeter(400.00);
			currentOnNmos[10] = AmperePerMeter(363.90);
			currentOnNmos[20] = AmperePerMeter(332.30);
			currentOnNmos[30] = AmperePerMeter(304.70);
			currentOnNmos[40] = AmperePerMeter(280.40);
			currentOnNmos[50] = AmperePerMeter(258.90);
			currentOnNmos[60] = AmperePerMeter(239.90);
			currentOnNmos[70] = AmperePerMeter(223.00);
			currentOnNmos[80] = AmperePerMeter(207.90);
			currentOnNmos[90] = AmperePerMeter(194.30);
			currentOnNmos[100] = AmperePerMeter(182.10);
			currentOnPmos[0] = AmperePerMeter(238.70);
			currentOnPmos[10] = AmperePerMeter(216.10);
			currentOnPmos[20] = AmperePerMeter(196.60);
			currentOnPmos[30] = AmperePerMeter(179.70);
			currentOnPmos[40] = AmperePerMeter(164.90);
			currentOnPmos[50] = AmperePerMeter(152.00);
			currentOnPmos[60] = AmperePerMeter(140.50);
			currentOnPmos[70] = AmperePerMeter(130.40);
			currentOnPmos[80] = AmperePerMeter(121.40);
			currentOnPmos[90] = AmperePerMeter(113.30);
			currentOnPmos[100] = AmperePerMeter(106.10);
			currentOffNmos[0] = AmperePerMeter(100.20e-6);
			currentOffNmos[10] = AmperePerMeter(137.50e-6);
			currentOffNmos[20] = AmperePerMeter(185.80e-6);
			currentOffNmos[30] = AmperePerMeter(247.20e-6);
			currentOffNmos[40] = AmperePerMeter(324.20e-6);
			currentOffNmos[50] = AmperePerMeter(419.30e-6);
			currentOffNmos[60] = AmperePerMeter(535.40e-6);
			currentOffNmos[70] = AmperePerMeter(675.70e-6);
			currentOffNmos[80] = AmperePerMeter(843.100e-6);
			currentOffNmos[90] = AmperePerMeter(1041.00e-6);
			currentOffNmos[100] = AmperePerMeter(1273.00e-6);
			currentOffPmos[0] = AmperePerMeter(100.20e-6);
			currentOffPmos[10] = AmperePerMeter(135.40e-6);
			currentOffPmos[20] = AmperePerMeter(179.70e-6);
			currentOffPmos[30] = AmperePerMeter(234.90e-6);
			currentOffPmos[40] = AmperePerMeter(302.50e-6);
			currentOffPmos[50] = AmperePerMeter(384.30e-6);
			currentOffPmos[60] = AmperePerMeter(482.20e-6);
			currentOffPmos[70] = AmperePerMeter(598.00e-6);
			currentOffPmos[80] = AmperePerMeter(733.90e-6);
			currentOffPmos[90] = AmperePerMeter(891.60e-6);
			currentOffPmos[100] = AmperePerMeter(1073.00e-6);
			pnSizeRatio = currentOnNmos[0]/currentOnPmos[0];
		 // Largernode dummy for interpolation only
		} else if (_deviceRoadmap == IGZO) {
			/* IGZO TFT - virtual source model derived */
			vdd = 1.4_V;
			vpp = 1.4_V;
			vth = 0.667_V;
			phyGateLength = 0.044e-6_m;
			capIdealGate = FaradPerMeter(8.93e-16);
			capFringe = FaradPerMeter(6.51e-17);
			capOx = FaradPerSquareMeter(2.03e-2);
			effectiveElectronMobility = Mobility(10.0e-4); // Mobility of 10 cm^2/Vs
			pnSizeRatio = 1.0;
			effectiveResistanceMultiplier = 2.0;

			// NMOS ON-current, Unit: μA/μm 
			currentOnNmos[0] = AmperePerMeter(155.6);
			currentOnNmos[10] = AmperePerMeter(153.0);
			currentOnNmos[20] = AmperePerMeter(150.1);
			currentOnNmos[30] = AmperePerMeter(147.6);
			currentOnNmos[40] = AmperePerMeter(145.2);
			currentOnNmos[50] = AmperePerMeter(142.7);
			currentOnNmos[60] = AmperePerMeter(140.2);
			currentOnNmos[70] = AmperePerMeter(137.6);
			currentOnNmos[80] = AmperePerMeter(135.1);
			currentOnNmos[90] = AmperePerMeter(132.5);
			currentOnNmos[100] = AmperePerMeter(130.0);

			// PMOS ON-current stays zero (IGZO has no PMOS)

			// NMOS off current, Unit: A/m
			currentOffNmos[0] = AmperePerMeter(6.79e-7);
			currentOffNmos[10] = AmperePerMeter(1.358e-6);
			currentOffNmos[20] = AmperePerMeter(2.716e-6);
			currentOffNmos[30] = AmperePerMeter(5.432e-6);
			currentOffNmos[40] = AmperePerMeter(1.0864e-5);
			currentOffNmos[50] = AmperePerMeter(2.1728e-5);
			currentOffNmos[60] = AmperePerMeter(4.3456e-5);
			currentOffNmos[70] = AmperePerMeter(8.6912e-5);
			currentOffNmos[80] = AmperePerMeter(1.73824e-4);
			currentOffNmos[90] = AmperePerMeter(3.47648e-4);
			currentOffNmos[100] = AmperePerMeter(6.95296e-4);

			// PMOS off current stays zero (N/A in IGZO)
		} else {
            std::cout << "Unknown device roadmap!" << std::endl;
            exit(1);
		}
	} else if (_featureSizeInNano >= 45) {
		if (_deviceRoadmap == HP) {
			/* PTM model: 45nm_HP.pm, from http://ptm.asu.edu/ */
			vdd = 1.0_V;
			vth = 171.0969e-3_V;
			vpp = vdd + vth;
			phyGateLength = 3.0e-8_m;
			capIdealGate = FaradPerMeter(4.091305e-10);
			capFringe = FaradPerMeter(4.957928e-10);
			capJunction = FaradPerSquareMeter(1.00e-3);
			capOx = FaradPerSquareMeter(3.77e-2);
			effectiveElectronMobility = Mobility(297.70e-4);
			effectiveHoleMobility = Mobility(95.27e-4);
			effectiveResistanceMultiplier = 1.54;	/* from CACTI */
			transconductanceNmos = SiemensPerMeter(7.37E+02);
			transconductancePmos = SiemensPerMeter(6.30E+02);
			currentOnNmos[0] = AmperePerMeter(1.27e3);
			currentOnNmos[10] = AmperePerMeter(1.24e3);
			currentOnNmos[20] = AmperePerMeter(1.22e3);
			currentOnNmos[30] = AmperePerMeter(1.19e3);
			currentOnNmos[40] = AmperePerMeter(1.16e3);
			currentOnNmos[50] = AmperePerMeter(1.13e3);
			currentOnNmos[60] = AmperePerMeter(1.11e3);
			currentOnNmos[70] = AmperePerMeter(1.08e3);
			currentOnNmos[80] = AmperePerMeter(1.05e3);
			currentOnNmos[90] = AmperePerMeter(1.02e3);
			currentOnNmos[100] = AmperePerMeter(1.00e3);
			currentOnPmos[0] = AmperePerMeter(1.08e3);
			currentOnPmos[10] = AmperePerMeter(1.04e3);
			currentOnPmos[20] = AmperePerMeter(1.00e3);
			currentOnPmos[30] = AmperePerMeter(0.96e3);
			currentOnPmos[40] = AmperePerMeter(0.92e3);
			currentOnPmos[50] = AmperePerMeter(0.88e3);
			currentOnPmos[60] = AmperePerMeter(0.85e3);
			currentOnPmos[70] = AmperePerMeter(0.81e3);
			currentOnPmos[80] = AmperePerMeter(0.78e3);
			currentOnPmos[90] = AmperePerMeter(0.75e3);
			currentOnPmos[100] = AmperePerMeter(0.72e3);
			currentOffNmos[0] = AmperePerMeter(100.00e-3);
			currentOffNmos[10] = AmperePerMeter(120.70e-3);
			currentOffNmos[20] = AmperePerMeter(144.10e-3);
			currentOffNmos[30] = AmperePerMeter(170.50e-3);
			currentOffNmos[40] = AmperePerMeter(199.80e-3);
			currentOffNmos[50] = AmperePerMeter(232.30e-3);
			currentOffNmos[60] = AmperePerMeter(268.00e-3);
			currentOffNmos[70] = AmperePerMeter(307.10e-3);
			currentOffNmos[80] = AmperePerMeter(349.50e-3);
			currentOffNmos[90] = AmperePerMeter(395.40e-3);
			currentOffNmos[100] = AmperePerMeter(444.80e-3);
			currentOffPmos[0] = AmperePerMeter(100.20e-3);
			currentOffPmos[10] = AmperePerMeter(118.70e-3);
			currentOffPmos[20] = AmperePerMeter(139.30e-3);
			currentOffPmos[30] = AmperePerMeter(162.00e-3);
			currentOffPmos[40] = AmperePerMeter(186.80e-3);
			currentOffPmos[50] = AmperePerMeter(213.90e-3);
			currentOffPmos[60] = AmperePerMeter(243.30e-3);
			currentOffPmos[70] = AmperePerMeter(274.90e-3);
			currentOffPmos[80] = AmperePerMeter(308.90e-3);
			currentOffPmos[90] = AmperePerMeter(345.20e-3);
			currentOffPmos[100] = AmperePerMeter(383.80e-3);
			pnSizeRatio = currentOnNmos[0]/currentOnPmos[0];
		} else if (_deviceRoadmap == LOP) {
			/* PTM model: 45nm_LP.pm, from http://ptm.asu.edu/ */
			vdd = 1_V;
			vth = 464.3718e-3_V;
			vpp = vdd + vth;
			phyGateLength = 3.0e-8_m;
			capIdealGate = FaradPerMeter(8.930709e-10);
			capFringe = FaradPerMeter(8.849901e-10);
			capJunction = FaradPerSquareMeter(1.00e-3);
			capOx = FaradPerSquareMeter(1.99e-2);
			effectiveElectronMobility = Mobility(456.14e-4);
			effectiveHoleMobility = Mobility(96.98e-4);
			effectiveResistanceMultiplier = 1.77;	/* from CACTI */
			transconductanceNmos = SiemensPerMeter(1.32E+02);
			transconductancePmos = SiemensPerMeter(8.65E+01);
			currentOnNmos[0] = AmperePerMeter(500.20);
			currentOnNmos[10] = AmperePerMeter(462.00);
			currentOnNmos[20] = AmperePerMeter(427.80);
			currentOnNmos[30] = AmperePerMeter(397.10);
			currentOnNmos[40] = AmperePerMeter(369.40);
			currentOnNmos[50] = AmperePerMeter(344.50);
			currentOnNmos[60] = AmperePerMeter(322.10);
			currentOnNmos[70] = AmperePerMeter(301.80);
			currentOnNmos[80] = AmperePerMeter(283.40);
			currentOnNmos[90] = AmperePerMeter(266.70);
			currentOnNmos[100] = AmperePerMeter(251.50);
			currentOnPmos[0] = AmperePerMeter(300.00);
			currentOnPmos[10] = AmperePerMeter(275.70);
			currentOnPmos[20] = AmperePerMeter(254.20);
			currentOnPmos[30] = AmperePerMeter(235.10);
			currentOnPmos[40] = AmperePerMeter(218.10);
			currentOnPmos[50] = AmperePerMeter(202.80);
			currentOnPmos[60] = AmperePerMeter(189.20);
			currentOnPmos[70] = AmperePerMeter(176.90);
			currentOnPmos[80] = AmperePerMeter(165.80);
			currentOnPmos[90] = AmperePerMeter(155.80);
			currentOnPmos[100] = AmperePerMeter(146.70);
			currentOffNmos[0] = AmperePerMeter(100.00e-6);
			currentOffNmos[10] = AmperePerMeter(140.50e-6);
			currentOffNmos[20] = AmperePerMeter(193.90e-6);
			currentOffNmos[30] = AmperePerMeter(263.10e-6);
			currentOffNmos[40] = AmperePerMeter(351.40e-6);
			currentOffNmos[50] = AmperePerMeter(462.50e-6);
			currentOffNmos[60] = AmperePerMeter(600.30e-6);
			currentOffNmos[70] = AmperePerMeter(769.20e-6);
			currentOffNmos[80] = AmperePerMeter(973.900e-6);
			currentOffNmos[90] = AmperePerMeter(1219.00e-6);
			currentOffNmos[100] = AmperePerMeter(1511.00e-6);
			currentOffPmos[0] = AmperePerMeter(100.20e-6);
			currentOffPmos[10] = AmperePerMeter(138.40e-6);
			currentOffPmos[20] = AmperePerMeter(187.60e-6);
			currentOffPmos[30] = AmperePerMeter(250.10e-6);
			currentOffPmos[40] = AmperePerMeter(328.10e-6);
			currentOffPmos[50] = AmperePerMeter(424.10e-6);
			currentOffPmos[60] = AmperePerMeter(540.90e-6);
			currentOffPmos[70] = AmperePerMeter(681.30e-6);
			currentOffPmos[80] = AmperePerMeter(848.30e-6);
			currentOffPmos[90] = AmperePerMeter(1045.00e-6);
			currentOffPmos[100] = AmperePerMeter(1275.00e-6);
			pnSizeRatio = currentOnNmos[0]/currentOnPmos[0];
		} else if (_deviceRoadmap == IGZO) {
			/* IGZO TFT - virtual source model derived */
			vdd = 1.4_V;
			vpp = 1.4_V;
			vth = 0.667_V;
			phyGateLength = 0.044e-6_m;
			capIdealGate = FaradPerMeter(8.93e-16);
			capFringe = FaradPerMeter(6.51e-17);
			capOx = FaradPerSquareMeter(2.03e-2);
			effectiveElectronMobility = Mobility(10.0e-4); // Mobility of 10 cm^2/Vs
			pnSizeRatio = 1.0;
			effectiveResistanceMultiplier = 2.0;

			// NMOS ON-current, Unit: μA/μm approximately scaled with temperature
			currentOnNmos[0] = AmperePerMeter(155.6);
			currentOnNmos[10] = AmperePerMeter(153.0);
			currentOnNmos[20] = AmperePerMeter(150.1);
			currentOnNmos[30] = AmperePerMeter(147.6);
			currentOnNmos[40] = AmperePerMeter(145.2);
			currentOnNmos[50] = AmperePerMeter(142.7);
			currentOnNmos[60] = AmperePerMeter(140.2);
			currentOnNmos[70] = AmperePerMeter(137.6);
			currentOnNmos[80] = AmperePerMeter(135.1);
			currentOnNmos[90] = AmperePerMeter(132.5);
			currentOnNmos[100] = AmperePerMeter(130.0);

			// PMOS ON-current stays zero (IGZO has no PMOS)

			// NMOS off current, Unit: A/m
			currentOffNmos[0] = AmperePerMeter(6.79e-7);
			currentOffNmos[10] = AmperePerMeter(1.358e-6);
			currentOffNmos[20] = AmperePerMeter(2.716e-6);
			currentOffNmos[30] = AmperePerMeter(5.432e-6);
			currentOffNmos[40] = AmperePerMeter(1.0864e-5);
			currentOffNmos[50] = AmperePerMeter(2.1728e-5);
			currentOffNmos[60] = AmperePerMeter(4.3456e-5);
			currentOffNmos[70] = AmperePerMeter(8.6912e-5);
			currentOffNmos[80] = AmperePerMeter(1.73824e-4);
			currentOffNmos[90] = AmperePerMeter(3.47648e-4);
			currentOffNmos[100] = AmperePerMeter(6.95296e-4);

			// PMOS off current stays zero (N/A in IGZO)
		} else {
            std::cout << "Unknown device roadmap!" << std::endl;
            exit(1);
		}
	} else if (_featureSizeInNano >= 32) {	/* TODO: actually 36nm */
		if (_deviceRoadmap == HP) {
			/* PTM model: 32nm_HP.pm, from http://ptm.asu.edu/ */
			vdd = 0.9_V;
			vth = 194.4951e-3_V;
			vpp = vdd + vth;
			phyGateLength = 2.8e-8_m;
			capIdealGate = FaradPerMeter(3.767721e-10);
			capFringe = FaradPerMeter(4.713762e-10);
			capJunction = FaradPerSquareMeter(1.00e-3);
			capOx = FaradPerSquareMeter(4.59e-2);
			effectiveElectronMobility = Mobility(257.73e-4);
			effectiveHoleMobility = Mobility(89.92e-4);
			effectiveResistanceMultiplier = 1.54;	/* from CACTI */
			transconductanceNmos = SiemensPerMeter(9.29E+02);
			transconductancePmos = SiemensPerMeter(6.73E+02);
			currentOnNmos[0] = AmperePerMeter(1.41e3);
			currentOnNmos[10] = AmperePerMeter(1.38e3);
			currentOnNmos[20] = AmperePerMeter(1.35e3);
			currentOnNmos[30] = AmperePerMeter(1.31e3);
			currentOnNmos[40] = AmperePerMeter(1.28e3);
			currentOnNmos[50] = AmperePerMeter(1.25e3);
			currentOnNmos[60] = AmperePerMeter(1.21e3);
			currentOnNmos[70] = AmperePerMeter(1.18e3);
			currentOnNmos[80] = AmperePerMeter(1.15e3);
			currentOnNmos[90] = AmperePerMeter(1.12e3);
			currentOnNmos[100] = AmperePerMeter(1.08e3);
			currentOnPmos[0] = AmperePerMeter(1.22e3);
			currentOnPmos[10] = AmperePerMeter(1.17e3);
			currentOnPmos[20] = AmperePerMeter(1.12e3);
			currentOnPmos[30] = AmperePerMeter(1.07e3);
			currentOnPmos[40] = AmperePerMeter(1.02e3);
			currentOnPmos[50] = AmperePerMeter(0.98e3);
			currentOnPmos[60] = AmperePerMeter(0.94e3);
			currentOnPmos[70] = AmperePerMeter(0.89e3);
			currentOnPmos[80] = AmperePerMeter(0.86e3);
			currentOnPmos[90] = AmperePerMeter(0.82e3);
			currentOnPmos[100] = AmperePerMeter(0.78e3);
			currentOffNmos[0] = AmperePerMeter(100.30e-3);
			currentOffNmos[10] = AmperePerMeter(120.40e-3);
			currentOffNmos[20] = AmperePerMeter(143.10e-3);
			currentOffNmos[30] = AmperePerMeter(168.60e-3);
			currentOffNmos[40] = AmperePerMeter(197.00e-3);
			currentOffNmos[50] = AmperePerMeter(228.40e-3);
			currentOffNmos[60] = AmperePerMeter(262.90e-3);
			currentOffNmos[70] = AmperePerMeter(300.60e-3);
			currentOffNmos[80] = AmperePerMeter(341.70e-3);
			currentOffNmos[90] = AmperePerMeter(386.10e-3);
			currentOffNmos[100] = AmperePerMeter(433.90e-3);
			currentOffPmos[0] = AmperePerMeter(100.10e-3);
			currentOffPmos[10] = AmperePerMeter(119.00e-3);
			currentOffPmos[20] = AmperePerMeter(140.00e-3);
			currentOffPmos[30] = AmperePerMeter(163.30e-3);
			currentOffPmos[40] = AmperePerMeter(188.80e-3);
			currentOffPmos[50] = AmperePerMeter(216.70e-3);
			currentOffPmos[60] = AmperePerMeter(247.00e-3);
			currentOffPmos[70] = AmperePerMeter(279.70e-3);
			currentOffPmos[80] = AmperePerMeter(314.90e-3);
			currentOffPmos[90] = AmperePerMeter(352.60e-3);
			currentOffPmos[100] = AmperePerMeter(392.80e-3);
			pnSizeRatio = currentOnNmos[0]/currentOnPmos[0];
		} else if (_deviceRoadmap == LOP) {
			/* PTM model: 32nm_LP.pm, from http://ptm.asu.edu/ */
			vdd = 0.9_V;
			vth = 442.034e-3_V;
			vpp = vdd + vth;
			phyGateLength = 2.8e-8_m;
			capIdealGate = FaradPerMeter(8.375279e-10);
			capFringe = FaradPerMeter(6.856677e-10);
			capJunction = FaradPerSquareMeter(1.00e-3);
			capOx = FaradPerSquareMeter(2.19e-2);
			effectiveElectronMobility = Mobility(395.20e-4);
			effectiveHoleMobility = Mobility(88.67e-4);
			effectiveResistanceMultiplier = 1.77;	/* from CACTI */
			transconductanceNmos = SiemensPerMeter(2.56E+02);
			transconductancePmos = SiemensPerMeter(1.19E+02);
			currentOnNmos[0] = AmperePerMeter(600.20);
			currentOnNmos[10] = AmperePerMeter(562.80);
			currentOnNmos[20] = AmperePerMeter(528.20);
			currentOnNmos[30] = AmperePerMeter(496.20);
			currentOnNmos[40] = AmperePerMeter(466.80);
			currentOnNmos[50] = AmperePerMeter(439.70);
			currentOnNmos[60] = AmperePerMeter(414.80);
			currentOnNmos[70] = AmperePerMeter(391.90);
			currentOnNmos[80] = AmperePerMeter(370.70);
			currentOnNmos[90] = AmperePerMeter(351.30);
			currentOnNmos[100] = AmperePerMeter(333.30);
			currentOnPmos[0] = AmperePerMeter(400.00);
			currentOnPmos[10] = AmperePerMeter(368.40);
			currentOnPmos[20] = AmperePerMeter(340.30);
			currentOnPmos[30] = AmperePerMeter(315.30);
			currentOnPmos[40] = AmperePerMeter(292.90);
			currentOnPmos[50] = AmperePerMeter(272.80);
			currentOnPmos[60] = AmperePerMeter(254.80);
			currentOnPmos[70] = AmperePerMeter(238.50);
			currentOnPmos[80] = AmperePerMeter(223.80);
			currentOnPmos[90] = AmperePerMeter(210.50);
			currentOnPmos[100] = AmperePerMeter(198.40);
			currentOffNmos[0] = AmperePerMeter(100.10e-6);
			currentOffNmos[10] = AmperePerMeter(143.60e-6);
			currentOffNmos[20] = AmperePerMeter(202.10e-6);
			currentOffNmos[30] = AmperePerMeter(279.30e-6);
			currentOffNmos[40] = AmperePerMeter(379.50e-6);
			currentOffNmos[50] = AmperePerMeter(507.50e-6);
			currentOffNmos[60] = AmperePerMeter(668.80e-6);
			currentOffNmos[70] = AmperePerMeter(869.20e-6);
			currentOffNmos[80] = AmperePerMeter(1115.00e-6);
			currentOffNmos[90] = AmperePerMeter(1415.00e-6);
			currentOffNmos[100] = AmperePerMeter(1774.00e-6);
			currentOffPmos[0] = AmperePerMeter(100.10e-6);
			currentOffPmos[10] = AmperePerMeter(140.70e-6);
			currentOffPmos[20] = AmperePerMeter(194.00e-6);
			currentOffPmos[30] = AmperePerMeter(262.50e-6);
			currentOffPmos[40] = AmperePerMeter(349.30e-6);
			currentOffPmos[50] = AmperePerMeter(457.70e-6);
			currentOffPmos[60] = AmperePerMeter(591.20e-6);
			currentOffPmos[70] = AmperePerMeter(753.70e-6);
			currentOffPmos[80] = AmperePerMeter(949.30e-6);
			currentOffPmos[90] = AmperePerMeter(1182.00e-6);
			currentOffPmos[100] = AmperePerMeter(1457.00e-6);
			pnSizeRatio = currentOnNmos[0]/currentOnPmos[0];
		} else if (_deviceRoadmap == CNT) { // Larger node dummy for interpolation
			/* CNFET @ 20nm gate length */
			vdd = 0.7_V;
			vth = 0.18_V;
			vpp = vdd + vth;
			phyGateLength = 0.020e-6_m;
			capIdealGate = FaradPerMeter(1.18e-15);
			capFringe = FaradPerMeter(4.18e-17);
			capOx = FaradPerSquareMeter(5.90e-2);
			effectiveElectronMobility = Mobility(78700e-4);
			effectiveHoleMobility = Mobility(78700e-4);
			pnSizeRatio = 1.0;
			effectiveResistanceMultiplier = 2.0;

			currentOnNmos[0] = AmperePerMeter(3361.1);
			currentOnNmos[10] = AmperePerMeter(3300.0);
			currentOnNmos[20] = AmperePerMeter(3240.0);
			currentOnNmos[30] = AmperePerMeter(3180.0);
			currentOnNmos[40] = AmperePerMeter(3120.0);
			currentOnNmos[50] = AmperePerMeter(3060.0);
			currentOnNmos[60] = AmperePerMeter(3000.0);
			currentOnNmos[70] = AmperePerMeter(2940.0);
			currentOnNmos[80] = AmperePerMeter(2880.0);
			currentOnNmos[90] = AmperePerMeter(2820.0);
			currentOnNmos[100] = AmperePerMeter(2760.0);

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

			currentOffNmos[0] = AmperePerMeter(8.94e-2);
			currentOffNmos[10] = AmperePerMeter(1.00e-1);
			currentOffNmos[20] = AmperePerMeter(1.12e-1);
			currentOffNmos[30] = AmperePerMeter(1.26e-1);
			currentOffNmos[40] = AmperePerMeter(1.41e-1);
			currentOffNmos[50] = AmperePerMeter(1.57e-1);
			currentOffNmos[60] = AmperePerMeter(1.75e-1);
			currentOffNmos[70] = AmperePerMeter(1.94e-1);
			currentOffNmos[80] = AmperePerMeter(2.15e-1);
			currentOffNmos[90] = AmperePerMeter(2.37e-1);
			currentOffNmos[100] = AmperePerMeter(2.60e-1);

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
            std::cout << "Unknown device roadmap!" << std::endl;
            exit(1);
		}
	} else if (_featureSizeInNano >= 22) {
		if (_deviceRoadmap == HP) {
			/* PTM model: 22nm.pm, from http://ptm.asu.edu/ */
			vdd = 0.85_V;
			vth = 208.9006e-3_V;
			vpp = vdd + vth;
			phyGateLength = 2.6e-8_m;
			capIdealGate = FaradPerMeter(3.287e-10);
			capFringe = FaradPerMeter(4.532e-10);
			capOx = FaradPerSquareMeter(3.83e-2);
			effectiveElectronMobility = Mobility(397.26e-4);
			effectiveHoleMobility = Mobility(83.60e-4);
			effectiveResistanceMultiplier = 1.54;	/* from CACTI */
			transconductanceNmos = SiemensPerMeter(1.08E+03);
			transconductancePmos = SiemensPerMeter(6.98E+02);
			currentOnNmos[0] = AmperePerMeter(1.50e3);
			currentOnNmos[10] = AmperePerMeter(1.47e3);
			currentOnNmos[20] = AmperePerMeter(1.43e3);
			currentOnNmos[30] = AmperePerMeter(1.39e3);
			currentOnNmos[40] = AmperePerMeter(1.35e3);
			currentOnNmos[50] = AmperePerMeter(1.31e3);
			currentOnNmos[60] = AmperePerMeter(1.28e3);
			currentOnNmos[70] = AmperePerMeter(1.24e3);
			currentOnNmos[80] = AmperePerMeter(1.20e3);
			currentOnNmos[90] = AmperePerMeter(1.17e3);
			currentOnNmos[100] = AmperePerMeter(1.13e3);
			currentOnPmos[0] = AmperePerMeter(1.32e3);
			currentOnPmos[10] = AmperePerMeter(1.25e3);
			currentOnPmos[20] = AmperePerMeter(1.19e3);
			currentOnPmos[30] = AmperePerMeter(1.13e3);
			currentOnPmos[40] = AmperePerMeter(1.07e3);
			currentOnPmos[50] = AmperePerMeter(1.02e3);
			currentOnPmos[60] = AmperePerMeter(0.97e3);
			currentOnPmos[70] = AmperePerMeter(0.92e3);
			currentOnPmos[80] = AmperePerMeter(0.88e3);
			currentOnPmos[90] = AmperePerMeter(0.84e3);
			currentOnPmos[100] = AmperePerMeter(0.80e3);
			currentOffNmos[0] = AmperePerMeter(100.20e-3);
			currentOffNmos[10] = AmperePerMeter(120.40e-3);
			currentOffNmos[20] = AmperePerMeter(143.50e-3);
			currentOffNmos[30] = AmperePerMeter(169.50e-3);
			currentOffNmos[40] = AmperePerMeter(198.70e-3);
			currentOffNmos[50] = AmperePerMeter(231.20e-3);
			currentOffNmos[60] = AmperePerMeter(267.00e-3);
			currentOffNmos[70] = AmperePerMeter(306.30e-3);
			currentOffNmos[80] = AmperePerMeter(349.30e-3);
			currentOffNmos[90] = AmperePerMeter(396.00e-3);
			currentOffNmos[100] = AmperePerMeter(446.60e-3);
			currentOffPmos[0] = AmperePerMeter(100.20e-3);
			currentOffPmos[10] = AmperePerMeter(119.40e-3);
			currentOffPmos[20] = AmperePerMeter(140.80e-3);
			currentOffPmos[30] = AmperePerMeter(164.60e-3);
			currentOffPmos[40] = AmperePerMeter(190.90e-3);
			currentOffPmos[50] = AmperePerMeter(219.50e-3);
			currentOffPmos[60] = AmperePerMeter(250.70e-3);
			currentOffPmos[70] = AmperePerMeter(284.50e-3);
			currentOffPmos[80] = AmperePerMeter(320.90e-3);
			currentOffPmos[90] = AmperePerMeter(359.80e-3);
			currentOffPmos[100] = AmperePerMeter(401.50e-3);
			pnSizeRatio = currentOnNmos[0]/currentOnPmos[0];
		} else if (_deviceRoadmap == LOP) {
			/* PTM model: 22nm_LP.pm, from http://ptm.asu.edu/ */
			vdd = 0.85_V;
			vth = 419.915e-3_V;
			vpp = vdd + vth;
			phyGateLength = 2.6e-8_m;
			capIdealGate = FaradPerMeter(5.245e-10);
			capFringe = FaradPerMeter(8.004e-10);
			capOx = FaradPerSquareMeter(2.65e-2);
			effectiveElectronMobility = Mobility(731.29e-4);
			effectiveHoleMobility = Mobility(111.22e-4);
			effectiveResistanceMultiplier = 1.77;	/* from CACTI */
			transconductanceNmos = SiemensPerMeter(4.56E+02);
			transconductancePmos = SiemensPerMeter(1.85E+02);
			currentOnNmos[0] = AmperePerMeter(791.90);
			currentOnNmos[10] = AmperePerMeter(756.40);
			currentOnNmos[20] = AmperePerMeter(722.20);
			currentOnNmos[30] = AmperePerMeter(689.40);
			currentOnNmos[40] = AmperePerMeter(658.10);
			currentOnNmos[50] = AmperePerMeter(628.30);
			currentOnNmos[60] = AmperePerMeter(600.00);
			currentOnNmos[70] = AmperePerMeter(573.30);
			currentOnNmos[80] = AmperePerMeter(548.00);
			currentOnNmos[90] = AmperePerMeter(524.20);
			currentOnNmos[100] = AmperePerMeter(501.70);
			currentOnPmos[0] = AmperePerMeter(600.20);
			currentOnPmos[10] = AmperePerMeter(561.30);
			currentOnPmos[20] = AmperePerMeter(525.50);
			currentOnPmos[30] = AmperePerMeter(492.50);
			currentOnPmos[40] = AmperePerMeter(462.20);
			currentOnPmos[50] = AmperePerMeter(434.30);
			currentOnPmos[60] = AmperePerMeter(408.70);
			currentOnPmos[70] = AmperePerMeter(385.10);
			currentOnPmos[80] = AmperePerMeter(363.40);
			currentOnPmos[90] = AmperePerMeter(343.30);
			currentOnPmos[100] = AmperePerMeter(324.80);
			currentOffNmos[0] = AmperePerMeter(100.00e-6);
			currentOffNmos[10] = AmperePerMeter(147.30e-6);
			currentOffNmos[20] = AmperePerMeter(212.10e-6);
			currentOffNmos[30] = AmperePerMeter(299.60e-6);
			currentOffNmos[40] = AmperePerMeter(415.30e-6);
			currentOffNmos[50] = AmperePerMeter(565.80e-6);
			currentOffNmos[60] = AmperePerMeter(758.90e-6);
			currentOffNmos[70] = AmperePerMeter(1003.00e-6);
			currentOffNmos[80] = AmperePerMeter(1307.00e-6);
			currentOffNmos[90] = AmperePerMeter(1682.00e-6);
			currentOffNmos[100] = AmperePerMeter(2139.00e-6);
			currentOffPmos[0] = AmperePerMeter(100.00e-6);
			currentOffPmos[10] = AmperePerMeter(147.30e-6);
			currentOffPmos[20] = AmperePerMeter(212.10e-6);
			currentOffPmos[30] = AmperePerMeter(299.60e-6);
			currentOffPmos[40] = AmperePerMeter(415.30e-6);
			currentOffPmos[50] = AmperePerMeter(565.80e-6);
			currentOffPmos[60] = AmperePerMeter(758.90e-6);
			currentOffPmos[70] = AmperePerMeter(1003.00e-6);
			currentOffPmos[80] = AmperePerMeter(1307.00e-6);
			currentOffPmos[90] = AmperePerMeter(1682.00e-6);
			currentOffPmos[100] = AmperePerMeter(2139.00e-6);
			pnSizeRatio = currentOnNmos[0]/currentOnPmos[0];
		} else if (_deviceRoadmap == CNT) {
			/* CNFET @ 20nm gate length */
			vdd = 0.7_V;
			vth = 0.18_V;
			vpp = vdd + vth;
			phyGateLength = 0.020e-6_m;
			capIdealGate = FaradPerMeter(1.18e-15);
			capFringe = FaradPerMeter(4.18e-17);
			capOx = FaradPerSquareMeter(5.90e-2);
			effectiveElectronMobility = Mobility(78700e-4);
			effectiveHoleMobility = Mobility(78700e-4);
			pnSizeRatio = 1.0;
			effectiveResistanceMultiplier = 2.0;

			currentOnNmos[0] = AmperePerMeter(3361.1);
			currentOnNmos[10] = AmperePerMeter(3300.0);
			currentOnNmos[20] = AmperePerMeter(3240.0);
			currentOnNmos[30] = AmperePerMeter(3180.0);
			currentOnNmos[40] = AmperePerMeter(3120.0);
			currentOnNmos[50] = AmperePerMeter(3060.0);
			currentOnNmos[60] = AmperePerMeter(3000.0);
			currentOnNmos[70] = AmperePerMeter(2940.0);
			currentOnNmos[80] = AmperePerMeter(2880.0);
			currentOnNmos[90] = AmperePerMeter(2820.0);
			currentOnNmos[100] = AmperePerMeter(2760.0);

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

			currentOffNmos[0] = AmperePerMeter(8.94e-2);
			currentOffNmos[10] = AmperePerMeter(1.00e-1);
			currentOffNmos[20] = AmperePerMeter(1.12e-1);
			currentOffNmos[30] = AmperePerMeter(1.26e-1);
			currentOffNmos[40] = AmperePerMeter(1.41e-1);
			currentOffNmos[50] = AmperePerMeter(1.57e-1);
			currentOffNmos[60] = AmperePerMeter(1.75e-1);
			currentOffNmos[70] = AmperePerMeter(1.94e-1);
			currentOffNmos[80] = AmperePerMeter(2.15e-1);
			currentOffNmos[90] = AmperePerMeter(2.37e-1);
			currentOffNmos[100] = AmperePerMeter(2.60e-1);

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
            std::cout << "Unknown device roadmap!" << std::endl;
            exit(1);
        }
	} else if (featureSizeInNano >= 14) {
		if (deviceRoadmap != LOP) {

			std::cout<<"Only LOP for 14 nm and beyond is supported"<< std::endl;
			exit(-1);

		} else if (_deviceRoadmap == LOP){
			// device specifications follow IRDS 2016
			vdd = 0.8_V;
			vth = Volt(thresholdVoltages[0]);
			vpp = vdd + vth;
			heightFin = 4.2e-8_m;
			widthFin = 8.0e-9_m;
			finPitch = 4.8e-8_m;

			maximumFinCount =4;
			effectiveWidth=widthFin+heightFin*2;

			phyGateLength = 2.6e-8_m; // Changed to 2.6e-8 following IRDS 2016
			capIdealGate = Farad(caplist[0] * 1E-18) / effectiveWidth;
			capDrainTotal = Farad(drainTotalCapacitances[0]) / effectiveWidth;
			effectiveResistanceMultiplier = effectiveResistanceMultipliers[0];	// From CACTI
			transconductanceNmos = SiemensPerMeter(gm[0]);
			transconductancePmos = SiemensPerMeter(gm[0]);	
			onCurrentTransconductance = SiemensPerMeter(gm[0]);  // Transconductance gm at on current


			currentOnNmos[0] = AmperePerMeter(currentlist[0]);
			currentOnNmos[10] = AmperePerMeter(853);
			currentOnNmos[20] = AmperePerMeter(814);
			currentOnNmos[30] = AmperePerMeter(777);
			currentOnNmos[40] = AmperePerMeter(742);
			currentOnNmos[50] = AmperePerMeter(708);
			currentOnNmos[60] = AmperePerMeter(677);
			currentOnNmos[70] = AmperePerMeter(646);
			currentOnNmos[80] = AmperePerMeter(618);
			currentOnNmos[90] = AmperePerMeter(591);
			currentOnNmos[100] = AmperePerMeter(565);
			currentOnPmos[0]  = currentOnNmos[0];
			currentOnPmos[10] = AmperePerMeter(767);
			currentOnPmos[20] = AmperePerMeter(718);
			currentOnPmos[30] = AmperePerMeter(672);
			currentOnPmos[40] = AmperePerMeter(631);
			currentOnPmos[50] = AmperePerMeter(593);
			currentOnPmos[60] = AmperePerMeter(558);
			currentOnPmos[70] = AmperePerMeter(526);
			currentOnPmos[80] = AmperePerMeter(496);
			currentOnPmos[90] = AmperePerMeter(469);
			currentOnPmos[100] = AmperePerMeter(443);
			currentOffNmos[0] = AmperePerMeter(0.0001);
			currentOffNmos[10] = AmperePerMeter(184.4553e-6);
			currentOffNmos[20] = AmperePerMeter(328.7707e-6);
			currentOffNmos[30] = AmperePerMeter(566.8658e-6);
			currentOffNmos[40] = AmperePerMeter(948.1816e-6);
			currentOffNmos[50] = AmperePerMeter(1.5425e-3);
			currentOffNmos[60] = AmperePerMeter(2.4460e-3);
			currentOffNmos[70] = AmperePerMeter(3.7885e-3);
			currentOffNmos[80] = AmperePerMeter(5.7416e-3);
			currentOffNmos[90] = AmperePerMeter(8.5281e-3);
			currentOffNmos[100] = AmperePerMeter(1.24327e-2);;
			currentOffPmos[0] = AmperePerMeter(102.3333e-6);
			currentOffPmos[10] = AmperePerMeter(203.4774e-6);
			currentOffPmos[20] = AmperePerMeter(389.0187e-6);
			currentOffPmos[30] = AmperePerMeter(717.5912e-6);
			currentOffPmos[40] = AmperePerMeter(1.2810e-3);
			currentOffPmos[50] = AmperePerMeter(2.2192e-3);
			currentOffPmos[60] = AmperePerMeter(3.7395e-3);
			currentOffPmos[70] = AmperePerMeter(6.1428e-3);
			currentOffPmos[80] = AmperePerMeter(9.8554e-3);
			currentOffPmos[90] = AmperePerMeter(1.54702e-2);
			currentOffPmos[100] = AmperePerMeter(2.37959e-2);
			pnSizeRatio = (int)(currentOnNmos[0]/currentOnPmos[0]);
		}
	} else if (featureSizeInNano >= 10) {
		if (deviceRoadmap != LOP) {

			std::cout<<"Only LOP for 14 nm and beyond is supported"<< std::endl;
			exit(-1);

		} else if (_deviceRoadmap == LOP) {

			// device specifications follow IRDS 2017
			vdd = 0.75_V;
			vth = Volt(thresholdVoltages[1]);
			vpp = vdd + vth;
			heightFin = 4.5e-8_m;
			widthFin = 8.0e-9_m;
			finPitch = 3.6e-8_m;

			maximumFinCount =3;
			effectiveWidth=widthFin+heightFin*2;

			phyGateLength = 2.2e-8_m;
			capIdealGate = Farad(caplist[1] * 1E-18) / effectiveWidth;
			capDrainTotal = Farad(drainTotalCapacitances[1]) / effectiveWidth;
			effectiveResistanceMultiplier = effectiveResistanceMultipliers[1];	// From CACTI
			transconductanceNmos = SiemensPerMeter(gm[1]);
			transconductancePmos = SiemensPerMeter(gm[1]);			
			onCurrentTransconductance = SiemensPerMeter(gm[1]);  // Transconductance gm at on current


			currentOnNmos[0] = AmperePerMeter(currentlist[1]);
			currentOnNmos[10] = AmperePerMeter(824);
			currentOnNmos[20] = AmperePerMeter(787);
			currentOnNmos[30] = AmperePerMeter(751);
			currentOnNmos[40] = AmperePerMeter(717);
			currentOnNmos[50] = AmperePerMeter(684);
			currentOnNmos[60] = AmperePerMeter(654);
			currentOnNmos[70] = AmperePerMeter(624);
			currentOnNmos[80] = AmperePerMeter(597);
			currentOnNmos[90] = AmperePerMeter(571);
			currentOnNmos[100] = AmperePerMeter(546);
			currentOnPmos[0]  = currentOnNmos[0];  
			currentOnPmos[10] = AmperePerMeter(725);
			currentOnPmos[20] = AmperePerMeter(678);
			currentOnPmos[30] = AmperePerMeter(636);
			currentOnPmos[40] = AmperePerMeter(597);
			currentOnPmos[50] = AmperePerMeter(561);
			currentOnPmos[60] = AmperePerMeter(527);
			currentOnPmos[70] = AmperePerMeter(497);
			currentOnPmos[80] = AmperePerMeter(469);
			currentOnPmos[90] = AmperePerMeter(443);
			currentOnPmos[100] = AmperePerMeter(419);
			currentOffNmos[0] = AmperePerMeter(0.000127);
			currentOffNmos[10] = AmperePerMeter(184.4892e-6);
			currentOffNmos[20] = AmperePerMeter(329.1615e-6);
			currentOffNmos[30] = AmperePerMeter(568.0731e-6);
			currentOffNmos[40] = AmperePerMeter(951.0401e-6);
			currentOffNmos[50] = AmperePerMeter(1.5484e-3);
			currentOffNmos[60] = AmperePerMeter(2.4574e-3);
			currentOffNmos[70] = AmperePerMeter(3.8090e-3);
			currentOffNmos[80] = AmperePerMeter(5.7767e-3);
			currentOffNmos[90] = AmperePerMeter(8.5862e-3);
			currentOffNmos[100] = AmperePerMeter(1.2525e-2);
			currentOffPmos[0] = AmperePerMeter(100.5839e-6);
			currentOffPmos[10] = AmperePerMeter(200.2609e-6);
			currentOffPmos[20] = AmperePerMeter(383.3239e-6);
			currentOffPmos[30] = AmperePerMeter(707.8499e-6);
			currentOffPmos[40] = AmperePerMeter(1.2649e-3);
			currentOffPmos[50] = AmperePerMeter(2.1932e-3);
			currentOffPmos[60] = AmperePerMeter(3.6987e-3);
			currentOffPmos[70] = AmperePerMeter(6.0804e-3);
			currentOffPmos[80] = AmperePerMeter(9.7622e-3);
			currentOffPmos[90] = AmperePerMeter(1.53340e-2);
			currentOffPmos[100] = AmperePerMeter(2.36007e-2);
			pnSizeRatio = (int)(currentOnNmos[0]/currentOnPmos[0]);
		}
	} else if (featureSizeInNano >= 7) {
		if (deviceRoadmap != LOP) {

			std::cout<<"Only LOP for 14 nm and beyond is supported"<< std::endl;
			exit(-1);

		} else if (_deviceRoadmap == LOP){

			// based on IRDS 2017
			vdd = 0.7_V;
			vth = Volt(thresholdVoltages[2]);
			vpp = vdd + vth;
			heightFin = 5.0e-8_m;
			widthFin = 7e-9_m;
			finPitch = 3.0e-8_m;

			maximumFinCount = 2;
			effectiveWidth = 107e-9_m;

			phyGateLength = 2.2e-8_m;
			capIdealGate = Farad(caplist[2] * 1E-18) / effectiveWidth;//8.49489e-10;
			capDrainTotal = Farad(drainTotalCapacitances[2]) / effectiveWidth;
			effectiveResistanceMultiplier = effectiveResistanceMultipliers[2];	// From CACTI
			transconductanceNmos = SiemensPerMeter(gm[2]);
			transconductancePmos = SiemensPerMeter(gm[2]);
			onCurrentTransconductance = SiemensPerMeter(gm[2]);  // Transconductance gm at on current


			currentOnNmos[0] = AmperePerMeter(currentlist[2]);
			currentOnNmos[10] = AmperePerMeter(786);
			currentOnNmos[20] = AmperePerMeter(750);
			currentOnNmos[30] = AmperePerMeter(716);
			currentOnNmos[40] = AmperePerMeter(684);
			currentOnNmos[50] = AmperePerMeter(653);
			currentOnNmos[60] = AmperePerMeter(624);
			currentOnNmos[70] = AmperePerMeter(595);
			currentOnNmos[80] = AmperePerMeter(569);
			currentOnNmos[90] = AmperePerMeter(545);
			currentOnNmos[100] = AmperePerMeter(521);
			currentOnPmos[0]  = currentOnNmos[0];  
			currentOnPmos[10] = AmperePerMeter(689);
			currentOnPmos[20] = AmperePerMeter(645);
			currentOnPmos[30] = AmperePerMeter(605);
			currentOnPmos[40] = AmperePerMeter(567);
			currentOnPmos[50] = AmperePerMeter(533);
			currentOnPmos[60] = AmperePerMeter(501);
			currentOnPmos[70] = AmperePerMeter(473);
			currentOnPmos[80] = AmperePerMeter(446);
			currentOnPmos[90] = AmperePerMeter(421);
			currentOnPmos[100] = AmperePerMeter(398);
			currentOffNmos[0] = AmperePerMeter(0.000147);
			currentOffNmos[10] = AmperePerMeter(1.85E-04);
			currentOffNmos[20] = AmperePerMeter(3.32E-04);
			currentOffNmos[30] = AmperePerMeter(5.74E-04);
			currentOffNmos[40] = AmperePerMeter(9.62E-04);
			currentOffNmos[50] = AmperePerMeter(1.5695e-3);
			currentOffNmos[60] = AmperePerMeter(2.4953e-3);
			currentOffNmos[70] = AmperePerMeter(3.8744e-3);
			currentOffNmos[80] = AmperePerMeter(5.8858e-3);
			currentOffNmos[90] = AmperePerMeter(8.7624e-3);
			currentOffNmos[100] = AmperePerMeter(1.28025e-2);
			currentOffPmos[0] = AmperePerMeter(100.9536e-6);
			currentOffPmos[10] = AmperePerMeter(201.3937e-6);
			currentOffPmos[20] = AmperePerMeter(386.2086e-6);
			currentOffPmos[30] = AmperePerMeter(714.4288e-6);
			currentOffPmos[40] = AmperePerMeter(1.2788e-3);
			currentOffPmos[50] = AmperePerMeter(2.2207e-3);
			currentOffPmos[60] = AmperePerMeter(3.7509e-3);
			currentOffPmos[70] = AmperePerMeter(6.1750e-3);
			currentOffPmos[80] = AmperePerMeter(9.9278e-3);
			currentOffPmos[90] = AmperePerMeter(1.56146e-2);
			currentOffPmos[100] = AmperePerMeter(2.40633e-2);
			pnSizeRatio = (int)(currentOnNmos[0]/currentOnPmos[0]);
		}
	} 

	/* Technology update beyond 7 nm */ 
	else if (featureSizeInNano >= 5) {
		if (deviceRoadmap != LOP) {

			std::cout<<"Only LOP for 14 nm and beyond is supported"<< std::endl;
			exit(-1);

		} else if (_deviceRoadmap == LOP){

			// IRDS 2021
			vdd = 0.7_V;
			vth = Volt(thresholdVoltages[3]);
			vpp = vdd + vth;

			widthFin = 6.0e-9_m;
			finPitch = 28.0e-9_m;
			phyGateLength = 2.0e-8_m;

			// height is not needed as long as effective width is specified
			effectiveWidth = 106.0_nm;
			maximumFinCount =2;		

			capIdealGate = Farad(caplist[3] * 1E-18) / effectiveWidth;
			capDrainTotal = Farad(drainTotalCapacitances[3]) / effectiveWidth;

			effectiveResistanceMultiplier = effectiveResistanceMultipliers[3];	// From CACTI
			transconductanceNmos = SiemensPerMeter(gm[3]);
			transconductancePmos = SiemensPerMeter(gm[3]);
			onCurrentTransconductance = SiemensPerMeter(gm[3]);  // Transconductance gm at on current


			currentOnNmos[0] = AmperePerMeter(currentlist[3]);
			currentOnNmos[10] = AmperePerMeter(786);
			currentOnNmos[20] = AmperePerMeter(750);
			currentOnNmos[30] = AmperePerMeter(716);
			currentOnNmos[40] = AmperePerMeter(684);
			currentOnNmos[50] = AmperePerMeter(653);
			currentOnNmos[60] = AmperePerMeter(624);
			currentOnNmos[70] = AmperePerMeter(595);
			currentOnNmos[80] = AmperePerMeter(569);
			currentOnNmos[90] = AmperePerMeter(545);
			currentOnNmos[100] = AmperePerMeter(521);
			currentOnPmos[0]  = currentOnNmos[0]; 
			currentOnPmos[10] = AmperePerMeter(689);
			currentOnPmos[20] = AmperePerMeter(645);
			currentOnPmos[30] = AmperePerMeter(605);
			currentOnPmos[40] = AmperePerMeter(567);
			currentOnPmos[50] = AmperePerMeter(533);
			currentOnPmos[60] = AmperePerMeter(501);
			currentOnPmos[70] = AmperePerMeter(473);
			currentOnPmos[80] = AmperePerMeter(446);
			currentOnPmos[90] = AmperePerMeter(421);
			currentOnPmos[100] = AmperePerMeter(398);
			currentOffNmos[0] = AmperePerMeter(0.000138);
			currentOffNmos[10] = AmperePerMeter(1.85E-04);
			currentOffNmos[20] = AmperePerMeter(3.32E-04);
			currentOffNmos[30] = AmperePerMeter(5.74E-04);
			currentOffNmos[40] = AmperePerMeter(9.62E-04);
			currentOffNmos[50] = AmperePerMeter(1.5695e-3);
			currentOffNmos[60] = AmperePerMeter(2.4953e-3);
			currentOffNmos[70] = AmperePerMeter(3.8744e-3);
			currentOffNmos[80] = AmperePerMeter(5.8858e-3);
			currentOffNmos[90] = AmperePerMeter(8.7624e-3);
			currentOffNmos[100] = AmperePerMeter(1.28025e-2);
			currentOffPmos[0] = AmperePerMeter(100.9536e-6);
			currentOffPmos[10] = AmperePerMeter(201.3937e-6);
			currentOffPmos[20] = AmperePerMeter(386.2086e-6);
			currentOffPmos[30] = AmperePerMeter(714.4288e-6);
			currentOffPmos[40] = AmperePerMeter(1.2788e-3);
			currentOffPmos[50] = AmperePerMeter(2.2207e-3);
			currentOffPmos[60] = AmperePerMeter(3.7509e-3);
			currentOffPmos[70] = AmperePerMeter(6.1750e-3);
			currentOffPmos[80] = AmperePerMeter(9.9278e-3);
			currentOffPmos[90] = AmperePerMeter(1.56146e-2);
			currentOffPmos[100] = AmperePerMeter(2.40633e-2);
			pnSizeRatio = (int)(currentOnNmos[0]/currentOnPmos[0]);
		}
	} else if (featureSizeInNano >= 3) {
		if (deviceRoadmap != LOP) {

			std::cout<<"Only LOP for 14 nm and beyond is supported"<< std::endl;
			exit(-1);

		} else if (_deviceRoadmap == LOP){

			// IRDS 2022
			vdd = 0.7_V;
			vth = Volt(thresholdVoltages[4]);
			vpp = vdd + vth;
			widthFin = 5.0e-9_m;
			finPitch = 24.0e-9_m;
			phyGateLength = 1.8e-8_m;

			// height is not needed as long as effective width is specified
			effectiveWidth = 101.0_nm;
			maximumFinCount =2;	

			capIdealGate = Farad(caplist[4] * 1E-18) / effectiveWidth;   //6.44E-10; //8.91E-10;
			capDrainTotal = Farad(drainTotalCapacitances[4]) / effectiveWidth;

			effectiveResistanceMultiplier = effectiveResistanceMultipliers[4];	// From CACTI
			transconductanceNmos = SiemensPerMeter(gm[4]);
			transconductancePmos = SiemensPerMeter(gm[4]);	
			onCurrentTransconductance = SiemensPerMeter(gm[4]);  // Transconductance gm at on current

			currentOnNmos[0] = AmperePerMeter(currentlist[4]);
			currentOnNmos[10] = AmperePerMeter(786);
			currentOnNmos[20] = AmperePerMeter(750);
			currentOnNmos[30] = AmperePerMeter(716);
			currentOnNmos[40] = AmperePerMeter(684);
			currentOnNmos[50] = AmperePerMeter(653);
			currentOnNmos[60] = AmperePerMeter(624);
			currentOnNmos[70] = AmperePerMeter(595);
			currentOnNmos[80] = AmperePerMeter(569);
			currentOnNmos[90] = AmperePerMeter(545);
			currentOnNmos[100] = AmperePerMeter(521);
			currentOnPmos[0]  = currentOnNmos[0]; 
			currentOnPmos[10] = AmperePerMeter(689);
			currentOnPmos[20] = AmperePerMeter(645);
			currentOnPmos[30] = AmperePerMeter(605);
			currentOnPmos[40] = AmperePerMeter(567);
			currentOnPmos[50] = AmperePerMeter(533);
			currentOnPmos[60] = AmperePerMeter(501);
			currentOnPmos[70] = AmperePerMeter(473);
			currentOnPmos[80] = AmperePerMeter(446);
			currentOnPmos[90] = AmperePerMeter(421);
			currentOnPmos[100] = AmperePerMeter(398);
			currentOffNmos[0] = AmperePerMeter(0.000158);
			currentOffNmos[10] = AmperePerMeter(1.85E-04);
			currentOffNmos[20] = AmperePerMeter(3.32E-04);
			currentOffNmos[30] = AmperePerMeter(5.74E-04);
			currentOffNmos[40] = AmperePerMeter(9.62E-04);
			currentOffNmos[50] = AmperePerMeter(1.5695e-3);
			currentOffNmos[60] = AmperePerMeter(2.4953e-3);
			currentOffNmos[70] = AmperePerMeter(3.8744e-3);
			currentOffNmos[80] = AmperePerMeter(5.8858e-3);
			currentOffNmos[90] = AmperePerMeter(8.7624e-3);
			currentOffNmos[100] = AmperePerMeter(1.28025e-2);
			currentOffPmos[0] = AmperePerMeter(100.9536e-6);
			currentOffPmos[10] = AmperePerMeter(201.3937e-6);
			currentOffPmos[20] = AmperePerMeter(386.2086e-6);
			currentOffPmos[30] = AmperePerMeter(714.4288e-6);
			currentOffPmos[40] = AmperePerMeter(1.2788e-3);
			currentOffPmos[50] = AmperePerMeter(2.2207e-3);
			currentOffPmos[60] = AmperePerMeter(3.7509e-3);
			currentOffPmos[70] = AmperePerMeter(6.1750e-3);
			currentOffPmos[80] = AmperePerMeter(9.9278e-3);
			currentOffPmos[90] = AmperePerMeter(1.56146e-2);
			currentOffPmos[100] = AmperePerMeter(2.40633e-2);
			pnSizeRatio = (int)(currentOnNmos[0]/currentOnPmos[0]);
		}
	} 
	else if (featureSizeInNano >= 2) {
		if (deviceRoadmap != LOP) {

			std::cout<<"Only LOP for 14 nm and beyond is supported"<< std::endl;
			exit(-1);

		} else if (_deviceRoadmap == LOP){ // 2 nm

			// IRDS 2022
			vdd = 0.65_V;
			vth = Volt(thresholdVoltages[5]);
			vpp = vdd + vth;
			finPitch = 26e-9_m;
			phyGateLength = 1.4e-8_m;

			// GAA-specific parameters
			maximumFinsPerGAA=1;
			maximumSheetCount=3;
			sheetThickness = 6_nm;
			sheetWidth = 15_nm;

			widthFin=sheetWidth; // For drain height calculation 	
			effectiveWidth=(sheetThickness+sheetWidth)*2;

			capIdealGate = Farad(caplist[5] * 1E-18) / (effectiveWidth * maximumSheetCount); 
			capDrainTotal = Farad(drainTotalCapacitances[5]) / effectiveWidth;

			effectiveResistanceMultiplier = effectiveResistanceMultipliers[5];	// From CACTI
			transconductanceNmos = SiemensPerMeter(gm[5]);
			transconductancePmos = SiemensPerMeter(gm[5]);	
			onCurrentTransconductance = SiemensPerMeter(gm[5]);  // Transconductance gm at on current


			currentOnNmos[0] = AmperePerMeter(currentlist[5]);
			currentOnNmos[10] = AmperePerMeter(786);
			currentOnNmos[20] = AmperePerMeter(750);
			currentOnNmos[30] = AmperePerMeter(716);
			currentOnNmos[40] = AmperePerMeter(684);
			currentOnNmos[50] = AmperePerMeter(653);
			currentOnNmos[60] = AmperePerMeter(624);
			currentOnNmos[70] = AmperePerMeter(595);
			currentOnNmos[80] = AmperePerMeter(569);
			currentOnNmos[90] = AmperePerMeter(545);
			currentOnNmos[100] = AmperePerMeter(521);
			currentOnPmos[0]  = currentOnNmos[0]; 
			currentOnPmos[10] = AmperePerMeter(689);
			currentOnPmos[20] = AmperePerMeter(645);
			currentOnPmos[30] = AmperePerMeter(605);
			currentOnPmos[40] = AmperePerMeter(567);
			currentOnPmos[50] = AmperePerMeter(533);
			currentOnPmos[60] = AmperePerMeter(501);
			currentOnPmos[70] = AmperePerMeter(473);
			currentOnPmos[80] = AmperePerMeter(446);
			currentOnPmos[90] = AmperePerMeter(421);
			currentOnPmos[100] = AmperePerMeter(398);
			currentOffNmos[0] = AmperePerMeter(0.0000733);
			currentOffNmos[10] = AmperePerMeter(1.85E-04);
			currentOffNmos[20] = AmperePerMeter(3.32E-04);
			currentOffNmos[30] = AmperePerMeter(5.74E-04);
			currentOffNmos[40] = AmperePerMeter(9.62E-04);
			currentOffNmos[50] = AmperePerMeter(1.5695e-3);
			currentOffNmos[60] = AmperePerMeter(2.4953e-3);
			currentOffNmos[70] = AmperePerMeter(3.8744e-3);
			currentOffNmos[80] = AmperePerMeter(5.8858e-3);
			currentOffNmos[90] = AmperePerMeter(8.7624e-3);
			currentOffNmos[100] = AmperePerMeter(1.28025e-2);
			currentOffPmos[0] = AmperePerMeter(100.9536e-6);
			currentOffPmos[10] = AmperePerMeter(201.3937e-6);
			currentOffPmos[20] = AmperePerMeter(386.2086e-6);
			currentOffPmos[30] = AmperePerMeter(714.4288e-6);
			currentOffPmos[40] = AmperePerMeter(1.2788e-3);
			currentOffPmos[50] = AmperePerMeter(2.2207e-3);
			currentOffPmos[60] = AmperePerMeter(3.7509e-3);
			currentOffPmos[70] = AmperePerMeter(6.1750e-3);
			currentOffPmos[80] = AmperePerMeter(9.9278e-3);
			currentOffPmos[90] = AmperePerMeter(1.56146e-2);
			currentOffPmos[100] = AmperePerMeter(2.40633e-2);
			pnSizeRatio = (int)(currentOnNmos[0]/currentOnPmos[0]);
		}
	} 		
	else if (featureSizeInNano >= 1) {
		if (deviceRoadmap != LOP) {

			std::cout<<"Only LOP for 14 nm and beyond is supported"<< std::endl;
			exit(-1);

		} else if (_deviceRoadmap == LOP) {

			// IRDS 2022
			vdd = 0.6_V;
			vth = Volt(thresholdVoltages[6]);
			vpp = vdd + vth;
			finPitch = 24e-9_m;
			phyGateLength = 1.2e-8_m;

			// IRDS 2022 - GAA specfic parameters
			maximumFinsPerGAA=1;
			maximumSheetCount=4;
			sheetThickness = 6_nm;
			sheetWidth = 10_nm;

			widthFin= sheetWidth; // For drain height calculation 
			effectiveWidth=(sheetThickness+sheetWidth)*2;

			capIdealGate = Farad(caplist[6] * 1E-18) / (effectiveWidth * maximumSheetCount);
			capDrainTotal = Farad(drainTotalCapacitances[6]) / effectiveWidth;

			effectiveResistanceMultiplier = effectiveResistanceMultipliers[6];	// From CACTI
			transconductanceNmos = SiemensPerMeter(gm[6]);
			transconductancePmos = SiemensPerMeter(gm[6]);	
			onCurrentTransconductance = SiemensPerMeter(gm[6]);  // Transconductance gm at on current	

			currentOnNmos[0] = AmperePerMeter(currentlist[6]);
			currentOnNmos[10] = AmperePerMeter(786);
			currentOnNmos[20] = AmperePerMeter(750);
			currentOnNmos[30] = AmperePerMeter(716);
			currentOnNmos[40] = AmperePerMeter(684);
			currentOnNmos[50] = AmperePerMeter(653);
			currentOnNmos[60] = AmperePerMeter(624);
			currentOnNmos[70] = AmperePerMeter(595);
			currentOnNmos[80] = AmperePerMeter(569);
			currentOnNmos[90] = AmperePerMeter(545);
			currentOnNmos[100] = AmperePerMeter(521);
			currentOnPmos[0]  = currentOnNmos[0]; 
			currentOnPmos[10] = AmperePerMeter(689);
			currentOnPmos[20] = AmperePerMeter(645);
			currentOnPmos[30] = AmperePerMeter(605);
			currentOnPmos[40] = AmperePerMeter(567);
			currentOnPmos[50] = AmperePerMeter(533);
			currentOnPmos[60] = AmperePerMeter(501);
			currentOnPmos[70] = AmperePerMeter(473);
			currentOnPmos[80] = AmperePerMeter(446);
			currentOnPmos[90] = AmperePerMeter(421);
			currentOnPmos[100] = AmperePerMeter(398);
			currentOffNmos[0] = AmperePerMeter(0.000169);
			currentOffNmos[10] = AmperePerMeter(1.85E-04);
			currentOffNmos[20] = AmperePerMeter(3.32E-04);
			currentOffNmos[30] = AmperePerMeter(5.74E-04);
			currentOffNmos[40] = AmperePerMeter(9.62E-04);
			currentOffNmos[50] = AmperePerMeter(1.5695e-3);
			currentOffNmos[60] = AmperePerMeter(2.4953e-3);
			currentOffNmos[70] = AmperePerMeter(3.8744e-3);
			currentOffNmos[80] = AmperePerMeter(5.8858e-3);
			currentOffNmos[90] = AmperePerMeter(8.7624e-3);
			currentOffNmos[100] = AmperePerMeter(1.28025e-2);
			currentOffPmos[0] = AmperePerMeter(100.9536e-6);
			currentOffPmos[10] = AmperePerMeter(201.3937e-6);
			currentOffPmos[20] = AmperePerMeter(386.2086e-6);
			currentOffPmos[30] = AmperePerMeter(714.4288e-6);
			currentOffPmos[40] = AmperePerMeter(1.2788e-3);
			currentOffPmos[50] = AmperePerMeter(2.2207e-3);
			currentOffPmos[60] = AmperePerMeter(3.7509e-3);
			currentOffPmos[70] = AmperePerMeter(6.1750e-3);
			currentOffPmos[80] = AmperePerMeter(9.9278e-3);
			currentOffPmos[90] = AmperePerMeter(1.56146e-2);
			currentOffPmos[100] = AmperePerMeter(2.40633e-2);
			pnSizeRatio = (int)(currentOnNmos[0]/currentOnPmos[0]);
		}
	}
	else 
	{
            std::cout << "Unknown device roadmap!" << std::endl;
            exit(1);
    }
        // If FeFET, we scale ideal gate capacitance by standard factor across all tech nodes
        if (isFeFET == true){
	    	//cout << "Modify capIdealGate for FeFET:" << std::endl;
            capIdealGateFeFET = capIdealGate * 1.73;
        }

	capOverlap = capIdealGate * 0.2;
	//capSidewall = 2.5e-10;	/* Unit: F/m, this value is from CACTI, PTM model shows the value is 5e-10 */
	const FaradPerSquareMeter cjd(1e-3);	// Bottom junction capacitance
	const FaradPerMeter cjswd(2.5e-10);	// Isolation-edge sidewall junction capacitance
	const FaradPerMeter cjswgd(0.5e-10);	// Gate-edge sidewall junction capacitance
	double mjd = 0.5;             /* Bottom junction capacitance grating coefficient */
	double mjswd = 0.33;           /* Isolation-edge sidewall junction capacitance grading coefficient */
	double mjswgd = 0.33;          /* Gate-edge sidewall junction capacitance grading coefficient */
	buildInPotential = 0.9_V;			// This value is from BSIM4
	capJunction = cjd / pow(1 + vdd / buildInPotential, mjd);
	capSidewall = cjswd / pow(1 + vdd / buildInPotential, mjswd);
	capDrainToChannel = cjswgd / pow(1 + vdd / buildInPotential, mjswgd);

	// junction capacitance for 14 nm and beyond; 

	if (featureSizeInNano == 14 ) capJunction = FaradPerSquareMeter(0.0120);
	else if (featureSizeInNano == 10 ) capJunction = FaradPerSquareMeter(0.0134);
	else if (featureSizeInNano== 7 ) capJunction = FaradPerSquareMeter(0.0137);
	else if (featureSizeInNano == 5 ) capJunction = FaradPerSquareMeter(0.0119);
	else if (featureSizeInNano == 3 ) capJunction = FaradPerSquareMeter(0.0128);
	else if (featureSizeInNano == 2 ) capJunction = FaradPerSquareMeter(0.0091);
	else if (featureSizeInNano == 1 ) capJunction = FaradPerSquareMeter(0.0102);
	else capJunction = cjd / pow(1 + vdd / buildInPotential, mjd);

	const Unit<MeterAtom<>, SecondAtom<-1>> saturationVelocity(1e5);	// Silicon saturation velocity
	vdsatNmos = phyGateLength * saturationVelocity / effectiveElectronMobility;
	vdsatPmos = phyGateLength * saturationVelocity / effectiveHoleMobility;

	/* Properties not used so far */
	// TODO: capPolywire stays zero, we need to find the values

	/* Interpolate */
	for (int i = 1; i < 100; i++) {
		if (i % 10) {
			AmperePerMeter a = currentOnNmos[i / 10 * 10];
			AmperePerMeter b = currentOnNmos[i / 10 * 10 + 10];
			currentOnNmos[i] = a + (b-a) * (i % 10) / 10;
		}
	}

	for (int i = 1; i < 100; i++) {
		if (i % 10) {
			AmperePerMeter a = currentOnPmos[i / 10 * 10];
			AmperePerMeter b = currentOnPmos[i / 10 * 10 + 10];
			currentOnPmos[i] = a + (b-a) * (i % 10) / 10;
		}
	}

	for (int i = 1; i < 100; i++) {
		if (i % 10) {
			AmperePerMeter a = currentOffNmos[i / 10 * 10];
			AmperePerMeter b = currentOffNmos[i / 10 * 10 + 10];
			currentOffNmos[i] = a + (b-a) * (i % 10) / 10;
		}
	}

	for (int i = 1; i < 100; i++) {
		if (i % 10) {
			AmperePerMeter a = currentOffPmos[i / 10 * 10];
			AmperePerMeter b = currentOffPmos[i / 10 * 10 + 10];
			currentOffPmos[i] = a + (b-a) * (i % 10) / 10;
		}
	}

	initialized = true;
}
void Technology::PrintProperty() {
	std::cout << "Fabrication Process Technology Node:" << std::endl;
	std::cout << "TODO" << std::endl;
}

void Technology::InterpolateWith(Technology rhs, double _alpha) {
	using namespace mse::unit;

	if (featureSizeInNano != rhs.featureSizeInNano) {
		vdd = (1 - _alpha) * vdd + _alpha * rhs.vdd;
		vth = (1 - _alpha) * vth + _alpha * rhs.vth;
		phyGateLength = (1 - _alpha) * phyGateLength + _alpha * rhs.phyGateLength;
		capIdealGate = (1 - _alpha) * capIdealGate + _alpha * rhs.capIdealGate;
                if (isFeFET){
		    capIdealGateFeFET = (1 - _alpha) * capIdealGateFeFET + _alpha * rhs.capIdealGateFeFET;
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
			currentOffNmos[i] = AmperePerMeter(pow(currentOffNmos[i].value(), 1 - _alpha) * pow(rhs.currentOffNmos[i].value(), _alpha));
			currentOffPmos[i] = AmperePerMeter(pow(currentOffPmos[i].value(), 1 - _alpha) * pow(rhs.currentOffPmos[i].value(), _alpha));
		}
		//capSidewall = 2.5e-10;	/* Unit: F/m, this value is from CACTI, PTM model shows the value is 5e-10 */
		const FaradPerSquareMeter cjd(1e-3);	// Bottom junction capacitance
		const FaradPerMeter cjswd(2.5e-10);	// Isolation-edge sidewall junction capacitance
		const FaradPerMeter cjswgd(0.5e-10);	// Gate-edge sidewall junction capacitance
		double mjd = 0.5;             /* Bottom junction capacitance grating coefficient */
		double mjswd = 0.33;           /* Isolation-edge sidewall junction capacitance grading coefficient */
		double mjswgd = 0.33;          /* Gate-edge sidewall junction capacitance grading coefficient */
		buildInPotential = 0.9_V;			// This value is from BSIM4
		capJunction = cjd / pow(1 + vdd / buildInPotential, mjd);
		capSidewall = cjswd / pow(1 + vdd / buildInPotential, mjswd);
		capDrainToChannel = cjswgd / pow(1 + vdd / buildInPotential, mjswgd);

		const Unit<MeterAtom<>, SecondAtom<-1>> saturationVelocity(1e5);	// Silicon saturation velocity
		vdsatNmos = phyGateLength * saturationVelocity / effectiveElectronMobility;
		vdsatPmos = phyGateLength * saturationVelocity / effectiveHoleMobility;
	}
}
