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


#ifndef MSE_MEMCELL_H
#define MSE_MEMCELL_H

#include "constant.hpp"
#include "typedef.hpp"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class MemCell {
public:
	/* Functions */
	void ReadCellFromFile(const std::string& inputFile);
    void ApplyPVT();
	void CellScaling(int _targetProcessNode);
	double GetMemristance(double _relativeReadVoltage);  /* Get the LRS resistance of memristor at log-linera region of I-V curve */
	void CalculateWriteEnergy();
	double CalculateReadPower();
	void PrintCell();

	/* Properties */
	MemCellType memCellType = PCRAM;	/* Memory cell type (like MRAM, PCRAM, etc.) */
	int processNode = 0;        /* Cell original process technology node, Unit: nm*/
	double area = 0;			/* Cell area, Unit: F^2 */
	double aspectRatio = 0;		/* Cell aspect ratio, H/W */
	double widthInFeatureSize;	/* Cell width, Unit: F */
	double heightInFeatureSize;	/* Cell height, Unit: F */
	double resistanceOn = 0;	/* Turn-on resistance */
	double resistanceOff = 0;	/* Turn-off resistance */
	double capacitanceOn;   /* Cell capacitance when memristor is on */
	double capacitanceOff;  /* Cell capacitance when memristor is off */
	bool   readMode = true;		/* true = voltage-mode, false = current-mode */
	double readVoltage = 0;		/* Read voltage */
	double readCurrent = 0;		/* Read current */
	double minSenseVoltage = 0.08; /* Minimum sense voltage */
        double wordlineBoostRatio = 1.0; /*TO-DO: function not realized: ratio of boost wordline voltage to vdd */
	double readPower = 0;       /* Read power per bitline (uW)*/
	bool   resetMode = true;		/* true = voltage-mode, false = current-mode */
	double resetVoltage = 0;	/* Reset voltage */
	double resetCurrent = 0;	/* Reset current */
	double resetPulse = 0;		/* Reset pulse duration (ns) */
	double resetEnergy = 0;     /* Reset energy per cell (pJ) */
	bool   setMode = true;			/* true = voltage-mode, false = current-mode */
	double setVoltage = 0;		/* Set voltage */
	double setCurrent = 0;		/* Set current */
	double setPulse = 0;		/* Set pulse duration (ns) */
	double setEnergy = 0;       /* Set energy per cell (pJ) */
	CellAccessType accessType = CMOS_access;	/* Cell access type: CMOS, BJT, or diode */

	/* Optional properties */
	int stitching = 0;			/* If non-zero, add stitching overhead for every x cells */
	double gateOxThicknessFactor = 2; /* The oxide thickness of FBRAM could be larger than the traditional SOI MOS */
	double widthSOIDevice = 0; /* The gate width of SOI device as FBRAM element, Unit: F*/
	double widthAccessCMOS = 0;	/* The gate width of CMOS access transistor, Unit: F */
	double widthAccessCMOSR = 0;	/* The gate width of CMOS access transistor, Unit: F */
	double voltageDropAccessDevice = 0;  /* The voltage drop on the access device, Unit: V */
	double leakageCurrentAccessDevice = 0;  /* Reverse current of access device, Unit: uA */
	double capDRAMCell = 0;		/* The DRAM cell capacitance if the memory cell is DRAM, Unit: F */
	double widthSRAMCellNMOS = 2.08;	/* The gate width of NMOS in SRAM cells, Unit: F, default NMOS width in SRAM cells is 2.08 (from CACTI) */
	double widthSRAMCellPMOS = 1.23;	/* The gate width of PMOS in SRAM cells, Unit: F, default PMOS width in SRAM cells is 1.23 (from CACTI) */

	/* For memristor */
	bool readFloating = false;      /* If unselected wordlines/bitlines are floating to reduce total leakage */
	double resistanceOnAtSetVoltage = 0; /* Low resistance state when set voltage is applied */
	double resistanceOffAtSetVoltage = 0; /* High resistance state when set voltage is applied */
	double resistanceOnAtResetVoltage = 0; /* Low resistance state when reset voltage is applied */
	double resistanceOffAtResetVoltage = 0; /* High resistance state when reset voltage is applied */
	double resistanceOnAtReadVoltage = 0; /* Low resistance state when read voltage is applied */
	double resistanceOffAtReadVoltage = 0; /* High resistance state when read voltage is applied */
	double resistanceOnAtHalfReadVoltage = 0; /* Low resistance state when 1/2 read voltage is applied */
	double resistanceOffAtHalfReadVoltage = 0; /* High resistance state when 1/2 read voltage is applied */
	double resistanceOnAtHalfResetVoltage = 0; /* Low resistance state when 1/2 reset voltage is applied */

        /*For multi-level cells SA*/
        int nFingers = 8;
        double nLvl = 4;
	/* For NAND flash */
	double flashEraseVoltage;		/* The erase voltage, Unit: V, highest W/E voltage in ITRS sheet */
	double flashPassVoltage;		/* The voltage applied on the unselected wordline within the same block during programming, Unit: V */
	double flashProgramVoltage;		/* The program voltage, Unit: V */
	double flashEraseTime;			/* The flash erase time, Unit: s */
	double flashProgramTime;		/* The SLC flash program time, Unit: s */
	double gateCouplingRatio;		/* The ratio of control gate to total floating gate capacitance */

    /* For eDRAM. */
    double retentionTime = invalid_value;           /* Cell time to data loss (us) */
    double temperature;             /* Temperature for which the cell input values are valid. */
	double maxStorageNodeDrop;
};

#endif /* MSE_MEMCELL_H */
