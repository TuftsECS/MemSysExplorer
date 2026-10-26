#include "component/BankWithoutHtree.hpp"
#include "global.hpp"

#include <cmath>

mse::component::BankWithoutHtree::BankWithoutHtree(
    int matRows,
    int matColumns,
    long long capacity,
    long blockSize,
    int associativity,
    int rowsPerSet,
    int activeMatsPerRow,
    int activeMatsPerColumn,
    int bitlineMuxInputs,
    bool internalSenseAmp,
    int senseAmpMuxLevel1Inputs,
    int senseAmpMuxLevel2Inputs,
    int subarrayRows,
    int subarrayColumns,
    int activeSubarraysPerRow,
    int activeSubarraysPerColumn,
    BufferDesignTarget areaOptimizationLevel,
    MemoryType memoryType)
        : Bank(matRows, matColumns, capacity, blockSize, associativity, rowsPerSet, activeMatsPerRow, activeMatsPerColumn,
               bitlineMuxInputs, internalSenseAmp, senseAmpMuxLevel1Inputs, senseAmpMuxLevel2Inputs, subarrayRows, subarrayColumns,
               activeSubarraysPerRow, activeSubarraysPerColumn, areaOptimizationLevel, memoryType) {}

void mse::component::BankWithoutHtree::forEachSubcomponent(const SubcomponentVisitor& visit) noexcept {
    Bank::forEachSubcomponent(visit);

    auto visitIfPresent = [&visit] (auto& subcomponent) { if (subcomponent) visit(*subcomponent); };

    visitIfPresent(_globalBitlineMux);
    visitIfPresent(_globalSenseAmp);
    visitIfPresent(_globalComparator);
}

bool mse::component::BankWithoutHtree::setup() noexcept {
    using namespace mse::unit;

    if (!internalSenseAmp) {
        if (cell::memoryCell().properties().destructiveRead) {
            warn("DRAM does not support external sense amps");
            return false;
        }
        if (gGlobalWire.wireRepeaterType != repeated_none) {
            return false;
        }
    }

    // Using double to avoid overflow
    const int addressBits = static_cast<int>(std::log2(static_cast<double>(capacity) / blockSize / associativity) + 0.1);
    // Some address bits power gate the inactive mats, the rest are routed to the mats
    const int gatingAddressBits = static_cast<int>(std::log2(this->matRows * this->matColumns / this->activeMatsPerColumn / this->activeMatsPerRow) + 0.1);
    _matAddressBits = addressBits - gatingAddressBits;
    _matDataBits = blockSize;

    MuxInputs muxInputs{this->bitlineMuxInputs, this->senseAmpMuxLevel1Inputs, this->senseAmpMuxLevel2Inputs};
    if (memoryType == dataT) {
        _matDataBits = blockSize / this->activeMatsPerColumn / this->activeMatsPerRow;
        // Not enough ways to spread a set over multiple rows
        if (this->rowsPerSet > associativity) {
            return false;
        }

        _ways = associativity;
        // At least 1, otherwise the bank is already invalid
        muxInputs = muxInputsForWays(_ways / this->rowsPerSet);
    } else if (memoryType == tag) {
        // Tag arrays cannot share bitlines between rows of a set
        if (this->rowsPerSet > 1) {
            return false;
        }

        _ways = associativity / this->activeMatsPerColumn / this->activeMatsPerRow;
        // The mat does not hold at least one way
        if (_ways < 1) {
            return false;
        }
    }

    if (!build(mat, this->subarrayRows, this->subarrayColumns, _matAddressBits, _matDataBits, _ways, this->rowsPerSet,
            this->activeSubarraysPerRow, this->activeSubarraysPerColumn, muxInputs.bitline, internalSenseAmp,
            muxInputs.senseAmpLevel1, muxInputs.senseAmpLevel2, areaOptimizationLevel, memoryType)) {
        return false;
    }

    // The global sense amps are pitch matched to the mats
    if (!internalSenseAmp) {
        const cell::MemoryCellProperties& properties = cell::memoryCell().properties();

        // TODO cells without a resistive global bitline model always sense voltage, even CTT cells in current mode and NAND flash
        bool voltageSense = true;
        if (properties.globalBitline.model == cell::MemoryCellProperties::GlobalBitlineModel::Resistive) {
            voltageSense = properties.sensingMethod == cell::MemoryCellProperties::SenseMethod::Voltage;
        }

        const int senseAmps = memoryType == dataT ? blockSize : blockSize * associativity;
        if (!build("Global", _globalSenseAmp, senseAmps, !voltageSense, cell::memoryCell().parameters().minSenseVoltage.value(), mat.dimensions().width * matColumns / senseAmps,
                properties.multiLevel, properties.senseAmp)) {
            return false;
        }

        if (!build("Global Bitline", _globalBitlineMux, matRows * matColumns / activeMatsPerColumn / activeMatsPerRow, senseAmps,
                _globalSenseAmp->loadCapacitance(), _globalSenseAmp->loadCapacitance(), Ampere(), properties.maxMuxResistance)) {
            return false;
        }

        if (memoryType == tag && !build("Global", _globalComparator, blockSize, Farad())) { // TODO only for testing
            return false;
        }
    }

    return meetsAspectRatio();
}

