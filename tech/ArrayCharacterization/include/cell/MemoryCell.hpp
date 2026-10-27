// This file declares the core types each cell should implement to have common interfaces
// Each cell needs a set of parameters, a list of its properties, and a topology
// MemoryCellParameters is used as a general type to load parameters from input files per cell
// MemoryCellProperties lists instrinsic values of each cell type that influence system design
// MemoryCellTopology holds all related information about physical cell implementation
// Together, these all combine in the MemoryCell type to complete a cell definition
// Then the cell can be used with general interfaces throughout the program

#ifndef MSE_CELL_MEMORYCELL_HPP
#define MSE_CELL_MEMORYCELL_HPP

#include "unit.hpp"
#include "cmosDevice.hpp"
#include "input.hpp"
#include "typedef.hpp"
#include "cell/Operation.hpp"

#include <array>
#include <cmath>
#include <functional>
#include <optional>
#include <ostream>
#include <span>
#include <vector>

namespace mse::cell {

//==================================================
// Parameters every cell reads from its cell file
// Derived parameters add the keys of their own cell type and fill them with fillParameter

class MemoryCellParameters {
public:

    ScalarParameter<double> area{"Area", "", 0.0}; // Unit: F^2
    ScalarParameter<double> aspectRatio{"AspectRatio", "", 0.0}; // Height over width
    ScalarParameter<unit::Volt> minSenseVoltage{"MinSenseVoltage", "", unit::Volt(0.08)};
    ScalarParameter<CellAccessType> accessType{"Access.Type", "", CMOS_access};
    ScalarParameter<int> stitching{"Stitching", "", 0}; // Adds stitching overhead to the wordline every this many cells when nonzero

    unit::Feature cellWidth;
    unit::Feature cellHeight;

    MemoryCellParameters(YamlInputFile& inputFile) : _inputFile(inputFile) {
        fillParameter(area, aspectRatio, minSenseVoltage, accessType, stitching);
        if (aspectRatio.value() != 0) {
            cellHeight = unit::Feature(std::sqrt(area.value() * aspectRatio.value()));
            cellWidth = unit::Feature(std::sqrt(area.value() / aspectRatio.value()));
        }
    }

protected:

    template <typename... Ts>
    bool fillParameter(Ts&... parameters) const noexcept {
        return _inputFile.get().fillParameter(parameters...);
    }

private:

    std::reference_wrapper<YamlInputFile> _inputFile;
};

//==================================================
// Cells that store more than one bit, sensed with a reference for each level

struct MultiLevel {
    double levels = 2;
    int senseAmpFingers = 8;
};

// Parameters of cell types that can store more than one bit
// Cells with more than 2 levels are multi-level, read from MultiLevel in the cell file

struct MultiLevelParameters {
    ScalarParameter<double> levels{"MultiLevel.Levels", "", 2.0};
    ScalarParameter<int> senseAmpFingers{"MultiLevel.SenseAmpFingers", "", 8};

    // The cell's levels, nullopt when it stores one bit
    std::optional<MultiLevel> multiLevel() const noexcept {
        if (!(levels.value() > 2)) {
            return std::nullopt;
        }

        return MultiLevel{levels.value(), senseAmpFingers.value()};
    }
};

// How the sense amps reading a cell are modeled

struct SenseAmpModel {
    // Multi-level CTT and RRAM sense amps use a model fitted for their fingers
    bool fittedMultiLevel = false;
    // Values measured for the cell's sense amps, which replace the general model
    std::optional<unit::Second> readLatency;
    std::optional<unit::Joule> readEnergy;
};

//==================================================
// Each memory cell will have its own intrinsic properties
// These properties are used to inform the system what is required from peripheral components
// Each cell should have its own MemoryCellProperties dervied type and assign these values
// The array reads these instead of checking which cell type it holds

struct MemoryCellProperties {
    enum class SenseMethod {
        Voltage,
        Current
    };

