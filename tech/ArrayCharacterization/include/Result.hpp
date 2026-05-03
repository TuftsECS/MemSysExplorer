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


#ifndef MSE_RESULT_H
#define MSE_RESULT_H

#include "BankWithHtree.hpp"
#include "BankWithoutHtree.hpp"
#include "Wire.hpp"

#include "yaml-cpp/yaml.h"

#include <memory>

class Result {
public:
	Result();

	/* Functions */
	void print();
	void printAsCache(Result& tagBank, CacheAccessMode cacheAccessMode);
	void reset();
	void printToYamlFile(std::ofstream& outputFile);
	void printAsCacheToYamlFile(Result& tagBank, CacheAccessMode cacheAccessMode, std::ofstream& outputFile);
	YAML::Node toYamlNode();
	YAML::Node toYamlNodeAsCache(Result& tagBank, CacheAccessMode cacheAccessMode);
	void compareAndUpdate(Result& newResult);

	OptimizationTarget optimizationTarget = read_latency_optimized;	/* Exploration should not be assigned here */

	std::unique_ptr<Bank> bank;
	std::unique_ptr<Wire> localWire;		/* TO-DO: this one has the same name as one of the global variables */
	std::unique_ptr<Wire> globalWire;

	double limitReadLatency = invalid_value;			/* The maximum allowable read latency, Unit: s */
	double limitWriteLatency = invalid_value;			/* The maximum allowable write latency, Unit: s */
	double limitReadDynamicEnergy = invalid_value;		/* The maximum allowable read dynamic energy, Unit: J */
	double limitWriteDynamicEnergy = invalid_value;		/* The maximum allowable write dynamic energy, Unit: J */
	double limitReadEdp = invalid_value;				/* The maximum allowable read EDP, Unit: s-J */
	double limitWriteEdp = invalid_value;				/* The maximum allowable write EDP, Unit: s-J */
	double limitArea = invalid_value;					/* The maximum allowable area, Unit: m^2 */
	double limitLeakage = invalid_value;				/* The maximum allowable leakage power, Unit: W */
};

#endif /* MSE_RESULT_H */