mse::Dimensions mse::component::BankWithoutHtree::calculateDimensions() const noexcept {
    using namespace mse::unit;

    Meter height = mat.dimensions().height * matRows;
    Meter width = mat.dimensions().width * matColumns;

    // Without repeaters the wires are on another metal layer and take no silicon area
    int wiresSharingWidth = 1;
    Meter effectivePitch;
    if (gGlobalWire.wireRepeaterType != repeated_none) {
        wiresSharingWidth = static_cast<int>(std::floor(gGlobalWire.repeaterSpacing / gGlobalWire.repeaterHeight));
        effectivePitch = Meter(gGlobalWire.repeatedWirePitch);
    }

    width += std::ceil(static_cast<double>(matRows) * matColumns * _matAddressBits / wiresSharingWidth) * effectivePitch;

    if (!internalSenseAmp) {
        height += _globalSenseAmp->dimensions().height;
        height += _globalBitlineMux->dimensions().height;
        if (_globalComparator) {
            height += associativity * _globalComparator->dimensions().area() / width;
        }
    }

    return Dimensions{
        .width = width,
        .height = height
    };
}

mse::component::PathTiming mse::component::BankWithoutHtree::calculateTiming() const noexcept {
    using namespace mse::unit;

    mat.setInputRamp(constant::infiniteRamp);
    const MatTiming& matTiming = mat.timing();

    // The bank's own part of each access, which every operation then adds the mat to
    Second readLatency;
    Second writeLatency;
    Second refreshLatency = matTiming.latency[cell::Operation::Refresh] * matColumns; // Every mat

    // Only the wire to the nearest mat adds latency
    const double matHeight = mat.dimensions().height.value();
    const double wireLength = matHeight * (matRows + 1) - matHeight;
    if (internalSenseAmp) {
        double latency;
        double energy;
        double leakage;
        gGlobalWire.CalculateLatencyAndPower(wireLength, &latency, &energy, &leakage);
        readLatency += Second(latency);
        writeLatency += Second(latency);
        refreshLatency += Second(latency);
    } else {
        const SubArrayRC& subarrayRC = mat.subarray.rc();
        const double muxResistance = _globalBitlineMux->rc().passResistance.value();
        const double muxCapacitance = _globalBitlineMux->rc().passCapacitance.value();
        double localBitlineResistance = subarrayRC.bitlineResistance.value() + 3 * muxResistance;
        const double localBitlineCapacitance = subarrayRC.bitlineCapacitance.value() + 6 * muxCapacitance;
        const double globalBitlineResistance = wireLength * gGlobalWire.resWirePerUnit;
        const double globalBitlineCapacitance = wireLength * gGlobalWire.capWirePerUnit;
        const double globalMuxCapacitance = _globalBitlineMux->rc().delayLoadCapacitance.value();
        const double senseVoltage = _globalSenseAmp->senseVoltage.value();

        const cell::MemoryCellProperties& properties = cell::memoryCell().properties();
        const cell::MemoryCellProperties::GlobalBitline& globalBitline = properties.globalBitline;

        // TODO only SRAM and resistive cells have a global bitline model, other cells add no global bitline latency
        if (globalBitline.model == cell::MemoryCellProperties::GlobalBitlineModel::Sram) {
            // Should equal the reset and set voltages for SRAM
            const double prechargeVoltage = globalBitline.readVoltage.value();
            double latency = localBitlineResistance * globalBitlineCapacitance / 2
                    + (localBitlineResistance + globalBitlineResistance) * (globalBitlineCapacitance / 2 + globalMuxCapacitance);
            latency *= std::log(prechargeVoltage / (prechargeVoltage - senseVoltage));
            latency += localBitlineResistance * globalBitlineCapacitance / 2;
            latency += _globalBitlineMux->timing().readLatency.value();
            writeLatency += Second(latency);
            latency += _globalSenseAmp->timing().readLatency.value();
            readLatency += Second(latency);
        } else if (globalBitline.model == cell::MemoryCellProperties::GlobalBitlineModel::Resistive) {
            double tau = muxResistance * globalBitlineCapacitance / 2
                    + (muxResistance + globalBitlineResistance) * (globalBitlineCapacitance + localBitlineCapacitance) / 2
                    + (muxResistance + globalBitlineResistance + localBitlineResistance) * localBitlineCapacitance / 2;
            writeLatency += Second(0.63 * tau);

            if (properties.sensingMethod == cell::MemoryCellProperties::SenseMethod::Current) { // Current sensing, ICCAD 2009 model
                localBitlineResistance += subarrayRC.cellOffResistance.value();
                tau = globalBitlineResistance * globalBitlineCapacitance / 2
                        * (localBitlineResistance + globalBitlineResistance / 3) / (localBitlineResistance + globalBitlineResistance);
                readLatency += Second(0.63 * tau);
            } else { // Voltage sensing
                const double prechargeVoltage = subarrayRC.prechargeVoltage.value();
                const double onVoltage = subarrayRC.cellOnVoltage.value();
                const double offVoltage = subarrayRC.cellOffVoltage.value();
                // Current-in sensing uses the cell resistances, voltage divider sensing uses the equivalent resistances
                const bool currentInput = globalBitline.readVoltage.value() == 0;
                if (currentInput) {
                    localBitlineResistance += subarrayRC.cellOnResistance.value();
                } else {
                    localBitlineResistance += subarrayRC.equivalentOnResistance.value();
                }
                tau = localBitlineResistance * globalBitlineCapacitance + (localBitlineResistance + globalBitlineResistance) * globalBitlineCapacitance / 2;
                const double onLatency = tau * std::log((prechargeVoltage - onVoltage) / (prechargeVoltage - onVoltage - senseVoltage));
                if (currentInput) {
                    localBitlineResistance += (globalBitline.resistanceOff - globalBitline.resistanceOn).value();
                } else {
                    localBitlineResistance += subarrayRC.equivalentOffResistance.value() - subarrayRC.equivalentOnResistance.value();
                }
                tau = localBitlineResistance * globalBitlineCapacitance + (localBitlineResistance + globalBitlineResistance) * globalBitlineCapacitance / 2;
                const double offLatency = tau * std::log((offVoltage - prechargeVoltage) / (offVoltage - prechargeVoltage - senseVoltage));

                // Replace the subarray bitline latency with the slower of the combined on and off paths
                const SubArrayTiming& subarrayTiming = mat.subarray.timing();
                readLatency -= subarrayTiming.bitlineLatency;
                if (onLatency + subarrayTiming.bitlineOnLatency.value() > offLatency + subarrayTiming.bitlineOffLatency.value()) {
                    readLatency += Second(onLatency + subarrayTiming.bitlineOnLatency.value());
                } else {
                    readLatency += Second(offLatency + subarrayTiming.bitlineOffLatency.value());
                }
            }
        }
    }

    if (!internalSenseAmp) {
        readLatency += _globalBitlineMux->timing().readLatency + _globalSenseAmp->timing().readLatency;
        writeLatency += _globalBitlineMux->timing().writeLatency + _globalSenseAmp->timing().writeLatency;
        if (_globalComparator) {
            _globalComparator->setInputRamp(physics::HorowitzRamp(1e40));
            readLatency += _globalComparator->timing().readLatency;
        }
    }

    const cell::OperationSet supported = operations();
    PathTiming timing;
    for (cell::Operation operation : cell::allOperations) {
        if (!supported.contains(operation)) {
            continue;
        }

        switch (operation) {
            case cell::Operation::Read:
                timing.latency[operation] = readLatency + matTiming.latency[operation];
                break;
            case cell::Operation::Write:
            case cell::Operation::Set:
            case cell::Operation::Reset:
                timing.latency[operation] = writeLatency + matTiming.latency[operation];
                break;
            case cell::Operation::Refresh:
                timing.latency[operation] = refreshLatency;
                break;
        }
    }

    return timing;
}

