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


#include "Result.hpp"
#include "global.hpp"
#include "formula.hpp"
#include "macros.hpp"
#include "enumInfo.hpp"
#include "unit.hpp"

#include "yaml-cpp/yaml.h"

#include <iostream>
#include <fstream>
#include <format>

// The DRAM read paths leave the bitline mux out of the subarray latency, so it is left out here too
// The retention time of cells that refresh in seconds, 0 for the rest
static double retentionTime() {
    return mse::cell::memoryCell().properties().retentionTime.value_or(mse::unit::Second()).value();
}

static double muxReadLatency(const mse::component::SubArray& subarray) {
	// Destructive reads have no bitline mux in front of the sense amps
	bool bitlineMuxInReadPath = !mse::cell::memoryCell().properties().destructiveRead;
	double bitlineMuxLatency = bitlineMuxInReadPath ? subarray.bitlineMux.timing().readLatency.value() : 0;
	return bitlineMuxLatency + subarray.senseAmpMuxLevel1.timing().readLatency.value()
			+ subarray.senseAmpMuxLevel2.timing().readLatency.value();
}

Result::Result() {
	localWire = std::make_unique<Wire>();
	globalWire = std::make_unique<Wire>();
}

Result::Result(const Result& other) : Result() {
	bank = other.bank;
	*localWire = *(other.localWire);
	*globalWire = *(other.globalWire);
}

void Result::reset() {
	bank.reset();
}

double Result::readLatency() const {
	return bank ? bank->timing().latency[mse::cell::Operation::Read].value() : mse::constant::invalidValue;
}

double Result::writeLatency() const {
	return bank ? bank->timing().latency[mse::cell::Operation::Write].value() : mse::constant::invalidValue;
}

double Result::readDynamicEnergy() const {
	return bank ? bank->power().dynamicEnergy[mse::cell::Operation::Read].value() : mse::constant::invalidValue;
}

double Result::writeDynamicEnergy() const {
	return bank ? bank->power().dynamicEnergy[mse::cell::Operation::Write].value() : mse::constant::invalidValue;
}

double Result::leakage() const {
	return bank ? bank->power().leakage.value() : mse::constant::invalidValue;
}

double Result::area() const {
	return bank ? bank->dimensions().area().value() : mse::constant::invalidValue;
}

void Result::compareAndUpdate(Result& newResult) {
	if (newResult.readLatency() <= limitReadLatency && newResult.writeLatency() <= limitWriteLatency
			&& newResult.readDynamicEnergy() <= limitReadDynamicEnergy && newResult.writeDynamicEnergy() <= limitWriteDynamicEnergy
			&& newResult.readLatency() * newResult.readDynamicEnergy() <= limitReadEdp
			&& newResult.writeLatency() * newResult.writeDynamicEnergy() <= limitWriteEdp
			&& newResult.area() <= limitArea && newResult.leakage() <= limitLeakage) {
		bool toUpdate = false;
		switch (optimizationTarget) {
			case read_latency_optimized:
				if 	(newResult.readLatency() < readLatency())
					toUpdate = true;
				break;
			case write_latency_optimized:
				if 	(newResult.writeLatency() < writeLatency())
					toUpdate = true;
				break;
			case read_energy_optimized:
				if 	(newResult.readDynamicEnergy() < readDynamicEnergy())
					toUpdate = true;
				break;
			case write_energy_optimized:
				if 	(newResult.writeDynamicEnergy() < writeDynamicEnergy())
					toUpdate = true;
				break;
			case read_edp_optimized:
				if 	(newResult.readLatency() * newResult.readDynamicEnergy() < readLatency() * readDynamicEnergy())
					toUpdate = true;
				break;
			case write_edp_optimized:
				if 	(newResult.writeLatency() * newResult.writeDynamicEnergy() < writeLatency() * writeDynamicEnergy())
					toUpdate = true;
				break;
			case area_optimized:
				if 	(newResult.area() < area())
					toUpdate = true;
				break;
			case leakage_optimized:
				if 	(newResult.leakage() < leakage())
					toUpdate = true;
				break;
			default:	/* Exploration */
				/* should not happen */
				;
		}
		if (toUpdate) {
			bank = newResult.bank;
			*localWire = *(newResult.localWire);
			*globalWire = *(newResult.globalWire);
		}
	}
}

static void printLine(unsigned level, std::string_view title, const mse::component::Streamable auto&... values) {
    if (level > 0) {
        for (unsigned depth = level; depth > 1; --depth) {
            std::cout << "|  ";
        }
        std::cout << "|- ";
    }
    std::cout << title;
    if constexpr (sizeof...(values) > 0) {
        std::cout << ": ";
        (std::cout << ... << values);
    }
    std::cout << "\n";
}

// Prints what one reporter holds at a level: each group under its name, then the ungrouped values
static void printReporter(unsigned level, const mse::component::Reporter& reporter) {
    for (const auto& [group, name] : mse::component::Reporter::reportGroups) {
        if (reporter.getGroup(group).empty()) {
            continue;
        }
        printLine(level, name);
        for (const mse::component::ReportValue& entry : reporter.getGroup(group)) {
            printLine(level + 1, entry.title, entry.value);
        }
    }

    for (const mse::component::ReportValue& entry : reporter.getGroup()) {
        printLine(level, entry.title, entry.value);
    }
}

// Each component reports into its own reporter, and its subcomponents print one level below it
static void printTree(unsigned level, mse::component::Component& component) {
    mse::component::Reporter reporter(std::cout);
    component.report(reporter);

    if (component.instanceName.empty()) {
        printLine(level, component.componentName());
    } else {
        printLine(level, component.componentName(), component.instanceName);
    }
    printReporter(level + 1, reporter);
    component.forEachSubcomponent([&level] (mse::component::Component& subcomponent) noexcept {
        printTree(level + 1, subcomponent);
    });
}

