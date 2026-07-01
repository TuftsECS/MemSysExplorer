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
#include "cmos.hpp"

#include <variant>

namespace mse::cell {

//==================================================
// TODO comment when done

class MemoryCellParameters {
public:

    MemoryCellParameters(YamlInputFile& inputFile) : _inputFile(inputFile) {}

protected:

    template <typename... Ts>
    bool fillParameter(Ts&&... parameters) const noexcept {
        return _inputFile.get().fillParameter(std::forward<Ts>(parameters)...);
    }

private:

    std::reference_wrapper<YamlInputFile> _inputFile;
};

//==================================================
// Each memory cell will have its own intrinsic properties
// These properties are used to inform the system what is required from peripheral components
// Each cell should have its own MemoryCellProperties dervied type and assign these values

struct MemoryCellProperties { // TODO expand properties
    enum class SenseMethod {
        Voltage,
        Current
    };

    SenseMethod sensingMethod;
    unit::Volt sensingVoltage;
    unit::Volt prechargeVoltage;

    bool sharedContact;

    bool isMLC;
    bool columnLeak;
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
    virtual unit::Ohm bitlinePullDownResistance(int bitline, [[maybe_unused]] int path = 0) const noexcept = 0;
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
};

} // namespace mse::cell

#endif