mse::component::PathPower mse::component::BankWithoutHtree::calculatePower() const noexcept {
    using namespace mse::unit;

    const PathPower& matPower = mat.power();

    // The bank's own part of each access, which every operation then adds the mats to
    Joule readEnergy;
    Joule writeEnergy;
    Joule refreshEnergy = matPower.dynamicEnergy[cell::Operation::Refresh] * matRows * matColumns; // Every mat

    PathPower power;

    const cell::MemoryCellProperties& properties = cell::memoryCell().properties();
    const cell::MemoryCellProperties::GlobalBitline& globalBitline = properties.globalBitline;

    const double matHeight = mat.dimensions().height.value();
    double wireLength = matHeight * (matRows + 1);
    for (int i = 0; i < matRows; i++) {
        wireLength -= matHeight;
        if (internalSenseAmp) {
            double latency;
            double energy;
            double leakage;
            gGlobalWire.CalculateLatencyAndPower(wireLength, &latency, &energy, &leakage);
            double routedBits = 0;
            if (i < activeMatsPerColumn) {
                routedBits = _matAddressBits + _matDataBits;
                if (memoryType == tag) {
                    routedBits += _ways;
                }
                readEnergy += Joule(energy * routedBits * activeMatsPerRow);
                writeEnergy += Joule(energy * routedBits * activeMatsPerRow);
                refreshEnergy += Joule(energy * routedBits * activeMatsPerRow);
            }
            power.leakage += Watt(leakage * routedBits * matColumns);
        } else if (i < activeMatsPerColumn) {
            const double globalBitlineCapacitance = wireLength * gGlobalWire.capWirePerUnit;
            const double addressEnergy = globalBitlineCapacitance * gTech.vdd.value() * gTech.vdd.value() * _matAddressBits;
            // TODO only SRAM and resistive cells have a global bitline model, other cells add no global bitline energy
            if (globalBitline.model == cell::MemoryCellProperties::GlobalBitlineModel::Sram) {
                // Should equal the reset and set voltages for SRAM
                const double prechargeVoltage = globalBitline.readVoltage.value();
                readEnergy += Joule(addressEnergy);
                writeEnergy += Joule(addressEnergy);
                readEnergy += Joule(globalBitlineCapacitance * prechargeVoltage * prechargeVoltage * _ways);
                writeEnergy += Joule(globalBitlineCapacitance * prechargeVoltage * prechargeVoltage * _matDataBits);
            } else if (globalBitline.model == cell::MemoryCellProperties::GlobalBitlineModel::Resistive) {
                const double writeVoltage = globalBitline.writeVoltage.value();
                readEnergy += Joule(addressEnergy);
                writeEnergy += Joule(addressEnergy);
                writeEnergy += Joule(globalBitlineCapacitance * writeVoltage * writeVoltage * _matDataBits);
                // Voltage-in voltage sensing
                if (properties.sensingMethod == cell::MemoryCellProperties::SenseMethod::Voltage) {
                    const double prechargeVoltage = mat.subarray.rc().prechargeVoltage.value();
                    const double onVoltage = mat.subarray.rc().cellOnVoltage.value();
                    readEnergy += Joule(globalBitlineCapacitance * (prechargeVoltage * prechargeVoltage - onVoltage * onVoltage) * _matDataBits);
                }
            }
        }
    }

    if (!internalSenseAmp) {
        readEnergy += (_globalBitlineMux->power().dynamicReadEnergy + _globalSenseAmp->power().dynamicReadEnergy) * activeMatsPerRow;
        writeEnergy += (_globalBitlineMux->power().dynamicWriteEnergy + _globalSenseAmp->power().dynamicWriteEnergy) * activeMatsPerRow;
        power.leakage += (_globalBitlineMux->power().leakage + _globalSenseAmp->power().leakage) * matColumns;
        if (_globalComparator) {
            readEnergy += _ways * _globalComparator->power().dynamicReadEnergy;
            power.leakage += associativity * _globalComparator->power().leakage;
        }
    }

    // A fast access cache only activates 1 / associativity of the wires on a write
    if (gInputParameter.designTarget == cache && gInputParameter.cacheAccessMode == fast_access_mode) {
        writeEnergy /= gInputParameter.associativity;
    }

    const cell::OperationSet supported = operations();
    for (cell::Operation operation : cell::allOperations) {
        if (!supported.contains(operation)) {
            continue;
        }

        switch (operation) {
            case cell::Operation::Read:
                power.dynamicEnergy[operation] = readEnergy + matPower.dynamicEnergy[operation] * activeMatsPerRow * activeMatsPerColumn;
                break;
            case cell::Operation::Write:
            case cell::Operation::Set:
            case cell::Operation::Reset:
                power.dynamicEnergy[operation] = writeEnergy + matPower.dynamicEnergy[operation] * activeMatsPerRow * activeMatsPerColumn;
                break;
            case cell::Operation::Refresh:
                power.dynamicEnergy[operation] = refreshEnergy;
                break;
        }
    }
    power.leakage += matPower.leakage * matRows * matColumns;

    return power;
}
