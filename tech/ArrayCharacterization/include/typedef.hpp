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


#ifndef MSE_TYPEDEF_HPP
#define MSE_TYPEDEF_HPP

enum MemCellType {
	SRAM,
	DRAM,
	eDRAM,
	eDRAM3T,
	eDRAM3T333,
	MRAM,
	PCRAM,
	memristor,
	FBRAM,
	SLCNAND,
	MLCNAND,
	CTT,
	MLCCTT,
	FeFET,
	MLCFeFET,
	MLCRRAM
};

enum CellAccessType {
	CMOS_access,
	BJT_access,
	diode_access,
	none_access
};

enum DeviceRoadmap {
	HP,		/* High performance */
	LSTP,	/* Low standby power */
	LOP,	/* Low operating power */
    EDRAM,   /* Destiny Embedded DRAM */
	IGZO, 
	CNT
};

enum WireType {
	local_aggressive,	/* Width = 2.5F */
	local_conservative,
	semi_aggressive,	/* Width = 4F */
	semi_conservative,
	global_aggressive,	/* Width = 8F */
	global_conservative,
	dram_wordline		/* CACTI 6.5 has this one, but we don't plan to support it at this moment */
};

enum WireRepeaterType {
	repeated_none,		/* No repeater */
	repeated_opt,       /* Add Repeater, optimal delay */
	repeated_5,         /* Add Repeater, 5% delay overhead */
	repeated_10,        /* Add Repeater, 10% delay overhead */
	repeated_20,        /* Add Repeater, 20% delay overhead */
	repeated_30,        /* Add Repeater, 30% delay overhead */
	repeated_40,		/* Add Repeater, 40% delay overhead */
	repeated_50			/* Add Repeater, 50% delay overhead */
};

enum BufferDesignTarget {
	latency_first,				/* The buffer will be optimized for latency */
	latency_area_trade_off,		/* the buffer will be fixed to 2-stage */
	area_first					/* The buffer will be optimized for area */
};

enum MemoryType {
	dataT,
	tag,
	CAM
};

enum RoutingMode {
	h_tree,
	non_h_tree
};

enum WriteScheme {
	set_before_reset,
	reset_before_set,
	erase_before_set,
	erase_before_reset,
	write_and_verify,
	normal_write
};

enum DesignTarget {
	cache,
	RAM_chip,
	CAM_chip
};

enum OptimizationTarget {
	read_latency_optimized,
	write_latency_optimized,
	read_energy_optimized,
	write_energy_optimized,
	read_edp_optimized,
	write_edp_optimized,
	leakage_optimized,
	area_optimized,
	full_exploration
};

enum CacheAccessMode {
	normal_access_mode,		/* data array lookup and tag access happen in parallel
								final data block is broadcasted in data array h-tree
								after getting the signal from the tag array */
	sequential_access_mode,	/* data array is accessed after accessing the tag array */
	fast_access_mode		/* data and tag access happen in parallel */
};

#endif /* MSE_TYPEDEF_HPP */