void Result::print() {
    using namespace mse::unit;

    unsigned level = 0;

	std::cout << "\n=============\n"
              << "CONFIGURATION\n"
              << "=============\n";

    printLine(level, "Bank Organization", bank->matRows, " x ", bank->matColumns);
    level++;
    printLine(level, "Row Activation", bank->activeMatsPerColumn, " / ", bank->matRows);
    printLine(level, "Column Activation", bank->activeMatsPerRow, " / ", bank->matColumns);
    level--;

    printLine(level, "Mat Organization", bank->subarrayRows, " x ", bank->subarrayColumns);
    level++;
    printLine(level, "Row Activation", bank->activeSubarraysPerColumn, " / ", bank->subarrayRows);
    printLine(level, "Column Activation", bank->activeSubarraysPerRow, " / ", bank->subarrayColumns);
    printLine(level, "Subarray Size", bank->mat.subarray.rows, " Rows x ", bank->mat.subarray.columns, " Columns");
    level--;

    printLine(level, "Mux Level");
    level++;
    printLine(level, "Senseamp Mux", bank->bitlineMuxInputs);
    printLine(level, "Output Level-1 Mux", bank->senseAmpMuxLevel1Inputs);
    printLine(level, "Output Level-2 Mux", bank->senseAmpMuxLevel2Inputs);
    if (gInputParameter.designTarget == cache) {
        printLine(level, std::format("One set is partitioned into {} rows", bank->rowsPerSet));
    }
    level--;

    printLine(level, "Local Wire");
    level++;
    printLine(level, "Wire Type", localWire->wireType);
    printLine(level, "Repeater Type", localWire->wireRepeaterType);
    printLine(level, "Low Swing", localWire->isLowSwing ? "True" : "False");
    level--;

    printLine(level, "Global Wire");
    level++;
    printLine(level, "Wire Type", globalWire->wireType);
    printLine(level, "Repeater Type", globalWire->wireRepeaterType);
    printLine(level, "Low Swing", globalWire->isLowSwing ? "True" : "False");
    level--;

    printLine(level, "Buffer Design Style", bank->areaOptimizationLevel);

	std::cout << "=============\n"
              << "   RESULT\n"
              << "=============\n";

    const mse::component::SubArray& subarray = bank->mat.subarray;
    const std::string routing = gInputParameter.routingMode == h_tree ? "H-Tree" : "Non-H-Tree";
    // Every cell writes, cells that switch states also report their set and reset, and refreshing cells their refresh
    const mse::cell::OperationSet operations = bank->operations();
    const bool setResetCell = operations.contains(mse::cell::Operation::Set) && operations.contains(mse::cell::Operation::Reset);
    const bool refreshCell = operations.contains(mse::cell::Operation::Refresh) && retentionTime() > 0;
    const bool threeTransistorCell = gCell.memCellType == eDRAM3T333 || gCell.memCellType == eDRAM3T;
    const double cellArea = gCell.area * gTech.featureSize.value() * gTech.featureSize.value() * bank->capacity;

    // RESET, SET and write latencies break down the same way apart from the subarray's own parts
    auto writeLatency = [this, &level, &routing] (std::string_view title, Second bankLatency, Second matLatency,
            Second subarrayLatency, const auto& subarrayParts) {
        printLine(level, title, bankLatency);
        level++;
        printLine(level, routing + " Latency", Second(bankLatency.value() - matLatency.value()));
        printLine(level, "Mat Latency", matLatency);
        level++;
        printLine(level, "Predecoder Latency", bank->mat.timing().predecoderLatency);
        printLine(level, "Subarray Latency", subarrayLatency);
        level++;
        subarrayParts();
        level--;
        level--;
        level--;
    };

    // Erase and programming only break down to the mat
    auto flashLatency = [&level, &routing] (std::string_view title, Second bankLatency, Second matLatency) {
        printLine(level, title, bankLatency);
        level++;
        printLine(level, routing + " Latency", Second(bankLatency.value() - matLatency.value()));
        printLine(level, "Mat Latency", matLatency);
        level--;
    };

    // Every write-type energy uses the subarray's write energies below the mat, apart from the subarray's own parts
    auto writeEnergy = [this, &level, &routing, &subarray] (std::string_view title, std::string_view qualifier, Joule bankEnergy,
            Joule matEnergy, const auto& subarrayParts) {
        printLine(level, title, bankEnergy, qualifier);
        level++;
        printLine(level, routing + " Write Dynamic Energy", Joule(bankEnergy.value() - matEnergy.value()
                * bank->activeMatsPerColumn * bank->activeMatsPerRow));
        printLine(level, "Mat Dynamic Energy", matEnergy, " per mat");
        level++;
        printLine(level, "Predecoder Dynamic Energy", Joule(bank->mat.power().dynamicEnergy[mse::cell::Operation::Write].value()
                - subarray.power().dynamicEnergy[mse::cell::Operation::Write].value() * bank->activeSubarraysPerRow * bank->activeSubarraysPerColumn));
        printLine(level, "Subarray Dynamic Energy", subarray.power().dynamicEnergy[mse::cell::Operation::Write], " per active subarray");
        level++;
        printLine(level, "Row Decoder Dynamic Energy", subarray.power().rowDecoderEnergy[mse::cell::Operation::Write]);
        printLine(level, "Mux Decoder Dynamic Energy", Joule(subarray.bitlineMuxDecoder.power().dynamicWriteEnergy.value()
                + subarray.senseAmpMuxLevel1Decoder.power().dynamicWriteEnergy.value()
                + subarray.senseAmpMuxLevel2Decoder.power().dynamicWriteEnergy.value()));
        printLine(level, "Mux Dynamic Energy", Joule(subarray.bitlineMux.power().dynamicWriteEnergy.value()
                + subarray.senseAmpMuxLevel1.power().dynamicWriteEnergy.value()
                + subarray.senseAmpMuxLevel2.power().dynamicWriteEnergy.value()));
        subarrayParts();
        level--;
        level--;
        level--;
    };

    printLine(level, "Area");
    level++;
    printLine(level, "Total Area", bank->dimensions());
    level++;
    printLine(level, "Mat Area", bank->mat.dimensions(), " (", cellArea / bank->matRows / bank->matColumns
            / bank->mat.dimensions().area().value() * 100, "%)");
    printLine(level, "Subarray Area", subarray.dimensions(), " (", cellArea / bank->matRows / bank->matColumns
            / bank->subarrayRows / bank->subarrayColumns / subarray.dimensions().area().value() * 100, "%)");
    if (subarray.buffer) {
        printLine(level, "Subarray Buffer Area", subarray.buffer->dimensions());
    }
    level--;
    printLine(level, "Area Efficiency", cellArea / bank->dimensions().area().value() * 100, "%");
    level--;

    printLine(level, "Timing");
    level++;

    printLine(level, "Read Latency", bank->timing().latency[mse::cell::Operation::Read]);
    level++;
    printLine(level, routing + " Latency", Second(bank->timing().latency[mse::cell::Operation::Read].value() - bank->mat.timing().latency[mse::cell::Operation::Read].value()));
    printLine(level, "Mat Latency", bank->mat.timing().latency[mse::cell::Operation::Read]);
    level++;
    printLine(level, "Predecoder Latency", bank->mat.timing().predecoderLatency);
    printLine(level, "Subarray Latency", subarray.timing().latency[mse::cell::Operation::Read]);
    level++;
    printLine(level, "Row Decoder Latency", subarray.readRowDecoder ? subarray.readRowDecoder->timing().readLatency
            : subarray.rowDecoder.timing().readLatency);
    printLine(level, "Bitline Latency", threeTransistorCell ? subarray.timing().bitlineReadLatency : subarray.timing().bitlineLatency);
    if (subarray.senseAmp) {
        printLine(level, "Senseamp Latency", subarray.senseAmp->timing().readLatency);
    }
    printLine(level, "Mux Latency", Second(muxReadLatency(subarray)));
    printLine(level, "Precharge Latency", subarray.precharger.timing().readLatency);
    level--;
    if (bank->mat.comparator) {
        printLine(level, "Comparator Latency", bank->mat.comparator->timing().readLatency);
    }
    level--;
    level--;

    writeLatency("Write Latency", bank->timing().latency[mse::cell::Operation::Write], bank->mat.timing().latency[mse::cell::Operation::Write], subarray.timing().latency[mse::cell::Operation::Write],
            [&level, &subarray, &threeTransistorCell] {
        if (gCell.memCellType == MRAM) {
            printLine(level, "Write Pulse Duration", Second(gCell.resetPulse));    // MRAM reset/set is equal
        }
        printLine(level, "Row Decoder Latency", subarray.rowDecoder.timing().writeLatency);
        printLine(level, "Charge Latency", subarray.timing().chargeLatency);
        printLine(level, "Bitline Latency", threeTransistorCell ? subarray.timing().bitlineWriteLatency : subarray.timing().bitlineLatency);
    });
    if (setResetCell && gCell.memCellType == SLCNAND) {
        flashLatency("Erase Latency", bank->timing().latency[mse::cell::Operation::Reset], bank->mat.timing().latency[mse::cell::Operation::Reset]);
        flashLatency("Programming Latency", bank->timing().latency[mse::cell::Operation::Set], bank->mat.timing().latency[mse::cell::Operation::Set]);
    } else if (setResetCell) {
        writeLatency("RESET Latency", bank->timing().latency[mse::cell::Operation::Reset], bank->mat.timing().latency[mse::cell::Operation::Reset], subarray.timing().latency[mse::cell::Operation::Reset],
                [&level, &subarray] {
            printLine(level, "RESET Pulse Duration", Second(gCell.resetPulse));
            printLine(level, "Row Decoder Latency", subarray.rowDecoder.timing().writeLatency);
            printLine(level, "Charge Latency", subarray.timing().chargeLatency);
        });
        writeLatency("SET Latency", bank->timing().latency[mse::cell::Operation::Set], bank->mat.timing().latency[mse::cell::Operation::Set], subarray.timing().latency[mse::cell::Operation::Set],
                [&level, &subarray] {
            printLine(level, "SET Pulse Duration", Second(gCell.setPulse));
            printLine(level, "Row Decoder Latency", subarray.rowDecoder.timing().writeLatency);
            printLine(level, "Charge Latency", subarray.timing().chargeLatency);
        });
    }

    if (subarray.buffer) {
        printLine(level, "Subarray Buffer R/W Latency", subarray.buffer->timing().readLatency);
        printLine(level, "Subarray Buffer XOR Latency", subarray.buffer->timing().xorLatency);
    }

    Unit<BytesAtom<>, SecondAtom<-1>> readBandwidth((double)bank->blockSize / (subarray.timing().latency[mse::cell::Operation::Read].value()
            - subarray.rowDecoder.timing().readLatency.value() + subarray.precharger.timing().readLatency.value()) / 8);
    if (gCell.isMultiLevel()) {
        readBandwidth *= log2(gCell.nLvl);
    }
    printLine(level, "Read Bandwidth", readBandwidth);
    printLine(level, "Write Bandwidth", Unit<BytesAtom<>, SecondAtom<-1>>((double)bank->blockSize / (subarray.timing().latency[mse::cell::Operation::Write].value()) / 8));
    level--;

    printLine(level, "Power");
    level++;

    printLine(level, "Read Dynamic Energy", bank->power().dynamicEnergy[mse::cell::Operation::Read]);
    level++;
    printLine(level, routing + " Read Dynamic Energy", Joule(bank->power().dynamicEnergy[mse::cell::Operation::Read].value()
            - bank->mat.power().dynamicEnergy[mse::cell::Operation::Read].value() * bank->activeMatsPerColumn * bank->activeMatsPerRow));
    printLine(level, "Mat Dynamic Energy", bank->mat.power().dynamicEnergy[mse::cell::Operation::Read], " per mat");
    level++;
    printLine(level, "Predecoder Dynamic Energy", Joule(bank->mat.power().dynamicEnergy[mse::cell::Operation::Read].value()
            - subarray.power().dynamicEnergy[mse::cell::Operation::Read].value() * bank->activeSubarraysPerRow * bank->activeSubarraysPerColumn));
    printLine(level, "Subarray Dynamic Energy", subarray.power().dynamicEnergy[mse::cell::Operation::Read], " per active subarray");
    level++;
    printLine(level, "Row Decoder Dynamic Energy", subarray.power().rowDecoderEnergy[mse::cell::Operation::Read]);
    printLine(level, "Mux Decoder Dynamic Energy", Joule(subarray.bitlineMuxDecoder.power().dynamicReadEnergy.value()
            + subarray.senseAmpMuxLevel1Decoder.power().dynamicReadEnergy.value()
            + subarray.senseAmpMuxLevel2Decoder.power().dynamicReadEnergy.value()));
    if (gCell.memCellType == PCRAM || gCell.memCellType == FBRAM || gCell.memCellType == MRAM || gCell.memCellType == memristor
            || gCell.memCellType == FeFET) {
        printLine(level, "Bitline & Cell Read Energy", subarray.power().cellEnergy[mse::cell::Operation::Read]);
    }
    if (subarray.senseAmp) {
        printLine(level, "Senseamp Dynamic Energy", subarray.senseAmp->power().dynamicReadEnergy);
    }
    printLine(level, "Mux Dynamic Energy", Joule(subarray.bitlineMux.power().dynamicReadEnergy.value()
            + subarray.senseAmpMuxLevel1.power().dynamicReadEnergy.value()
            + subarray.senseAmpMuxLevel2.power().dynamicReadEnergy.value()));
    printLine(level, "Precharge Dynamic Energy", subarray.precharger.power().dynamicReadEnergy);
    level--;
    level--;
    level--;

    writeEnergy("Write Dynamic Energy", "", bank->power().dynamicEnergy[mse::cell::Operation::Write], bank->mat.power().dynamicEnergy[mse::cell::Operation::Write],
            [&level, &subarray] {
        if (gCell.memCellType == MRAM) {
            printLine(level, "Bitline & Cell Write Energy", subarray.power().cellEnergy[mse::cell::Operation::Reset]);
        }
    });
    if (setResetCell && gCell.memCellType == SLCNAND) {
        writeEnergy("Erase Dynamic Energy", " per block", bank->power().dynamicEnergy[mse::cell::Operation::Reset], bank->mat.power().dynamicEnergy[mse::cell::Operation::Reset], [] {});
        writeEnergy("Programming Dynamic Energy", " per page", bank->power().dynamicEnergy[mse::cell::Operation::Set], bank->mat.power().dynamicEnergy[mse::cell::Operation::Set], [] {});
    } else if (setResetCell) {
        writeEnergy("RESET Dynamic Energy", "", bank->power().dynamicEnergy[mse::cell::Operation::Reset], bank->mat.power().dynamicEnergy[mse::cell::Operation::Reset],
                [&level, &subarray] { printLine(level, "Cell RESET Dynamic Energy", subarray.power().cellEnergy[mse::cell::Operation::Reset]); });
        writeEnergy("SET Dynamic Energy", "", bank->power().dynamicEnergy[mse::cell::Operation::Set], bank->mat.power().dynamicEnergy[mse::cell::Operation::Set],
                [&level, &subarray] { printLine(level, "Cell SET Dynamic Energy", subarray.power().cellEnergy[mse::cell::Operation::Set]); });
    }

    if (subarray.buffer) {
        printLine(level, "Subarray Buffer R/W Energy", subarray.buffer->power().dynamicReadEnergy);
        printLine(level, "Subarray Buffer XOR Energy", subarray.buffer->power().xorDynamicEnergy);
    }

    printLine(level, "Leakage Power", bank->power().leakage);
    level++;
    printLine(level, routing + " Leakage Power", Watt(bank->power().leakage.value()
            - bank->mat.power().leakage.value() * bank->matColumns * bank->matRows));
    printLine(level, "Mat Leakage Power", bank->mat.power().leakage, " per mat");
    level--;
    if (refreshCell) {
        // David Note: refresh period could be shorter than retention time
        printLine(level, "Refresh Power", Watt(bank->power().dynamicEnergy[mse::cell::Operation::Refresh].value() / (retentionTime())));
        printLine(level, "Retention Time", Second(retentionTime()));
    }
    level--;

	std::cout << "\n=============\n"
              << " COMPONENTS\n"
              << "=============\n";

    // Values the search never asked for are calculated now, so they need the wires this bank was built with
    gLocalWire = *localWire;
    gGlobalWire = *globalWire;
    printTree(0, *bank);
}