    // The bank's models of the global bitlines between the mats and external sense amps
    enum class GlobalBitlineModel {
        None, // Adds no global bitline latency or energy
        Sram,
        Resistive
    };

    // The cell values a global bitline model reads
    struct GlobalBitline {
        GlobalBitlineModel model = GlobalBitlineModel::None;
        // The precharge voltage for SRAM, and the read voltage for resistive cells where 0 means current-in sensing
        unit::Volt readVoltage;
        // The larger of the set and reset voltages of resistive cells
        unit::Volt writeVoltage;
        unit::Ohm resistanceOn;
        unit::Ohm resistanceOff;
    };

    // The operations the cell has values for, which are the only ones computed along its access path
    OperationSet operations;

    SenseMethod sensingMethod = SenseMethod::Voltage;
    unit::Volt prechargeVoltage;

    bool sharedContact = false;
    bool columnLeak = false;
    std::optional<unit::Volt> leakageVoltage; // Supply the cells leak from, the peripheral vdd if empty

    // Only set for cells storing more than one bit
    std::optional<MultiLevel> multiLevel;
    SenseAmpModel senseAmp;

    // Reading drains the stored charge, so the bitlines can't be muxed before the sense amps
    bool destructiveRead = false;
    // The sense amps don't fit the pitch of one column, so a bitline mux has to share them
    bool senseAmpNeedsBitlineMux = false;
    // Wordlines are driven to vpp instead of vdd
    bool boostedWordline = false;
    // The cell array is stacked over the peripherals, so the peripherals only add area where they are larger
    bool stackedOverPeripherals = false;
    // Reads use their own wordline, the first one, which has its own row decoder, and writes use the second
    bool separateReadWordline = false;
    // Cells sit directly between their wordline and bitline without an access transistor
    bool crosspoint = false;
    // The fewest rows a subarray of these cells can have
    long long minimumRows = 0;
    // NAND flash, where RAM chips access a page at a time, a reset erases a block and a set programs a page
    bool flash = false;
    // Set and reset switch the cell the same way, so a write reports the reset pulse and energy as its own
    bool symmetricWrite = false;
    // The operations whose energy includes the current through the cell itself
    OperationSet cellEnergyOperations;

    // How long cells that refresh keep their data
    std::optional<unit::Second> retentionTime;
    // Mux pass transistors stay below this resistance so reads can divide the voltage with the cell
    std::optional<unit::Ohm> maxMuxResistance;
    GlobalBitline globalBitline;

    MemoryCellProperties(const MemoryCellParameters& parameters)
            : crosspoint(parameters.accessType == diode_access || parameters.accessType == none_access) {}
};

//==================================================
// Each cell needs to have a MemoryCellTopology type defining aspects of its physical structure
// This general topology interface lets cells have varying amounts of bitlines and wordlines
// Adapter subtypes let wordline and bitline devices be generalized too, making topologies very flexible
// Functions in this interface are meant to abstract away internal workings of each cell

class MemoryCellTopology {
public:

    virtual ~MemoryCellTopology() = default;

    // Simple functions finding the number of wordlines or bitlines for a cell
    size_t wordlineCount() const { return wordlineControlledDevices().size(); }
    size_t bitlineCount() const { return bitlineAccessDevices().size(); }

    // Calculates the total wordline capacitance for some wordline of a cell
    unit::Farad wordlineCapacitance(int wordline) const noexcept {
        unit::Farad totalCapacitance;
        for (const WordlineControlledDevice& device : wordlineControlledDevices()[wordline]) {
            totalCapacitance += device.wordlineCapacitance();
        }
        return totalCapacitance;
    }

    // Calculates the total bitline capacitance for some bitline of a cell
    unit::Farad bitlineCapacitance(int bitline) const noexcept {
        unit::Farad totalCapacitance;
        for (const BitlineAccessDevice& device : bitlineAccessDevices()[bitline]) {
            totalCapacitance += device.bitlineCapacitance();
        }
        return totalCapacitance;
    }

