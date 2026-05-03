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
* Contributed by: 
*   Qing Guo	    ( qguo@cs.rochester.edu )
* Note:
*   To support subarray differential write
*******************************************************************************/


#ifndef MSE_BUFFER_H
#define MSE_BUFFER_H

#include "FunctionUnit.hpp"

class Buffer: public FunctionUnit {
public:
	/* Functions */
	void PrintProperty();
	void Initialize(long long _numRow, long long _numColumn);
	void CalculateArea();
	void CalculateRC();
	void CalculateLatency();
	void CalculatePower();

	/* Properties */
	bool initialized = false;        /* Initialization flag */
	bool invalid = false;            /* Indicate that the current configuration is not valid */
	long long numRow;        /* Number of buffered cachelines */
	long long numColumn;     /* Number of columns */
	double xorLatency;       /* Latency of XORing S/A output and Buffer output */
	double xorDynamicEnergy; /* XOR dynamic energy */
	double xorLeakage;       /* XOR leakage energy */

	/* Optional properties (not valid for all the memory cells */
	double setLatency = 0;				/* Unit: s */
    double resetLatency = 0;				/* Unit: s */
	double setDynamicEnergy = 0;	/* Unit: J */
    double resetDynamicEnergy = 0;	/* Unit: J */
	double cellReadEnergy = 0;	/* Unit: J */
    double cellSetEnergy = 0;	/* Unit: J */
    double cellResetEnergy = 0;	/* Unit: J */
};

#endif /* MSE_Buffer_H */