void Result::printAsCache(Result& tagResult, CacheAccessMode cacheAccessMode) {
    using namespace mse::unit;

	if (bank->memoryType != dataT || tagResult.bank->memoryType != tag) {
		std::cout << "This is not a valid cache configuration.\n";
		return;
	} else {
		double cacheHitLatency, cacheMissLatency, cacheWriteLatency;
		double cacheHitDynamicEnergy, cacheMissDynamicEnergy, cacheWriteDynamicEnergy;
		double cacheLeakage;
		double cacheArea;
		if (cacheAccessMode == normal_access_mode) {
			/* Calculate latencies */
			cacheMissLatency = tagResult.bank->timing().latency[mse::cell::Operation::Read].value();		// Only the tag access latency
			cacheHitLatency = std::max(tagResult.bank->timing().latency[mse::cell::Operation::Read].value(), bank->mat.timing().latency[mse::cell::Operation::Read].value());	// Access tag and activate data row in parallel
			cacheHitLatency += bank->mat.subarray.timing().columnDecoderLatency.value();		// Add column decoder latency after hit signal arrives
			cacheHitLatency += bank->timing().latency[mse::cell::Operation::Read].value() - bank->mat.timing().latency[mse::cell::Operation::Read].value();	// H-tree in and out latency
			cacheWriteLatency = std::max(tagResult.bank->timing().latency[mse::cell::Operation::Write].value(), bank->timing().latency[mse::cell::Operation::Write].value());	// Data and tag are written in parallel
			/* Calculate power */
			cacheMissDynamicEnergy = tagResult.bank->power().dynamicEnergy[mse::cell::Operation::Read].value();	// No matter what tag is always accessed
			cacheMissDynamicEnergy += bank->power().dynamicEnergy[mse::cell::Operation::Read].value();	// Data is also partially accessed, TODO: not accurate here
			cacheHitDynamicEnergy = tagResult.bank->power().dynamicEnergy[mse::cell::Operation::Read].value() + bank->power().dynamicEnergy[mse::cell::Operation::Read].value();
			cacheWriteDynamicEnergy = tagResult.bank->power().dynamicEnergy[mse::cell::Operation::Write].value() + bank->power().dynamicEnergy[mse::cell::Operation::Write].value();
		} else if (cacheAccessMode == fast_access_mode) {
			/* Calculate latencies */
			cacheMissLatency = tagResult.bank->timing().latency[mse::cell::Operation::Read].value();
			cacheHitLatency = std::max(tagResult.bank->timing().latency[mse::cell::Operation::Read].value(), bank->timing().latency[mse::cell::Operation::Read].value());
			cacheWriteLatency = std::max(tagResult.bank->timing().latency[mse::cell::Operation::Write].value(), bank->timing().latency[mse::cell::Operation::Write].value());
			/* Calculate power */
			cacheMissDynamicEnergy = tagResult.bank->power().dynamicEnergy[mse::cell::Operation::Read].value();	// No matter what tag is always accessed
			cacheMissDynamicEnergy += bank->power().dynamicEnergy[mse::cell::Operation::Read].value();	// Data is also partially accessed, TODO: not accurate here
			cacheHitDynamicEnergy = tagResult.bank->power().dynamicEnergy[mse::cell::Operation::Read].value() + bank->power().dynamicEnergy[mse::cell::Operation::Read].value();
			cacheWriteDynamicEnergy = tagResult.bank->power().dynamicEnergy[mse::cell::Operation::Write].value() + bank->power().dynamicEnergy[mse::cell::Operation::Write].value();
		} else {		/* sequential access */
			/* Calculate latencies */
			cacheMissLatency = tagResult.bank->timing().latency[mse::cell::Operation::Read].value();
			cacheHitLatency = tagResult.bank->timing().latency[mse::cell::Operation::Read].value() + bank->timing().latency[mse::cell::Operation::Read].value();
			cacheWriteLatency = std::max(tagResult.bank->timing().latency[mse::cell::Operation::Write].value(), bank->timing().latency[mse::cell::Operation::Write].value());
			/* Calculate power */
			cacheMissDynamicEnergy = tagResult.bank->power().dynamicEnergy[mse::cell::Operation::Read].value();	// No matter what tag is always accessed
			cacheHitDynamicEnergy = tagResult.bank->power().dynamicEnergy[mse::cell::Operation::Read].value() + bank->power().dynamicEnergy[mse::cell::Operation::Read].value();
			cacheWriteDynamicEnergy = tagResult.bank->power().dynamicEnergy[mse::cell::Operation::Write].value() + bank->power().dynamicEnergy[mse::cell::Operation::Write].value();
		}
		/* Calculate leakage */
		cacheLeakage = tagResult.bank->power().leakage.value() + bank->power().leakage.value();
		/* Calculate area */
		cacheArea = tagResult.bank->dimensions().area().value() + bank->dimensions().area().value();	// TODO: simply add them together here

		/* start printing */
		std::cout << "\n=======================\n"
                  << "CACHE DESIGN -- SUMMARY\n"
                  << "=======================\n";

        unsigned level = 0;

        printLine(level, "Access Mode", cacheAccessMode);

        printLine(level, "Area");
        level++;
        printLine(level, "Total Area", MeterExp<2>(cacheArea));
        level++;
        printLine(level, "Data Array Area", bank->dimensions());
        printLine(level, "Tag Array Area", tagResult.bank->dimensions());
        level--;
        level--;

        printLine(level, "Timing");
        level++;
        printLine(level, "Cache Hit Latency", Second(cacheHitLatency));
        printLine(level, "Cache Miss Latency", Second(cacheMissLatency));
        printLine(level, "Cache Write Latency", Second(cacheWriteLatency));
        const bool refreshCell = bank->operations().contains(mse::cell::Operation::Refresh) && retentionTime() > 0;
        if (refreshCell) {
            const double refreshLatency = std::max(tagResult.bank->timing().latency[mse::cell::Operation::Refresh].value(), bank->timing().latency[mse::cell::Operation::Refresh].value());
            printLine(level, "Cache Refresh Latency", Second(refreshLatency), " per bank");
            printLine(level, "Cache Availability", ((retentionTime() - refreshLatency) / retentionTime()) * 100.0, "%");
        }
        level--;

        printLine(level, "Power");
        level++;
        printLine(level, "Cache Hit Dynamic Energy", Joule(cacheHitDynamicEnergy), " per access");
        printLine(level, "Cache Miss Dynamic Energy", Joule(cacheMissDynamicEnergy), " per access");
        printLine(level, "Cache Write Dynamic Energy", Joule(cacheWriteDynamicEnergy), " per access");
        if (refreshCell) {
            printLine(level, "Cache Refresh Dynamic Energy", Joule(tagResult.bank->power().dynamicEnergy[mse::cell::Operation::Refresh].value()
                    + bank->power().dynamicEnergy[mse::cell::Operation::Refresh].value()), " per bank");
        }
        printLine(level, "Cache Total Leakage Power", Watt(cacheLeakage));
        level++;
        printLine(level, "Cache Data Array Leakage Power", bank->power().leakage);
        printLine(level, "Cache Tag Array Leakage Power", tagResult.bank->power().leakage);
        level--;
        if (refreshCell) {
            printLine(level, "Cache Refresh Power", Watt(bank->power().dynamicEnergy[mse::cell::Operation::Refresh].value() / (retentionTime())), " per bank");
            printLine(level, "Cache Retention Time", Second(retentionTime()));
        }
        level--;

		std::cout << std::endl << "CACHE DATA ARRAY";
		print();
		std::cout << std::endl << "CACHE TAG ARRAY";
		tagResult.print();
	}
}