    // Calculations needed for other operations that should be implementable by all topologies
    //virtual unit::Ohm bitlinePullDownResistance(int bitline, [[maybe_unused]] int path = 0) const noexcept = 0;
    virtual unit::Ampere leakageCurrent() const noexcept = 0;

protected:
    
    //==================================================
    // WordlineControlledDevice and BitlineAccessDevice are interfaces for adapter types
    // Adapters for types that can be used as wordline controlled devices inherit WordlineControlledDevice
    //   Same for BitlineAccessDevice
    // Each adapter then implements the interface in terms of its adaptee type
    // This way, adaptee types aren't forced to implement this context heavy interface as a base type
    // Keeps the interface separate and isolates responsibilities of the adaptee type

    struct WordlineControlledDevice {
        virtual unit::Farad wordlineCapacitance() const noexcept = 0;
    };

    struct BitlineAccessDevice {
        virtual unit::Farad bitlineCapacitance() const noexcept = 0;
    };

    //==================================================
    // TODO recomment // CmosTransistor types can be used as wordline controlled devices or bitline access devices

    template <typename T>
    class CellLineAdapter;

    //==================================================
    // These types are for use in derived topologies to implement interface functions
    // WordlineList is a jagged array of references to wordline devices for each wordline
    // BitlineList is the same but for bitline devices
    // Each ...Span version is the return type used for the topology interface functions
    //   Which is needed because spans hold references to std::arrays of any size

    template <int N>
    using WordlineList = std::array<std::vector<std::reference_wrapper<const WordlineControlledDevice>>, N>;
    using WordlineListSpan = std::span<const std::vector<std::reference_wrapper<const WordlineControlledDevice>>>;

    template <int N>
    using BitlineList = std::array<std::vector<std::reference_wrapper<const BitlineAccessDevice>>, N>;
    using BitlineListSpan = std::span<const std::vector<std::reference_wrapper<const BitlineAccessDevice>>>;

    virtual WordlineListSpan wordlineControlledDevices() const noexcept = 0;
    virtual BitlineListSpan bitlineAccessDevices() const noexcept = 0;
};

template <>
class MemoryCellTopology::CellLineAdapter<CmosTransistor> final : public WordlineControlledDevice, public BitlineAccessDevice {
public:

    CellLineAdapter(const CmosTransistor& adaptee) : _adaptee(adaptee) {}
    unit::Farad wordlineCapacitance() const noexcept override { return _adaptee.get().gateCapacitance(); }
    unit::Farad bitlineCapacitance() const noexcept override { return _adaptee.get().drainCapacitance(); }

private:

    std::reference_wrapper<const CmosTransistor> _adaptee;
};

template <>
class MemoryCellTopology::CellLineAdapter<unit::Farad> final : public WordlineControlledDevice, public BitlineAccessDevice {
public:

    CellLineAdapter(unit::Farad value) : _value(value) {}
    unit::Farad wordlineCapacitance() const noexcept override { return _value; }
    unit::Farad bitlineCapacitance() const noexcept override { return _value; }

private:

    unit::Farad _value;
};

// TODO comment
// this is here until node system is added i guess

struct MemoryCellLatencyContext {
    unit::Farad bitlineCapacitance;
    unit::Ohm bitlineResistance;
    unit::Farad bitlineMuxCapacitance;
    unit::SecondExp<-1> rowDecoderRampOutput;
    unit::Second bitlineMuxReadLatency;
    unit::Second rowDecoderReadLatency;
    unit::Second rowDecoderWriteLatency;
    unit::SecondExp<-1> readRowDecoderRampOutput; // Cells with a separate read wordline
    unit::Second readDecoderLatency;
    unit::Second writeDriverLatency;
    unit::Second columnDecoderLatency;
    unit::Second chargeLatency;
    unit::Second senseAmpReadLatency;
    unit::Second senseAmpMuxLev1ReadLatency;
    unit::Second senseAmpMuxLev2ReadLatency;
    unit::Volt senseVoltage;
    int numColumn;
    int numRow;
};

struct MemoryCellEnergyContext {
    unit::Farad bitlineCapacitance;
    unit::Ohm bitlineResistance;
    unit::Farad bitlineMuxPreviousPowerCapacitance;
    unit::Ohm bitlineMuxResistance;
    unit::Volt senseVoltage;
    unit::Second senseAmpReadLatency;
    unit::Joule rowDecoderReadDynamic;
    int numRow;
    int numColumn;
    int muxSenseAmp;
    int muxOutputLev1;
    int muxOutputLev2;
};

//==================================================
// The cell's part of each operation, returned by MemoryCell::timing and MemoryCell::power
// Only the entries for the cell's supported operations are filled, the rest stay zero

struct CellTiming {
    OperationValues<unit::Second> latency;