YAML::Node Result::toYamlNode() {
	YAML::Node result;

	if(gInputParameter.designTarget != cache){
		// Helper to convert DeviceRoadmap enums to string
		auto roadmapToString = [](DeviceRoadmap roadmap) -> std::string {
			switch (roadmap) {
				case HP: return "HP";
				case LSTP: return "LSTP";
				case LOP: return "LOP";
				default: return "ULP";
			}
		};

		// Memory cell type
		switch (gCell.memCellType) {
			case SRAM: result["MemoryCell"]["MemoryCellType"] = "SRAM"; break;
			case DRAM: result["MemoryCell"]["MemoryCellType"] = "DRAM"; break;
			case eDRAM: result["MemoryCell"]["MemoryCellType"] = "eDRAM"; break;
			case eDRAM3T: result["MemoryCell"]["MemoryCellType"] = "3T eDRAM"; break;
			case eDRAM3T333: result["MemoryCell"]["MemoryCellType"] = "333eDRAM"; break;
			case MRAM: result["MemoryCell"]["MemoryCellType"] = "MRAM (Magnetoresistive)"; break;
			case PCRAM: result["MemoryCell"]["MemoryCellType"] = "PCRAM (Phase-Change)"; break;
			case memristor: result["MemoryCell"]["MemoryCellType"] = gCell.isMultiLevel() ? "Multi-Level Cell RRAM (Memristor)" : "RRAM (Memristor)"; break;
			case FBRAM: result["MemoryCell"]["MemoryCellType"] = "FBRAM (Floating Body)"; break;
			case SLCNAND: result["MemoryCell"]["MemoryCellType"] = "Single-Level Cell NAND Flash"; break;
			case MLCNAND: result["MemoryCell"]["MemoryCellType"] = "Multi-Level Cell NAND Flash"; break;
			case CTT: result["MemoryCell"]["MemoryCellType"] = gCell.isMultiLevel() ? "Multi-Level Cell CTT" : "Single-Level Cell CTT"; break;
			case FeFET: result["MemoryCell"]["MemoryCellType"] = gCell.isMultiLevel() ? "Multi-Level Cell FeFET" : "Single-Level Cell FeFET"; break;
			default: result["MemoryCell"]["MemoryCellType"] = "Unknown"; break;
		}


		// Cell area
		result["MemoryCell"]["CellArea_F2"]  = gCell.area;
		result["MemoryCell"]["CellArea_um2"] = gCell.area / 1000000.0 * gTech.featureSizeInNano * gTech.featureSizeInNano;
		result["MemoryCell"]["AspectRatio"]  = gCell.aspectRatio;

		// Resistive / Non-volatile memory
		if (gCell.memCellType == PCRAM || gCell.memCellType == MRAM || gCell.memCellType == memristor ||
			gCell.memCellType == FBRAM || gCell.memCellType == FeFET) {

			if (gCell.resistanceOn < 1e3)
				result["MemoryCell"]["R_on_Ohm"] = gCell.resistanceOn;
			else if (gCell.resistanceOn < 1e6)
				result["MemoryCell"]["R_on_KOhm"] = gCell.resistanceOn / 1e3;
			else
				result["MemoryCell"]["R_on_MOhm"] = gCell.resistanceOn / 1e6;

			if (gCell.resistanceOff < 1e3)
				result["MemoryCell"]["R_off_Ohm"] = gCell.resistanceOff;
			else if (gCell.resistanceOff < 1e6)
				result["MemoryCell"]["R_off_KOhm"] = gCell.resistanceOff / 1e3;
			else
				result["MemoryCell"]["R_off_MOhm"] = gCell.resistanceOff / 1e6;

			result["MemoryCell"]["ReadMode"]  = gCell.readMode ? "Voltage-Sensing" : "Current-Sensing";
			if (gCell.readCurrent > 0) result["MemoryCell"]["ReadCurrent_uA"] = gCell.readCurrent * 1e6;
			if (gCell.readVoltage > 0) result["MemoryCell"]["ReadVoltage_V"] = gCell.readVoltage;

			result["MemoryCell"]["ResetMode"] = gCell.resetMode ? "Voltage" : "Current";
			result["MemoryCell"]["ResetVoltage_V"] = gCell.resetVoltage;
			result["MemoryCell"]["ResetCurrent_uA"] = gCell.resetCurrent * 1e6;
			result["MemoryCell"]["ResetPulse_s"] = gCell.resetPulse / 1e9;

			result["MemoryCell"]["SetMode"] = gCell.setMode ? "Voltage" : "Current";
			result["MemoryCell"]["SetVoltage_V"] = gCell.setVoltage;
			result["MemoryCell"]["SetCurrent_uA"] = gCell.setCurrent * 1e6;
			result["MemoryCell"]["SetPulse_s"] = gCell.setPulse / 1e9;

			switch (gCell.accessType) {
				case CMOS_access: result["MemoryCell"]["AccessType"] = "CMOS"; break;
				case BJT_access: result["MemoryCell"]["AccessType"] = "BJT"; break;
				case diode_access: result["MemoryCell"]["AccessType"] = "Diode"; break;
				default: result["MemoryCell"]["AccessType"] = "None Access Device"; break;
			}
		}

		// SRAM
		if (gCell.memCellType == SRAM) {
			result["MemoryCell"]["WidthAccessCMOS_F"]   = gCell.widthAccessCMOS;
			result["MemoryCell"]["WidthSRAMCellNMOS_F"] = gCell.widthSRAMCellNMOS;
			result["MemoryCell"]["WidthSRAMCellPMOS_F"] = gCell.widthSRAMCellPMOS;
			result["MemoryCell"]["PeripheralRoadmap"]   = roadmapToString(gTech.deviceRoadmap);
			result["MemoryCell"]["PeripheralNode_nm"]   = gTech.featureSizeInNano;
			result["MemoryCell"]["VDD_V"]               = gTech.vdd.value();
			result["MemoryCell"]["WWL_SWING"]           = gTech.vdd.value();
			result["MemoryCell"]["Temperature_K"]       = gCell.temperature;
		}

		// DRAM / eDRAM
		if (gCell.memCellType == DRAM || gCell.memCellType == eDRAM) {
			result["MemoryCell"]["WidthAccessCMOS_F"] = gCell.widthAccessCMOS;
			result["MemoryCell"]["PeripheralRoadmap"] = roadmapToString(gTech.deviceRoadmap);
			result["MemoryCell"]["PeripheralNode_nm"] = gTech.featureSizeInNano;
			result["MemoryCell"]["VDD_V"] = gTech.vdd.value();
			result["MemoryCell"]["WWL_SWING"] = gTech.vpp.value();
			result["MemoryCell"]["Temperature_K"] = gCell.temperature;
		}

		// 3T DRAM
		if (gCell.memCellType == eDRAM3T || gCell.memCellType == eDRAM3T333) {
			result["MemoryCell"]["WidthWriteAccessCMOS_F"] = gCell.widthAccessCMOS;
			result["MemoryCell"]["WidthReadAccessCMOS_F"]  = gCell.widthAccessCMOSR;
			result["MemoryCell"]["PeripheralRoadmap"]      = roadmapToString(gTech.deviceRoadmap);
			result["MemoryCell"]["WriteAccessRoadmap"]     = roadmapToString(gTechW.deviceRoadmap);
			result["MemoryCell"]["ReadAccessRoadmap"]      = roadmapToString(gTechR.deviceRoadmap);
			result["MemoryCell"]["PeripheralNode_nm"]      = gTech.featureSizeInNano;
			result["MemoryCell"]["WriteAccessNode_nm"]     = gTechW.featureSizeInNano;
			result["MemoryCell"]["ReadAccessNode_nm"]      = gTechR.featureSizeInNano;
			result["MemoryCell"]["VDD_V"]                  = gTech.vdd.value();
			result["MemoryCell"]["WWL_SWING"]              = gTechW.vpp.value();
			result["MemoryCell"]["Temperature_K"]          = gCell.temperature;
		}

		// SLC NAND Flash
		if (gCell.memCellType == SLCNAND) {
			result["MemoryCell"]["PassVoltage_V"]     = gCell.flashPassVoltage;
			result["MemoryCell"]["ProgramVoltage_V"]  = gCell.flashProgramVoltage;
			result["MemoryCell"]["EraseVoltage_V"]    = gCell.flashEraseVoltage;
			result["MemoryCell"]["ProgramTime_s"]     = gCell.flashProgramTime / 1e9;
			result["MemoryCell"]["EraseTime_s"]       = gCell.flashEraseTime / 1e9;
			result["MemoryCell"]["GateCouplingRatio"] = gCell.gateCouplingRatio;
		}

		// Multi-level cells
		if (gCell.isMultiLevel()) {
			result["MemoryCell"]["NumberOfInputFingers"]   = gCell.nFingers;
			result["MemoryCell"]["NumberOfLevelsPerCell"] = gCell.nLvl;
		}

	}

	if(gInputParameter.designTarget != cache){
		//Capacity
		if (gInputParameter.capacity < 1024) {
			result["Capacity"]["Value"] = gInputParameter.capacity;
			result["Capacity"]["Unit"] = "B";
		} else if (gInputParameter.capacity < 1024 * 1024) {
			result["Capacity"]["Value"] = gInputParameter.capacity / 1024;
			result["Capacity"]["Unit"] = "KB";
		} else if (gInputParameter.capacity < 1024 * 1024 * 1024) {
			result["Capacity"]["Value"] = gInputParameter.capacity / 1024 / 1024;
			result["Capacity"]["Unit"] = "MB";
		} else {
			result["Capacity"]["Value"] = gInputParameter.capacity / 1024 / 1024 / 1024;
			result["Capacity"]["Unit"] = "GB";
		}



		switch (optimizationTarget) {
			case read_latency_optimized: result["OptimizationTarget"] = "ReadLatency"; break;
			case write_latency_optimized: result["OptimizationTarget"] = "WriteLatency"; break;
			case read_energy_optimized: result["OptimizationTarget"] = "ReadDynamicEnergy"; break;
			case write_energy_optimized: result["OptimizationTarget"] = "WriteDynamicEnergy"; break;
			case read_edp_optimized: result["OptimizationTarget"] = "ReadEDP"; break;
			case write_edp_optimized: result["OptimizationTarget"] = "WriteEDP"; break;
			case leakage_optimized: result["OptimizationTarget"] = "LeakagePower"; break;
			case area_optimized: result["OptimizationTarget"] = "Area"; break;
			default:                 result["OptimizationTarget"] = "Unknown";
		}
	}

    // Configuration
    result["Configuration"]["BankOrganization"]["Rows"] = bank->matRows;
    result["Configuration"]["BankOrganization"]["Columns"] = bank->matColumns;
    result["Configuration"]["BankOrganization"]["RowActivation"] = bank->activeMatsPerColumn;
    result["Configuration"]["BankOrganization"]["TotalRows"] = bank->matRows;
    result["Configuration"]["BankOrganization"]["ColumnActivation"] = bank->activeMatsPerRow;
    result["Configuration"]["BankOrganization"]["TotalColumns"] = bank->matColumns;

    result["Configuration"]["MatOrganization"]["Rows"] = bank->subarrayRows;
    result["Configuration"]["MatOrganization"]["Columns"] = bank->subarrayColumns;
    result["Configuration"]["MatOrganization"]["RowActivation"] = bank->activeSubarraysPerColumn;
    result["Configuration"]["MatOrganization"]["TotalRows"] = bank->subarrayRows;
    result["Configuration"]["MatOrganization"]["ColumnActivation"] = bank->activeSubarraysPerRow;
    result["Configuration"]["MatOrganization"]["TotalColumns"] = bank->subarrayColumns;
    result["Configuration"]["MatOrganization"]["SubarrayRows"] = bank->mat.subarray.rows;
    result["Configuration"]["MatOrganization"]["SubarrayColumns"] = bank->mat.subarray.columns;

    result["Configuration"]["MuxLevels"]["SenseampMux"] = bank->bitlineMuxInputs;
    result["Configuration"]["MuxLevels"]["OutputLevel1Mux"] = bank->senseAmpMuxLevel1Inputs;
    result["Configuration"]["MuxLevels"]["OutputLevel2Mux"] = bank->senseAmpMuxLevel2Inputs;
    if (gInputParameter.designTarget == cache)
        result["Configuration"]["MuxLevels"]["RowsPerSet"] = bank->rowsPerSet;

    // Local Wire
    switch (localWire->wireType) {
        case local_aggressive: result["Configuration"]["LocalWire"]["WireType"] = "LocalAggressive"; break;
        case local_conservative: result["Configuration"]["LocalWire"]["WireType"] = "LocalConservative"; break;
        case semi_aggressive: result["Configuration"]["LocalWire"]["WireType"] = "SemiAggressive"; break;
        case semi_conservative: result["Configuration"]["LocalWire"]["WireType"] = "SemiConservative"; break;
        case global_aggressive: result["Configuration"]["LocalWire"]["WireType"] = "GlobalAggressive"; break;
        case global_conservative: result["Configuration"]["LocalWire"]["WireType"] = "GlobalConservative"; break;
        default: result["Configuration"]["LocalWire"]["WireType"] = "DRAMWire";
    }
    switch (localWire->wireRepeaterType) {
        case repeated_none: result["Configuration"]["LocalWire"]["RepeaterType"] = "NoRepeaters"; break;
        case repeated_opt: result["Configuration"]["LocalWire"]["RepeaterType"] = "FullyOptimized"; break;
        case repeated_5: result["Configuration"]["LocalWire"]["RepeaterType"] = "Repeated5Percent"; break;
        case repeated_10: result["Configuration"]["LocalWire"]["RepeaterType"] = "Repeated10Percent"; break;
        case repeated_20: result["Configuration"]["LocalWire"]["RepeaterType"] = "Repeated20Percent"; break;
        case repeated_30: result["Configuration"]["LocalWire"]["RepeaterType"] = "Repeated30Percent"; break;
        case repeated_40: result["Configuration"]["LocalWire"]["RepeaterType"] = "Repeated40Percent"; break;
        case repeated_50: result["Configuration"]["LocalWire"]["RepeaterType"] = "Repeated50Percent"; break;
        default: result["Configuration"]["LocalWire"]["RepeaterType"] = "Unknown";
    }
    result["Configuration"]["LocalWire"]["LowSwing"] = localWire->isLowSwing ? "Yes" : "No";

    // Global Wire
    switch (globalWire->wireType) {
        case local_aggressive: result["Configuration"]["GlobalWire"]["WireType"] = "LocalAggressive"; break;
        case local_conservative: result["Configuration"]["GlobalWire"]["WireType"] = "LocalConservative"; break;
        case semi_aggressive: result["Configuration"]["GlobalWire"]["WireType"] = "SemiAggressive"; break;
        case semi_conservative: result["Configuration"]["GlobalWire"]["WireType"] = "SemiConservative"; break;
        case global_aggressive: result["Configuration"]["GlobalWire"]["WireType"] = "GlobalAggressive"; break;
        case global_conservative: result["Configuration"]["GlobalWire"]["WireType"] = "GlobalConservative"; break;
        default: result["Configuration"]["GlobalWire"]["WireType"] = "DRAMWire";
    }
    switch (globalWire->wireRepeaterType) {
        case repeated_none: result["Configuration"]["GlobalWire"]["RepeaterType"] = "NoRepeaters"; break;
        case repeated_opt: result["Configuration"]["GlobalWire"]["RepeaterType"] = "FullyOptimized"; break;
        case repeated_5: result["Configuration"]["GlobalWire"]["RepeaterType"] = "Repeated5Percent"; break;
        case repeated_10: result["Configuration"]["GlobalWire"]["RepeaterType"] = "Repeated10Percent"; break;
        case repeated_20: result["Configuration"]["GlobalWire"]["RepeaterType"] = "Repeated20Percent"; break;
        case repeated_30: result["Configuration"]["GlobalWire"]["RepeaterType"] = "Repeated30Percent"; break;
        case repeated_40: result["Configuration"]["GlobalWire"]["RepeaterType"] = "Repeated40Percent"; break;
        case repeated_50: result["Configuration"]["GlobalWire"]["RepeaterType"] = "Repeated50Percent"; break;
        default: result["Configuration"]["GlobalWire"]["RepeaterType"] = "Unknown";
    }
    result["Configuration"]["GlobalWire"]["LowSwing"] = globalWire->isLowSwing ? "Yes" : "No";

    switch (bank->areaOptimizationLevel) {
        case latency_first: result["Configuration"]["BufferDesignStyle"] = "LatencyOptimized"; break;
        case area_first: result["Configuration"]["BufferDesignStyle"] = "AreaOptimized"; break;
        default: result["Configuration"]["BufferDesignStyle"] = "Balanced";
    }

    // Area
    result["Results"]["Area"]["Total"]["Height_um"] = bank->dimensions().height.value() * 1e6;
    result["Results"]["Area"]["Total"]["Width_um"] = bank->dimensions().width.value() * 1e6;
    result["Results"]["Area"]["Total"]["Area_mm2"] = bank->dimensions().area().value() * 1e6;

    result["Results"]["Area"]["Mat"]["Height_um"] = bank->mat.dimensions().height.value() * 1e6;
    result["Results"]["Area"]["Mat"]["Width_um"] = bank->mat.dimensions().width.value() * 1e6;
    result["Results"]["Area"]["Mat"]["Area_mm2"] = bank->mat.dimensions().area().value() * 1e6;
    result["Results"]["Area"]["Mat"]["Efficiency_percent"] = 
        (gCell.area * gTech.featureSize.value() * gTech.featureSize.value() * bank->capacity / 
         bank->matRows / bank->matColumns / bank->mat.dimensions().area().value() * 100);

    result["Results"]["Area"]["Subarray"]["Height_um"] = bank->mat.subarray.dimensions().height.value() * 1e6;
    result["Results"]["Area"]["Subarray"]["Width_um"] = bank->mat.subarray.dimensions().width.value() * 1e6;
    result["Results"]["Area"]["Subarray"]["Area_mm2"] = bank->mat.subarray.dimensions().area().value() * 1e6;
    result["Results"]["Area"]["Subarray"]["Efficiency_percent"] =
        (gCell.area * gTech.featureSize.value() * gTech.featureSize.value() * bank->capacity / 
         bank->matRows / bank->matColumns / bank->subarrayRows / 
         bank->subarrayColumns / bank->mat.subarray.dimensions().area().value() * 100);

    result["Results"]["Area"]["AreaEfficiency_percent"] =
        (gCell.area * gTech.featureSize.value() * gTech.featureSize.value() * bank->capacity / bank->dimensions().area().value() * 100);

    // Timing
    result["Results"]["Timing"]["Read"]["Latency_ns"] = bank->timing().latency[mse::cell::Operation::Read].value() * 1e9;
    result["Results"]["Timing"]["Read"]["TreeLatency_ns"] = 
        (bank->timing().latency[mse::cell::Operation::Read].value() - bank->mat.timing().latency[mse::cell::Operation::Read].value()) * 1e9;
    result["Results"]["Timing"]["Read"]["MatLatency_ns"] = bank->mat.timing().latency[mse::cell::Operation::Read].value() * 1e9;
    result["Results"]["Timing"]["Read"]["PredecoderLatency_ns"] = bank->mat.timing().predecoderLatency.value() * 1e9;
    result["Results"]["Timing"]["Read"]["SubarrayLatency_ns"] = bank->mat.subarray.timing().latency[mse::cell::Operation::Read].value() * 1e9;
    result["Results"]["Timing"]["Read"]["RowDecoderLatency_ns"] = 
        bank->mat.subarray.rowDecoder.timing().readLatency.value() * 1e9;
	if (gCell.memCellType == eDRAM3T333 || gCell.memCellType == eDRAM3T) {
    	result["Results"]["Timing"]["Read"]["BitlineLatency_ns"] = bank->mat.subarray.timing().bitlineReadLatency.value() * 1e9;
	} else {
		result["Results"]["Timing"]["Read"]["BitlineLatency_ns"] = bank->mat.subarray.timing().bitlineLatency.value() * 1e9;
	}
    if (bank->mat.subarray.senseAmp)
        result["Results"]["Timing"]["Read"]["SenseampLatency_ns"] = 
            bank->mat.subarray.senseAmp->timing().readLatency.value() * 1e9;
    result["Results"]["Timing"]["Read"]["MuxLatency_ns"] =
        muxReadLatency(bank->mat.subarray) * 1e9;
    result["Results"]["Timing"]["Read"]["PrechargeLatency_ns"] = 
        bank->mat.subarray.precharger.timing().readLatency.value() * 1e9;

    // Every cell writes, cells that switch states also report their set and reset, and refreshing cells their refresh
    const mse::cell::OperationSet operations = bank->operations();
    const bool setResetCell = operations.contains(mse::cell::Operation::Set) && operations.contains(mse::cell::Operation::Reset);
    const bool refreshCell = operations.contains(mse::cell::Operation::Refresh) && retentionTime() > 0;

    result["Results"]["Timing"]["Write"]["Latency_ns"] = bank->timing().latency[mse::cell::Operation::Write].value() * 1e9;
    result["Results"]["Timing"]["Write"]["TreeLatency_ns"] = 
        (bank->timing().latency[mse::cell::Operation::Write].value() - bank->mat.timing().latency[mse::cell::Operation::Write].value()) * 1e9;
    result["Results"]["Timing"]["Write"]["MatLatency_ns"] = bank->mat.timing().latency[mse::cell::Operation::Write].value() * 1e9;
		result["Results"]["Timing"]["Write"]["PredecoderLatency_ns"] = bank->mat.timing().predecoderLatency.value() * 1e9;
		// TODO the write breakdown has no write values yet, so the subarray and row decoder latencies are the read ones
		result["Results"]["Timing"]["Write"]["SubarrayLatency_ns"] = bank->mat.subarray.timing().latency[mse::cell::Operation::Read].value() * 1e9;
		result["Results"]["Timing"]["Write"]["RowDecoderLatency_ns"] = 
			bank->mat.subarray.rowDecoder.timing().readLatency.value() * 1e9;
		if (gCell.memCellType == eDRAM3T333 || gCell.memCellType == eDRAM3T) {
			result["Results"]["Timing"]["Write"]["BitlineLatency_ns"] = bank->mat.subarray.timing().bitlineWriteLatency.value() * 1e9;
		} else {
			result["Results"]["Timing"]["Write"]["BitlineLatency_ns"] = bank->mat.subarray.timing().bitlineLatency.value() * 1e9;
		}

    if (setResetCell && gCell.memCellType == SLCNAND) {
        result["Results"]["Timing"]["Erase"]["Latency_ns"] = bank->timing().latency[mse::cell::Operation::Reset].value() * 1e9;
        result["Results"]["Timing"]["Programming"]["Latency_ns"] = bank->timing().latency[mse::cell::Operation::Set].value() * 1e9;
    } else if (setResetCell) {
        // RESET latency with proper TreeLatency calculation
        result["Results"]["Timing"]["Reset"]["Latency_ns"] = bank->timing().latency[mse::cell::Operation::Reset].value() * 1e9;
        result["Results"]["Timing"]["Reset"]["TreeLatency_ns"] = 
            (bank->timing().latency[mse::cell::Operation::Reset].value() - bank->mat.timing().latency[mse::cell::Operation::Reset].value()) * 1e9;
        result["Results"]["Timing"]["Reset"]["MatLatency_ns"] = bank->mat.timing().latency[mse::cell::Operation::Reset].value() * 1e9;
        result["Results"]["Timing"]["Reset"]["PulseDuration_ns"] = gCell.resetPulse * 1e9;

        // SET latency with proper TreeLatency calculation
        result["Results"]["Timing"]["Set"]["Latency_ns"] = bank->timing().latency[mse::cell::Operation::Set].value() * 1e9;
        result["Results"]["Timing"]["Set"]["TreeLatency_ns"] = 
            (bank->timing().latency[mse::cell::Operation::Set].value() - bank->mat.timing().latency[mse::cell::Operation::Set].value()) * 1e9;
        result["Results"]["Timing"]["Set"]["MatLatency_ns"] = bank->mat.timing().latency[mse::cell::Operation::Set].value() * 1e9;
        result["Results"]["Timing"]["Set"]["PulseDuration_ns"] = gCell.setPulse * 1e9;
    }

    double readBandwidth = (double)bank->blockSize /
        (bank->mat.subarray.timing().latency[mse::cell::Operation::Read].value() - bank->mat.subarray.rowDecoder.timing().readLatency.value()
         + bank->mat.subarray.precharger.timing().readLatency.value()) / 8;
    if (gCell.isMultiLevel()) {
        readBandwidth *= log2(gCell.nLvl);
    }
    result["Results"]["Timing"]["ReadBandwidth_Bps"] = readBandwidth;

    double writeBandwidth = (double)bank->blockSize / (bank->mat.subarray.timing().latency[mse::cell::Operation::Write].value()) / 8;
    result["Results"]["Timing"]["WriteBandwidth_Bps"] = writeBandwidth;

    // Power
    result["Results"]["Power"]["Read"]["DynamicEnergy_pJ"] = bank->power().dynamicEnergy[mse::cell::Operation::Read].value() * 1e12;
    result["Results"]["Power"]["Read"]["TreeDynamicEnergy_pJ"] =
        (bank->power().dynamicEnergy[mse::cell::Operation::Read].value() - bank->mat.power().dynamicEnergy[mse::cell::Operation::Read].value() * 
         bank->activeMatsPerColumn * bank->activeMatsPerRow) * 1e12;
    result["Results"]["Power"]["Read"]["MatDynamicEnergy_pJ"] = 
        bank->mat.power().dynamicEnergy[mse::cell::Operation::Read].value() * 1e12;
    result["Results"]["Power"]["Read"]["SubarrayDynamicEnergy_pJ"] = 
        bank->mat.subarray.power().dynamicEnergy[mse::cell::Operation::Read].value() * 1e12;

    result["Results"]["Power"]["Write"]["DynamicEnergy_pJ"] = bank->power().dynamicEnergy[mse::cell::Operation::Write].value() * 1e12;

    if (setResetCell && gCell.memCellType == SLCNAND) {
        result["Results"]["Power"]["Erase"]["DynamicEnergy_pJ"] = bank->power().dynamicEnergy[mse::cell::Operation::Reset].value() * 1e12;
        result["Results"]["Power"]["Programming"]["DynamicEnergy_pJ"] = bank->power().dynamicEnergy[mse::cell::Operation::Set].value() * 1e12;
    } else if (setResetCell) {
        result["Results"]["Power"]["Reset"]["DynamicEnergy_pJ"] = bank->power().dynamicEnergy[mse::cell::Operation::Reset].value() * 1e12;
        result["Results"]["Power"]["Reset"]["CellResetEnergy_pJ"] = 
            bank->mat.subarray.power().cellEnergy[mse::cell::Operation::Reset].value() * 1e12;

        result["Results"]["Power"]["Set"]["DynamicEnergy_pJ"] = bank->power().dynamicEnergy[mse::cell::Operation::Set].value() * 1e12;
        result["Results"]["Power"]["Set"]["CellSetEnergy_pJ"] = 
            bank->mat.subarray.power().cellEnergy[mse::cell::Operation::Set].value() * 1e12;
    }

    result["Results"]["Power"]["Leakage_mW"] = bank->power().leakage.value() * 1e3;

    if (refreshCell) {
        result["Results"]["Power"]["RefreshPower_W"] = 
            (bank->power().dynamicEnergy[mse::cell::Operation::Refresh].value() / retentionTime());
        result["Results"]["Power"]["RetentionTime_us"] = retentionTime() * 1e6;
    }

    return result;
}