    // Bitline delays kept for output and external sensing
    // Cells with separate read and write bitlines fill bitlineRead and bitlineWrite, the rest fill bitline, which stands in for both
    unit::Second bitline;
    std::optional<unit::Second> bitlineRead;
    std::optional<unit::Second> bitlineWrite;
    unit::Second bitlineOn;
    unit::Second bitlineOff;
};

struct CellPower {
    // Energy of the bitlines and lines the cell drives for each operation
    OperationValues<unit::Joule> dynamicEnergy;
    // Energy spent in the cells themselves, like the current through a resistive cell while it switches
    OperationValues<unit::Joule> cellEnergy;
    // Cells that drive wordlines past vdd replace the row decoder energy of each operation
    std::optional<OperationValues<unit::Joule>> rowDecoderEnergy;
};

//==================================================
// The largest currents an array's bitlines and wordlines carry for its cells

struct ArrayCurrents {
    unit::Ampere maxBitlineCurrent;
    unit::Ampere maxWordlineCurrent;
};

//==================================================
// Main interface for assembling cell types
// Each cell should derive from this and implement the virtual functions to pass up subtypes

struct MemoryCell {
    virtual ~MemoryCell() = default;

    // TODO this is here because of the adapter members in derived topologies
    MemoryCell() = default;
    MemoryCell(const MemoryCell&) = delete;
    MemoryCell(MemoryCell&&) = delete;
    MemoryCell& operator=(const MemoryCell&) = delete;
    MemoryCell& operator=(MemoryCell&&) = delete;

    virtual const MemoryCellTopology& topology() const noexcept = 0;
    virtual const MemoryCellProperties& properties() const noexcept = 0;
    virtual const MemoryCellParameters& parameters() const noexcept = 0;

    // The cell's latency and energy for each of its supported operations
    // These are split like component timing and power since the energy needs latencies the timing finds
    virtual CellTiming timing(const MemoryCellLatencyContext& context) const noexcept = 0;
    virtual CellPower power(const MemoryCellEnergyContext& context) const noexcept = 0;

    // The voltage the sense amps see on a bitline with the given capacitance
    // The array is too large when it is below the cell's minimum sense voltage
    virtual unit::Volt senseVoltage([[maybe_unused]] unit::Farad bitlineCapacitance) const noexcept {
        return parameters().minSenseVoltage;
    }

    // The currents the array's lines carry, nullopt when leakage or half selected cells make the array too large
    // selectedColumns is the number of columns accessed at once
    virtual std::optional<ArrayCurrents> arrayCurrents([[maybe_unused]] long long rows, [[maybe_unused]] long long columns,
            [[maybe_unused]] int selectedColumns) const noexcept {
        return ArrayCurrents();
    }

    // Bitline length the cell adds to an array besides the cells themselves, like select transistors
    virtual unit::Meter bitlineOverhead([[maybe_unused]] long long rows) const noexcept {
        return unit::Meter();
    }

    // The cell's own lines in the cell summary printed before the search
    virtual void printSummary([[maybe_unused]] std::ostream& stream) const {}

    // The cell's own values in the MemoryCell section of the result YAML
    virtual void writeSummary([[maybe_unused]] YAML::Node node) const {}
};

} // namespace mse::cell

#endif