YAML::Node Result::toYamlNodeAsCache(Result& tagResult, CacheAccessMode cacheAccessMode) {
    if (bank->memoryType != dataT || tagResult.bank->memoryType != tag) {
        std::cout << "This is not a valid cache configuration." << std::endl;
        return YAML::Node();
    }

    YAML::Node result;

	// Helper to convert DeviceRoadmap enums to string
	auto roadmapToString = [](DeviceRoadmap roadmap) -> std::string {
		switch (roadmap) {
			case HP: return "HP";
			case LSTP: return "LSTP";
			case LOP: return "LOP";
			default: return "ULP";
		}
	};

	// Memory cell type
	switch (gCell.memCellType) {
		case SRAM: result["MemoryCell"]["MemoryCellType"] = "SRAM"; break;
		case DRAM: result["MemoryCell"]["MemoryCellType"] = "DRAM"; break;
		case eDRAM: result["MemoryCell"]["MemoryCellType"] = "eDRAM"; break;
		case eDRAM3T: result["MemoryCell"]["MemoryCellType"] = "3T eDRAM"; break;
		case eDRAM3T333: result["MemoryCell"]["MemoryCellType"] = "333 eDRAM"; break;
		case MRAM: result["MemoryCell"]["MemoryCellType"] = "MRAM (Magnetoresistive)"; break;
		case PCRAM: result["MemoryCell"]["MemoryCellType"] = "PCRAM (Phase-Change)"; break;
		case memristor: result["MemoryCell"]["MemoryCellType"] = gCell.isMultiLevel() ? "Multi-Level Cell RRAM (Memristor)" : "RRAM (Memristor)"; break;
		case FBRAM: result["MemoryCell"]["MemoryCellType"] = "FBRAM (Floating Body)"; break;
		case SLCNAND: result["MemoryCell"]["MemoryCellType"] = "Single-Level Cell NAND Flash"; break;
		case MLCNAND: result["MemoryCell"]["MemoryCellType"] = "Multi-Level Cell NAND Flash"; break;
		case CTT: result["MemoryCell"]["MemoryCellType"] = gCell.isMultiLevel() ? "Multi-Level Cell CTT" : "Single-Level Cell CTT"; break;
		case FeFET: result["MemoryCell"]["MemoryCellType"] = gCell.isMultiLevel() ? "Multi-Level Cell FeFET" : "Single-Level Cell FeFET"; break;
		default: result["MemoryCell"]["MemoryCellType"] = "Unknown"; break;
	}

	// Cell area
	result["MemoryCell"]["CellArea_F2"]  = gCell.area;
	result["MemoryCell"]["CellArea_um2"] = gCell.area / 1000000.0 * gTech.featureSizeInNano * gTech.featureSizeInNano;
	result["MemoryCell"]["AspectRatio"]  = gCell.aspectRatio;

	// Resistive / Non-volatile memory
	if (gCell.memCellType == PCRAM || gCell.memCellType == MRAM || gCell.memCellType == memristor ||
		gCell.memCellType == FBRAM || gCell.memCellType == FeFET) {

		if (gCell.resistanceOn < 1e3)
			result["MemoryCell"]["R_on_Ohm"] = gCell.resistanceOn;
		else if (gCell.resistanceOn < 1e6)
			result["MemoryCell"]["R_on_KOhm"] = gCell.resistanceOn / 1e3;
		else
			result["MemoryCell"]["R_on_MOhm"] = gCell.resistanceOn / 1e6;

		if (gCell.resistanceOff < 1e3)
			result["MemoryCell"]["R_off_Ohm"] = gCell.resistanceOff;
		else if (gCell.resistanceOff < 1e6)
			result["MemoryCell"]["R_off_KOhm"] = gCell.resistanceOff / 1e3;
		else
			result["MemoryCell"]["R_off_MOhm"] = gCell.resistanceOff / 1e6;

		result["MemoryCell"]["ReadMode"]  = gCell.readMode ? "Voltage-Sensing" : "Current-Sensing";
		if (gCell.readCurrent > 0) result["MemoryCell"]["ReadCurrent_uA"] = gCell.readCurrent * 1e6;
		if (gCell.readVoltage > 0) result["MemoryCell"]["ReadVoltage_V"] = gCell.readVoltage;

		result["MemoryCell"]["ResetMode"] = gCell.resetMode ? "Voltage" : "Current";
		result["MemoryCell"]["ResetVoltage_V"] = gCell.resetVoltage;
		result["MemoryCell"]["ResetCurrent_uA"] = gCell.resetCurrent * 1e6;
		result["MemoryCell"]["ResetPulse_s"] = gCell.resetPulse / 1e9;

		result["MemoryCell"]["SetMode"] = gCell.setMode ? "Voltage" : "Current";
		result["MemoryCell"]["SetVoltage_V"] = gCell.setVoltage;
		result["MemoryCell"]["SetCurrent_uA"] = gCell.setCurrent * 1e6;
		result["MemoryCell"]["SetPulse_s"] = gCell.setPulse / 1e9;

		switch (gCell.accessType) {
			case CMOS_access: result["MemoryCell"]["AccessType"] = "CMOS"; break;
			case BJT_access: result["MemoryCell"]["AccessType"] = "BJT"; break;
			case diode_access: result["MemoryCell"]["AccessType"] = "Diode"; break;
			default: result["MemoryCell"]["AccessType"] = "None Access Device"; break;
		}
	}

	// SRAM
	if (gCell.memCellType == SRAM) {
		result["MemoryCell"]["WidthAccessCMOS_F"]   = gCell.widthAccessCMOS;
		result["MemoryCell"]["WidthSRAMCellNMOS_F"] = gCell.widthSRAMCellNMOS;
		result["MemoryCell"]["WidthSRAMCellPMOS_F"] = gCell.widthSRAMCellPMOS;
		result["MemoryCell"]["PeripheralRoadmap"]   = roadmapToString(gTech.deviceRoadmap);
		result["MemoryCell"]["PeripheralNode_nm"]   = gTech.featureSizeInNano;
		result["MemoryCell"]["VDD_V"]               = gTech.vdd.value();
		result["MemoryCell"]["Temperature_K"]       = gCell.temperature;
	}

	// DRAM / eDRAM
	if (gCell.memCellType == DRAM || gCell.memCellType == eDRAM) {
		result["MemoryCell"]["WidthAccessCMOS_F"] = gCell.widthAccessCMOS;
		result["MemoryCell"]["PeripheralRoadmap"] = roadmapToString(gTech.deviceRoadmap);
		result["MemoryCell"]["PeripheralNode_nm"] = gTech.featureSizeInNano;
		result["MemoryCell"]["VDD_V"] = gTech.vdd.value();
		result["MemoryCell"]["WL_SWING"] = gTech.vpp.value();
		result["MemoryCell"]["Temperature_K"] = gCell.temperature;
	}

	// 3T DRAM
	if (gCell.memCellType == eDRAM3T || gCell.memCellType == eDRAM3T333) {
		result["MemoryCell"]["WidthWriteAccessCMOS_F"] = gCell.widthAccessCMOS;
		result["MemoryCell"]["WidthReadAccessCMOS_F"]  = gCell.widthAccessCMOSR;
		result["MemoryCell"]["PeripheralRoadmap"]      = roadmapToString(gTech.deviceRoadmap);
		result["MemoryCell"]["WriteAccessRoadmap"]     = roadmapToString(gTechW.deviceRoadmap);
		result["MemoryCell"]["ReadAccessRoadmap"]      = roadmapToString(gTechR.deviceRoadmap);
		result["MemoryCell"]["PeripheralNode_nm"]      = gTech.featureSizeInNano;
		result["MemoryCell"]["WriteAccessNode_nm"]     = gTechW.featureSizeInNano;
		result["MemoryCell"]["ReadAccessNode_nm"]      = gTechR.featureSizeInNano;
		result["MemoryCell"]["VDD_V"]                  = gTech.vdd.value();
		result["MemoryCell"]["WWL_SWING"]              = gTechW.vpp.value();
		result["MemoryCell"]["Temperature_K"]          = gCell.temperature;
	}

	// SLC NAND Flash
	if (gCell.memCellType == SLCNAND) {
		result["MemoryCell"]["PassVoltage_V"]     = gCell.flashPassVoltage;
		result["MemoryCell"]["ProgramVoltage_V"]  = gCell.flashProgramVoltage;
		result["MemoryCell"]["EraseVoltage_V"]    = gCell.flashEraseVoltage;
		result["MemoryCell"]["ProgramTime_s"]     = gCell.flashProgramTime / 1e9;
		result["MemoryCell"]["EraseTime_s"]       = gCell.flashEraseTime / 1e9;
		result["MemoryCell"]["GateCouplingRatio"] = gCell.gateCouplingRatio;
	}

	// Multi-level cells
	if (gCell.isMultiLevel()) {
		result["MemoryCell"]["NumberOfInputFingers"]   = gCell.nFingers;
		result["MemoryCell"]["NumberOfLevelsPerCell"] = gCell.nLvl;
	}

    // Calculate cache metrics
    double cacheHitLatency, cacheMissLatency, cacheWriteLatency;
    double cacheHitDynamicEnergy, cacheMissDynamicEnergy, cacheWriteDynamicEnergy;
    double cacheLeakage;
    double cacheArea;

    if (cacheAccessMode == normal_access_mode) {
        cacheMissLatency = tagResult.bank->timing().latency[mse::cell::Operation::Read].value();
        cacheHitLatency = std::max(tagResult.bank->timing().latency[mse::cell::Operation::Read].value(), bank->mat.timing().latency[mse::cell::Operation::Read].value());
        cacheHitLatency += bank->mat.subarray.timing().columnDecoderLatency.value();
        cacheHitLatency += bank->timing().latency[mse::cell::Operation::Read].value() - bank->mat.timing().latency[mse::cell::Operation::Read].value();
        cacheWriteLatency = std::max(tagResult.bank->timing().latency[mse::cell::Operation::Write].value(), bank->timing().latency[mse::cell::Operation::Write].value());

        cacheMissDynamicEnergy = tagResult.bank->power().dynamicEnergy[mse::cell::Operation::Read].value() + bank->power().dynamicEnergy[mse::cell::Operation::Read].value();
        cacheHitDynamicEnergy = tagResult.bank->power().dynamicEnergy[mse::cell::Operation::Read].value() + bank->power().dynamicEnergy[mse::cell::Operation::Read].value();
        cacheWriteDynamicEnergy = tagResult.bank->power().dynamicEnergy[mse::cell::Operation::Write].value() + bank->power().dynamicEnergy[mse::cell::Operation::Write].value();
    } else if (cacheAccessMode == fast_access_mode) {
        cacheMissLatency = tagResult.bank->timing().latency[mse::cell::Operation::Read].value();
        cacheHitLatency = std::max(tagResult.bank->timing().latency[mse::cell::Operation::Read].value(), bank->timing().latency[mse::cell::Operation::Read].value());
        cacheWriteLatency = std::max(tagResult.bank->timing().latency[mse::cell::Operation::Write].value(), bank->timing().latency[mse::cell::Operation::Write].value());

        cacheMissDynamicEnergy = tagResult.bank->power().dynamicEnergy[mse::cell::Operation::Read].value() + bank->power().dynamicEnergy[mse::cell::Operation::Read].value();
        cacheHitDynamicEnergy = tagResult.bank->power().dynamicEnergy[mse::cell::Operation::Read].value() + bank->power().dynamicEnergy[mse::cell::Operation::Read].value();
        cacheWriteDynamicEnergy = tagResult.bank->power().dynamicEnergy[mse::cell::Operation::Write].value() + bank->power().dynamicEnergy[mse::cell::Operation::Write].value();
    } else {  // sequential_access_mode
        cacheMissLatency = tagResult.bank->timing().latency[mse::cell::Operation::Read].value();
        cacheHitLatency = tagResult.bank->timing().latency[mse::cell::Operation::Read].value() + bank->timing().latency[mse::cell::Operation::Read].value();
        cacheWriteLatency = std::max(tagResult.bank->timing().latency[mse::cell::Operation::Write].value(), bank->timing().latency[mse::cell::Operation::Write].value());

        cacheMissDynamicEnergy = tagResult.bank->power().dynamicEnergy[mse::cell::Operation::Read].value() + bank->power().dynamicEnergy[mse::cell::Operation::Read].value();
        cacheHitDynamicEnergy = tagResult.bank->power().dynamicEnergy[mse::cell::Operation::Read].value() + bank->power().dynamicEnergy[mse::cell::Operation::Read].value();
        cacheWriteDynamicEnergy = tagResult.bank->power().dynamicEnergy[mse::cell::Operation::Write].value() + bank->power().dynamicEnergy[mse::cell::Operation::Write].value();
    }

    cacheLeakage = tagResult.bank->power().leakage.value() + bank->power().leakage.value();
    cacheArea = tagResult.bank->dimensions().area().value() + bank->dimensions().area().value();

    // Generate YAML
    switch (cacheAccessMode) {
        case normal_access_mode: result["CacheDesign"]["AccessMode"] = "Normal"; break;
        case fast_access_mode:   result["CacheDesign"]["AccessMode"] = "Fast"; break;
        default:                 result["CacheDesign"]["AccessMode"] = "Sequential";
    }

	switch (gInputParameter.designTarget) {
		case cache: result["CacheDesign"]["DesignTarget"] = "Cache"; break;
		case RAM_chip: result["CacheDesign"]["DesignTarget"] = "RAMChip"; break;
		case CAM_chip: result["CacheDesign"]["DesignTarget"] = "CAMChip"; break;
		default: result["CacheDesign"]["DesignTarget"] = "Unknown"; break;
	}

	//Capacity
	if (gInputParameter.capacity < 1024) {
		result["Capacity"]["Value"] = gInputParameter.capacity;
		result["Capacity"]["Unit"] = "B";
	} else if (gInputParameter.capacity < 1024 * 1024) {
		result["Capacity"]["Value"] = gInputParameter.capacity / 1024;
		result["Capacity"]["Unit"] = "KB";
	} else if (gInputParameter.capacity < 1024 * 1024 * 1024) {
		result["Capacity"]["Value"] = gInputParameter.capacity / 1024 / 1024;
		result["Capacity"]["Unit"] = "MB";
	} else {
		result["Capacity"]["Value"] = gInputParameter.capacity / 1024 / 1024 / 1024;
		result["Capacity"]["Unit"] = "GB";
	}

    switch (optimizationTarget) {
        case read_latency_optimized: result["CacheDesign"]["OptimizationTarget"] = "ReadLatency"; break;
        case write_latency_optimized: result["CacheDesign"]["OptimizationTarget"] = "WriteLatency"; break;
        case read_energy_optimized: result["CacheDesign"]["OptimizationTarget"] = "ReadDynamicEnergy"; break;
        case write_energy_optimized: result["CacheDesign"]["OptimizationTarget"] = "WriteDynamicEnergy"; break;
		case read_edp_optimized: result["CacheDesign"]["OptimizationTarget"] = "ReadEDP"; break;
		case write_edp_optimized: result["CacheDesign"]["OptimizationTarget"] = "WriteEDP"; break;
		case leakage_optimized: result["CacheDesign"]["OptimizationTarget"] = "LeakagePower"; break;
		case area_optimized: result["CacheDesign"]["OptimizationTarget"] = "Area"; break;
        default:                 result["CacheDesign"]["OptimizationTarget"] = "Unknown";
    }

    result["CacheDesign"]["Area"]["Total_mm2"] = cacheArea * 1e6;
    result["CacheDesign"]["Area"]["DataArray_mm2"] = bank->dimensions().area().value() * 1e6;
    result["CacheDesign"]["Area"]["TagArray_mm2"] = tagResult.bank->dimensions().area().value() * 1e6;

    result["CacheDesign"]["Timing"]["CacheHitLatency_ns"] = cacheHitLatency * 1e9;
    result["CacheDesign"]["Timing"]["CacheMissLatency_ns"] = cacheMissLatency * 1e9;
    result["CacheDesign"]["Timing"]["CacheWriteLatency_ns"] = cacheWriteLatency * 1e9;

    const bool refreshCell = bank->operations().contains(mse::cell::Operation::Refresh) && retentionTime() > 0;
    if (refreshCell) {
        result["CacheDesign"]["Timing"]["CacheRefreshLatency_us"] =
            std::max(tagResult.bank->timing().latency[mse::cell::Operation::Refresh].value(), bank->timing().latency[mse::cell::Operation::Refresh].value()) * 1e6;
        result["CacheDesign"]["Timing"]["CacheAvailability_percent"] =
            ((retentionTime() - std::max(tagResult.bank->timing().latency[mse::cell::Operation::Refresh].value(), bank->timing().latency[mse::cell::Operation::Refresh].value())) /
             retentionTime()) * 100.0;
    }

    result["CacheDesign"]["Power"]["CacheHitDynamicEnergy_nJ"] = cacheHitDynamicEnergy * 1e9;
    result["CacheDesign"]["Power"]["CacheMissDynamicEnergy_nJ"] = cacheMissDynamicEnergy * 1e9;
    result["CacheDesign"]["Power"]["CacheWriteDynamicEnergy_nJ"] = cacheWriteDynamicEnergy * 1e9;

    if (refreshCell) {
        result["CacheDesign"]["Power"]["CacheRefreshDynamicEnergy_nJ"] =
            (tagResult.bank->power().dynamicEnergy[mse::cell::Operation::Refresh].value() + bank->power().dynamicEnergy[mse::cell::Operation::Refresh].value()) * 1e9;
    }

    result["CacheDesign"]["Power"]["CacheTotalLeakagePower_mW"] = cacheLeakage * 1e3;
    result["CacheDesign"]["Power"]["CacheDataArrayLeakagePower_mW"] = bank->power().leakage.value() * 1e3;
    result["CacheDesign"]["Power"]["CacheTagArrayLeakagePower_mW"] = tagResult.bank->power().leakage.value() * 1e3;

    if (refreshCell) {
        result["CacheDesign"]["Power"]["CacheRefreshPower_W"] =
            (bank->power().dynamicEnergy[mse::cell::Operation::Refresh].value() / retentionTime());
        result["CacheDesign"]["Power"]["CacheRetentionTime_ns"] = retentionTime() * 1e9;
    }

    // Add reset and set pulse durations (using global gCell pointer safely)
    //if (gCell) {
        result["CacheDesign"]["Timing"]["Reset"]["PulseDuration_ns"] =
            std::max(gCell.resetPulse, gCell.resetPulse) * 1e9;
        result["CacheDesign"]["Timing"]["Set"]["PulseDuration_ns"] =
            std::max(gCell.setPulse, gCell.setPulse) * 1e9;
    //} else {
        //result["CacheDesign"]["Timing"]["Reset"]["PulseDuration_ns"] = 0;
        //result["CacheDesign"]["Timing"]["Set"]["PulseDuration_ns"] = 0;
    //}

    // Add data and tag details
    result["DataArray"] = toYamlNode();
    result["TagArray"] = tagResult.toYamlNode();

    return result;
}



void Result::printToYamlFile(std::ofstream& outputFile) {
    YAML::Node node = toYamlNode();
    outputFile << node << std::endl;
}

void Result::printAsCacheToYamlFile(Result& tagResult, CacheAccessMode cacheAccessMode, std::ofstream& outputFile) {
    YAML::Node node = toYamlNodeAsCache(tagResult, cacheAccessMode);
    outputFile << node << std::endl;
}

