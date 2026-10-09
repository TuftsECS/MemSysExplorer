#include "component/SubArray.hpp"
#include "formula.hpp"
#include "global.hpp"

#include <cmath>

mse::component::SubArray::SubArray(long long rows, long long columns, bool multipleRowsPerSet, int bitlineMuxInputs, bool internalSenseAmp, int senseAmpMuxLevel1Inputs, int senseAmpMuxLevel2Inputs, BufferDesignTarget areaOptimizationLevel)
        : _multipleRowsPerSet(multipleRowsPerSet),
          _bitlineMuxInputs(bitlineMuxInputs),
          _internalSenseAmp(internalSenseAmp),
          _senseAmpMuxLevel1Inputs(senseAmpMuxLevel1Inputs),
          _senseAmpMuxLevel2Inputs(senseAmpMuxLevel2Inputs),
          _areaOptimizationLevel(areaOptimizationLevel),
          rows(rows),
          columns(columns) {}

void mse::component::SubArray::forEachSubcomponent(const SubcomponentVisitor& visit) noexcept {
    visit(rowDecoder);
    visit(bitlineMuxDecoder);
    visit(senseAmpMuxLevel1Decoder);
    visit(senseAmpMuxLevel2Decoder);
    if (senseAmp) {
        visit(*senseAmp);
    }
    visit(precharger);
    visit(bitlineMux);
    visit(senseAmpMuxLevel1);
    visit(senseAmpMuxLevel2);
    visit(buffer);
}

bool mse::component::SubArray::setup() noexcept {
    using namespace mse::unit;

    const Number finFactor = gTech.featureSize <= 14*1e-9 ? 2 : 1;
    const Unit<MeterAtom<>, FeatureAtom<-1>> featureSize{gTech.featureSize};
    const int temperatureIndex = gInputParameter.temperature - 300;

    const bool dramCell = gCell.memCellType == DRAM || gCell.memCellType == eDRAM
            || gCell.memCellType == eDRAM3T || gCell.memCellType == eDRAM3T333;
    const bool resistiveCell = gCell.memCellType == MRAM || gCell.memCellType == PCRAM || gCell.memCellType == memristor
            || gCell.memCellType == FeFET || gCell.memCellType == MLCFeFET || gCell.memCellType == MLCRRAM;

    // Each column holds part of the data in all the ways
    if (gInputParameter.designTarget == cache && gInputParameter.cacheAccessMode != sequential_access_mode
            && columns < gInputParameter.associativity) {
        return false;
    }

    // DRAM does not allow muxed bitlines because of its destructive readout
    if (dramCell && _bitlineMuxInputs > 1) {
        return false;
    }

    if (gCell.memCellType == SLCNAND && rows < gInputParameter.flashBlockSize / gInputParameter.pageSize) {
        return false;
    }

    // There is no room for the sense amp without a bitline mux
    const bool needsBitlineMux = gCell.memCellType == SLCNAND || gCell.memCellType == memristor || gCell.memCellType == FBRAM
            || gCell.memCellType == FeFET || gCell.memCellType == MLCFeFET || gCell.memCellType == MLCRRAM;
    if (needsBitlineMux && _internalSenseAmp && _bitlineMuxInputs < 2) {
        return false;
    }

    Ampere maxWordlineCurrent;
    if (gCell.memCellType == FBRAM) {
        // Bitline too long
        if (gCell.resistanceOff / gCell.resistanceOn < rows / constant::bitlineLeakageTolerance) {
            return false;
        }
        _maxBitlineCurrent = Ampere(std::max(gCell.resetCurrent, gCell.setCurrent) + gCell.leakageCurrentAccessDevice * (rows - 1));
    }

    if (resistiveCell) {
        if (gCell.accessType == CMOS_access) {
            // Bitline too long
            if (gTech.currentOnNmos[temperatureIndex] / gTech.currentOffNmos[temperatureIndex] < rows / constant::bitlineLeakageTolerance) {
                return false;
            }
            _maxBitlineCurrent = Ampere(std::max(gCell.resetCurrent, gCell.setCurrent) + gCell.leakageCurrentAccessDevice * (rows - 1));
        } else {
            // Write half select limits the array size
            Ampere resetCurrent(gCell.resetCurrent);
            if (gCell.resetCurrent == 0) {
                resetCurrent = Ampere((std::fabs(gCell.resetVoltage) - gCell.voltageDropAccessDevice) / gCell.resistanceOnAtResetVoltage);
            }

            const int selectedColumns = columns / _bitlineMuxInputs / _senseAmpMuxLevel1Inputs / _senseAmpMuxLevel2Inputs;
            if (gCell.accessType == none_access) {
                maxWordlineCurrent = resetCurrent * selectedColumns + resetCurrent * gCell.resistanceOnAtResetVoltage
                        / 2 / gCell.resistanceOnAtHalfResetVoltage * (columns - selectedColumns);
            } else { // Diode or BJT
                maxWordlineCurrent = resetCurrent * selectedColumns + Ampere(gCell.leakageCurrentAccessDevice)
                        * (columns - selectedColumns);
            }

            const Meter minWordlineDriverWidth(maxWordlineCurrent.value() / gTech.currentOnNmos[temperatureIndex]);
            if (minWordlineDriverWidth > Feature(gInputParameter.maxNmosSize) * featureSize) {
                return false;
            }

            if (gCell.accessType == none_access) {
                _maxBitlineCurrent = resetCurrent + resetCurrent * gCell.resistanceOnAtResetVoltage / 2
                        / gCell.resistanceOnAtHalfResetVoltage * (rows - 1);
            } else { // Diode or BJT
                _maxBitlineCurrent = resetCurrent + Ampere(gCell.leakageCurrentAccessDevice) * (rows - 1);
            }
        }
    }

    const Meter minBitlineMuxWidth = std::max(constant::minNmosSize * featureSize,
            Meter(_maxBitlineCurrent.value() / gTech.currentOnNmos[temperatureIndex]));
    if (minBitlineMuxWidth > Feature(gInputParameter.maxNmosSize) * featureSize) {
        return false;
    }

    if (_internalSenseAmp) {
        if (resistiveCell || gCell.memCellType == FBRAM || gCell.memCellType == CTT || gCell.memCellType == MLCCTT) {
            _voltageSense = gCell.readMode;
        } else { // SRAM, DRAM, eDRAM and NAND flash
            _voltageSense = true;
        }
    } else if (dramCell) {
        fatal("DRAM does not support external sense amplifiers");
    }

    _wordlineLength = columns * Feature(gCell.widthInFeatureSize) * featureSize;
    _bitlineLength = rows * Feature(gCell.heightInFeatureSize) * featureSize;
    if (gCell.stitching) {
        _wordlineLength += ((columns - 1) / gCell.stitching + 1) * constant::stitchingOverhead * featureSize;
    }
    // Two select transistors including contacts are 5F long in total
    if (gCell.memCellType == SLCNAND) {
        const int pageCount = gInputParameter.flashBlockSize / gInputParameter.pageSize;
        _bitlineLength += (rows / pageCount) * 5_feat * featureSize;
    }
    // Access transistor
    if (gCell.memCellType == CTT || gCell.memCellType == MLCCTT) {
        _bitlineLength += 5_feat * featureSize;
    }

    const Unit<FaradAtom<>, MeterAtom<-1>> wireCapacitance{gLocalWire.capWirePerUnit};
    const Unit<OhmAtom<>, MeterAtom<-1>> wireResistance{gLocalWire.resWirePerUnit};
    _rc.wordlineCapacitance = _wordlineLength * wireCapacitance;
    _rc.wordlineResistance = _wordlineLength * wireResistance;
    _rc.bitlineCapacitance = _bitlineLength * wireCapacitance;
    _rc.bitlineResistance = _bitlineLength * wireResistance;

    const Ohm muxDecoderLoadResistance = _rc.wordlineResistance;
    Farad muxDecoderLoadCapacitance = Farad(CalculateGateCap(minBitlineMuxWidth.value(), gTech)) * columns;
    muxDecoderLoadCapacitance += _rc.wordlineCapacitance;

    if (gCell.memCellType == DRAM || gCell.memCellType == eDRAM) {
        _senseVoltage = Volt(gTech.vdd / 2 * gCell.capDRAMCell / (gCell.capDRAMCell + _rc.bitlineCapacitance.value()));
        // Bitline too long
        if (_senseVoltage < Volt(gCell.minSenseVoltage)) {
            return false;
        }
    } else if (gCell.memCellType == SLCNAND) {
        // With a 0.5Vdd reference and a 0.6Vdd initial bitline, the sense amp can tell the data apart once the bitline drops to 0.4Vdd
        _senseVoltage = Volt(std::max(gCell.minSenseVoltage, 0.2 * gTech.vdd));
    } else {
        // TODO different memory technologies might have different values here
        _senseVoltage = Volt(gCell.minSenseVoltage);
    }

    const double cellWidth = gCell.widthInFeatureSize * gTech.featureSize;
    if (gCell.memCellType == SRAM) {
        // Two access transistors
        const double accessWidth = finFactor * gCell.widthAccessCMOS * gTech.featureSize;
        _rc.cellAccessResistance = Ohm(CalculateOnResistance(accessWidth, CmosType::NMOS, gInputParameter.temperature, gTech));
        _rc.cellAccessCapacitance = Farad(CalculateDrainCap(accessWidth, CmosType::NMOS, cellWidth, gTech));
        _rc.wordlineCapacitance += 2 * Farad(CalculateGateCap(accessWidth, gTech)) * columns;
        _rc.bitlineCapacitance += _rc.cellAccessCapacitance * rows / 2; // Shared contact
        _rc.prechargeVoltage = Volt(gTech.vdd) / 2;
    } else if (gCell.memCellType == DRAM || gCell.memCellType == eDRAM) {
        // One access transistor
        const double accessWidth = finFactor * gCell.widthAccessCMOS * gTech.featureSize;
        _rc.cellAccessResistance = Ohm(CalculateOnResistance(accessWidth, CmosType::NMOS, gInputParameter.temperature, gTech));
        _rc.cellAccessCapacitance = Farad(CalculateDrainCap(accessWidth, CmosType::NMOS, cellWidth, gTech));
        _rc.wordlineCapacitance += Farad(CalculateGateCap(accessWidth, gTech)) * columns;
        _rc.bitlineCapacitance += _rc.cellAccessCapacitance * rows / 2; // Shared contact
        _rc.prechargeVoltage = Volt(gTech.vdd) / 2;
    } else if (gCell.memCellType == eDRAM3T || gCell.memCellType == eDRAM3T333) {
        // Separate read and write access transistors
        const Number writeFinFactor = gTechW.featureSize <= 14*1e-9 ? 2 : 1;
        const Number readFinFactor = gTechR.featureSize <= 14*1e-9 ? 2 : 1;
        const double writeAccessWidth = writeFinFactor * gCell.widthAccessCMOS * gTechW.featureSize;
        const double readAccessWidth = readFinFactor * gCell.widthAccessCMOSR * gTechR.featureSize;
        _rc.cellWriteAccessResistance = Ohm(CalculateOnResistance(writeAccessWidth, CmosType::NMOS, gInputParameter.temperature, gTechW));
        _rc.cellReadAccessResistance = Ohm(CalculateOnResistance(readAccessWidth, CmosType::NMOS, gInputParameter.temperature, gTechR));
        _rc.cellWriteAccessCapacitance = Farad(CalculateDrainCap(writeAccessWidth, CmosType::NMOS, cellWidth, gTechW));
        _rc.cellReadAccessCapacitance = Farad(CalculateDrainCap(readAccessWidth, CmosType::NMOS, cellWidth, gTechR));
        _rc.wordlineCapacitance += std::max(Farad(CalculateGateCap(readAccessWidth, gTechR)), Farad(CalculateGateCap(writeAccessWidth, gTechW))) * columns;
        _rc.bitlineCapacitance += std::max(_rc.cellWriteAccessCapacitance, _rc.cellReadAccessCapacitance) * rows;
        _rc.prechargeVoltage = Volt(gTech.vdd);
    } else if (gCell.memCellType == FBRAM) {
        const double soiWidth = finFactor * gCell.widthSOIDevice * gTech.featureSize;
        _rc.cellAccessCapacitance = Farad(CalculateFBRAMDrainCap(soiWidth, gTech));
        _rc.wordlineCapacitance += Farad(CalculateFBRAMGateCap(soiWidth, gCell.gateOxThicknessFactor, gTech)) * columns;
        _rc.bitlineCapacitance += _rc.cellAccessCapacitance * rows / 2; // Shared contact
        _rc.cellOffResistance = Ohm(gCell.resistanceOff);
        _rc.cellOnResistance = Ohm(gCell.resistanceOn);
    } else if (resistiveCell) {
        // CMOS, BJT, or diode access devices
        if (gCell.accessType == CMOS_access) {
            const double accessWidth = finFactor * gCell.widthAccessCMOS * gTech.featureSize;
            _rc.cellAccessResistance = Ohm(CalculateOnResistance(accessWidth, CmosType::NMOS, gInputParameter.temperature, gTech));
            _rc.cellAccessCapacitance = Farad(CalculateDrainCap(accessWidth, CmosType::NMOS, cellWidth, gTech));
            if (gCell.memCellType == FeFET || gCell.memCellType == MLCFeFET) {
                _rc.wordlineCapacitance += Farad(CalculateFeFETGateCap(accessWidth, gTech)) * columns;
            } else {
                _rc.wordlineCapacitance += Farad(CalculateGateCap(accessWidth, gTech)) * columns;
            }
            _rc.bitlineCapacitance += _rc.cellAccessCapacitance * rows / 2; // Shared contact
        } else if (gCell.accessType == BJT_access) {
            // TODO
        } else { // None or diode access
            // TODO choose the right capacitance
            _rc.cellAccessCapacitance = Farad(std::max(gCell.capacitanceOn, gCell.capacitanceOff));
            _rc.wordlineCapacitance += Farad(std::max(gCell.capacitanceOff, gCell.capacitanceOn)) * columns;
            _rc.bitlineCapacitance += Farad(std::max(gCell.capacitanceOff, gCell.capacitanceOn)) * rows;
        }
        _rc.cellOffResistance = _rc.cellAccessResistance + Ohm(gCell.resistanceOff);
        _rc.cellOnResistance = _rc.cellAccessResistance + Ohm(gCell.resistanceOn);
    } else if (gCell.memCellType == SLCNAND) {
        // The NAND string is the page count per block plus two select transistors
        const int pageCount = gInputParameter.flashBlockSize / gInputParameter.pageSize;
        const int stringLength = pageCount + 2;
        const double transistorWidth = finFactor * gTech.featureSize;
        _rc.cellAccessResistance = Ohm(CalculateOnResistance(transistorWidth, CmosType::NMOS, gInputParameter.temperature, gTech)) * stringLength;
        _rc.cellAccessCapacitance = Farad(CalculateDrainCap(transistorWidth, CmosType::NMOS, cellWidth, gTech));
        // Each cell's gate is the control gate in series with the floating gate
        _rc.wordlineCapacitance += Farad(CalculateGateCap(transistorWidth, gTech)) * columns * gCell.gateCouplingRatio / (gCell.gateCouplingRatio + 1);
        _rc.bitlineCapacitance += _rc.cellAccessCapacitance * (rows / pageCount) / 2; // Shared contact, with rows / pageCount effective rows
        _rc.prechargeVoltage = Volt(gTech.vdd) * 0.6;
    } else if (gCell.memCellType == CTT || gCell.memCellType == MLCCTT) {
        const double transistorWidth = finFactor * gTech.featureSize;
        _rc.cellAccessResistance = Ohm(CalculateOnResistance(transistorWidth, CmosType::NMOS, gInputParameter.temperature, gTech));
        _rc.cellAccessCapacitance = Farad(CalculateDrainCap(transistorWidth, CmosType::NMOS, cellWidth, gTech));
        _rc.wordlineCapacitance += Farad(CalculateGateCap(transistorWidth, gTech)) * columns;
        _rc.bitlineCapacitance += _rc.cellAccessCapacitance * rows;
        _rc.prechargeVoltage = Volt(gTech.vdd) * 0.5;
    } else { // MLC NAND flash
        // TODO
    }

    if ((gCell.memCellType == FBRAM || resistiveCell) && gCell.readMode) { // Voltage sensing
        if (gCell.readVoltage == 0) { // Current-in voltage sensing
            _rc.cellOffVoltage = Ampere(gCell.readCurrent) * _rc.cellOffResistance;
            _rc.cellOnVoltage = Ampere(gCell.readCurrent) * _rc.cellOnResistance;
        } else { // Voltage divider sensing
            const Ohm seriesResistance(std::sqrt(_rc.cellOnResistance.value() * _rc.cellOffResistance.value()));
            _rc.equivalentOnResistance = _rc.cellOnResistance * seriesResistance / (_rc.cellOnResistance + seriesResistance);
            _rc.equivalentOffResistance = _rc.cellOffResistance * seriesResistance / (_rc.cellOffResistance + seriesResistance);
            _rc.cellOffVoltage = Volt(gCell.readVoltage) * _rc.cellOffResistance / (_rc.cellOffResistance + seriesResistance);
            _rc.cellOnVoltage = Volt(gCell.readVoltage) * _rc.cellOnResistance / (_rc.cellOnResistance + seriesResistance);
        }
        // TODO a charge pump could raise the sense amp working point
        _rc.prechargeVoltage = (_rc.cellOffVoltage + _rc.cellOnVoltage) / 2;
        _rc.prechargeVoltage = std::min(Volt(gTech.vdd), _rc.prechargeVoltage);
        if (_rc.prechargeVoltage - _rc.cellOnVoltage <= _senseVoltage) {
            warn("No reasonable precharge voltage exists for the read current or voltage");
            return false;
        }
    }

    if (!build(precharger, Volt(gTech.vdd), columns, _rc.bitlineCapacitance, _rc.bitlineResistance)) {
        return false;
    }

    if (!build("Row", rowDecoder, rows, _rc.wordlineCapacitance, _rc.wordlineResistance, _multipleRowsPerSet, _areaOptimizationLevel, maxWordlineCurrent)) {
        return false;
    }

    // TODO fix the mux decoder loads
    if (!build("Bitline Mux", bitlineMuxDecoder, _bitlineMuxInputs, muxDecoderLoadCapacitance, muxDecoderLoadResistance, false, _areaOptimizationLevel, Ampere())) {
        return false;
    }

    if (!build("Level 1 Mux", senseAmpMuxLevel1Decoder, _senseAmpMuxLevel1Inputs, muxDecoderLoadCapacitance, muxDecoderLoadResistance, false,
            _areaOptimizationLevel, Ampere())) {
        return false;
    }

    if (!build("Level 2 Mux", senseAmpMuxLevel2Decoder, _senseAmpMuxLevel2Inputs, muxDecoderLoadCapacitance, muxDecoderLoadResistance, false,
            _areaOptimizationLevel, Ampere())) {
        return false;
    }

    // TODO fix the level 2 mux loads
    if (!build("Level 2", senseAmpMuxLevel2, _senseAmpMuxLevel2Inputs, columns / _bitlineMuxInputs / _senseAmpMuxLevel1Inputs / _senseAmpMuxLevel2Inputs,
            Farad(), Farad(), _maxBitlineCurrent)) {
        return false;
    }

    if (!build("Level 1", senseAmpMuxLevel1, _senseAmpMuxLevel1Inputs, columns / _bitlineMuxInputs / _senseAmpMuxLevel1Inputs,
            senseAmpMuxLevel2.rc().delayLoadCapacitance, senseAmpMuxLevel2.rc().powerLoadCapacitance, _maxBitlineCurrent)) {
        return false;
    }

    if (_internalSenseAmp) {
        const bool mlc = gCell.memCellType == MLCCTT || gCell.memCellType == MLCFeFET || gCell.memCellType == MLCRRAM;
        if (!build(senseAmp, columns / _bitlineMuxInputs, !_voltageSense, _senseVoltage, _wordlineLength / columns * _bitlineMuxInputs,
                mlc, gCell.nLvl, gCell.nFingers)) {
            return false;
        }

        if (!build("Bitline", bitlineMux, _bitlineMuxInputs, columns / _bitlineMuxInputs,
                senseAmp->loadCapacitance(), senseAmp->loadCapacitance(), _maxBitlineCurrent)) {
            return false;
        }
    } else {
        if (!build("Bitline", bitlineMux, _bitlineMuxInputs, columns / _bitlineMuxInputs,
                senseAmpMuxLevel1.rc().delayLoadCapacitance, senseAmpMuxLevel1.rc().powerLoadCapacitance, _maxBitlineCurrent)) {
            return false;
        }
    }

    // Control signals are not buffered for now
    return build(buffer, 1, columns);
}

void mse::component::SubArray::report(Reporter& reporter) const noexcept {
    RampedComponent::report(reporter);

    const SubArrayTiming& timing = this->timing();
    reporter.report(ReportGroup::Timing, "Set Latency", timing.setLatency);
    reporter.report(ReportGroup::Timing, "Reset Latency", timing.resetLatency);
    reporter.report(ReportGroup::Timing, "Refresh Latency", timing.refreshLatency);
    reporter.report(ReportGroup::Timing, "Bitline Latency", timing.bitlineLatency);
    reporter.report(ReportGroup::Timing, "Charge Latency", timing.chargeLatency);
    reporter.report(ReportGroup::Timing, "Column Decoder Latency", timing.columnDecoderLatency);

    const SubArrayPower& power = this->power();
    reporter.report(ReportGroup::Power, "Set Dynamic Energy", power.setDynamicEnergy);
    reporter.report(ReportGroup::Power, "Reset Dynamic Energy", power.resetDynamicEnergy);
    reporter.report(ReportGroup::Power, "Refresh Dynamic Energy", power.refreshDynamicEnergy);
    reporter.report(ReportGroup::Power, "Cell Read Energy", power.cellReadEnergy);
    reporter.report(ReportGroup::Power, "Cell Set Energy", power.cellSetEnergy);
    reporter.report(ReportGroup::Power, "Cell Reset Energy", power.cellResetEnergy);
}

void mse::component::SubArray::reset() noexcept {
    RampedComponent::reset();
    forEachSubcomponent([] (Component& subcomponent) noexcept { subcomponent.reset(); });
}

mse::Dimensions mse::component::SubArray::calculateDimensions() const noexcept {
    using namespace mse::unit;

    Meter width = _wordlineLength;
    Meter height = _bitlineLength;
    Meter addedWidth;
    Meter addedHeight;

    const Dimensions& rowDecoderDimensions = rowDecoder.dimensions();
    if (rowDecoderDimensions.height > height) {
        // Assume magic folding
        addedWidth = rowDecoderDimensions.area() / height;
    } else {
        // Allow white space
        addedWidth = rowDecoderDimensions.width;
    }

    const Dimensions& prechargerDimensions = precharger.dimensions();
    if (prechargerDimensions.width > width) {
        // Assume magic folding
        addedHeight = prechargerDimensions.area() / prechargerDimensions.width;
    } else {
        // Allow white space
        addedHeight = prechargerDimensions.height;
    }

    addedHeight += bitlineMux.dimensions().height;

    if (senseAmp) {
        const Dimensions& senseAmpDimensions = senseAmp->dimensions();
        if (senseAmpDimensions.width > width * 1.001) {
            error("Sense amp is wider than the subarray");
        } else {
            addedHeight += senseAmpDimensions.height;
        }
    }

    addedHeight += senseAmpMuxLevel1.dimensions().height;
    addedHeight += senseAmpMuxLevel2.dimensions().height;

    addedWidth = std::max(addedWidth, bitlineMuxDecoder.dimensions().width);
    addedWidth = std::max(addedWidth, senseAmpMuxLevel1Decoder.dimensions().width);
    addedWidth = std::max(addedWidth, senseAmpMuxLevel2Decoder.dimensions().width);

    // Assume the buffer is as wide as the subarray
    addedHeight += buffer.dimensions().height;

    width += addedWidth;
    height += addedHeight;

    // Monolithic 3D cells fit above the peripherals
    if (gCell.memCellType == eDRAM3T333) {
        const Meter peripheralWidth = width - _wordlineLength;
        const Meter peripheralHeight = height - _bitlineLength;
        width = std::max(width, peripheralWidth);
        height = std::max(height, peripheralHeight);
    }

    return Dimensions{
        .width = width,
        .height = height
    };
}

mse::component::SubArrayTiming mse::component::SubArray::calculateTiming(physics::HorowitzRamp inputRamp) const noexcept {
    using namespace mse::unit;

    SubArrayTiming timing;

    Second senseAmpLatency;
    if (senseAmp) {
        senseAmpLatency = senseAmp->timing().readLatency;
    }

    precharger.setInputRamp(inputRamp);
    rowDecoder.setInputRamp(inputRamp);
    bitlineMuxDecoder.setInputRamp(inputRamp);
    senseAmpMuxLevel1Decoder.setInputRamp(inputRamp);
    senseAmpMuxLevel2Decoder.setInputRamp(inputRamp);

    timing.columnDecoderLatency = std::max({bitlineMuxDecoder.timing().readLatency, senseAmpMuxLevel1Decoder.timing().readLatency,
            senseAmpMuxLevel2Decoder.timing().readLatency});
    const Second decoderLatency = std::max(rowDecoder.timing().readLatency, timing.columnDecoderLatency);
    const physics::HorowitzRamp wordlineRamp = rowDecoder.timing().outputRamp;

    // TODO revisit this equation
    const Farad passCapacitance = bitlineMux.rc().passCapacitance + senseAmpMuxLevel1.rc().passCapacitance + senseAmpMuxLevel2.rc().passCapacitance;
    const Ohm passResistance = bitlineMux.rc().passResistance + senseAmpMuxLevel1.rc().passResistance + senseAmpMuxLevel2.rc().passResistance;
    const Second chargeTau = passResistance * (passCapacitance + _rc.bitlineCapacitance) + _rc.bitlineResistance * _rc.bitlineCapacitance / 2;
    timing.chargeLatency = physics::horowitz(chargeTau, 0, physics::stepRamp).delay;

    const Farad muxLoadCapacitance = bitlineMux.rc().delayLoadCapacitance;
    const Number finFactor = gTech.featureSize <= 14*1e-9 ? 2 : 1;

    if (gCell.memCellType == SRAM) {
        const Ohm pullDownResistance(CalculateOnResistance(finFactor * gCell.widthSRAMCellNMOS * gTech.featureSize, CmosType::NMOS,
                gInputParameter.temperature, gTech));
        Second tau = (_rc.cellAccessResistance + pullDownResistance) * (_rc.cellAccessCapacitance + _rc.bitlineCapacitance + muxLoadCapacitance)
                + _rc.bitlineResistance * (muxLoadCapacitance + _rc.bitlineCapacitance / 2);
        // One bitline rises and the other drops, so half the sense voltage is enough
        tau *= std::log(_rc.prechargeVoltage / (_rc.prechargeVoltage - _senseVoltage / 2));
        const double transconductance = CalculateTransconductance(finFactor * gCell.widthAccessCMOS * gTech.featureSize, CmosType::NMOS, gTech);
        const double beta = 1 / (pullDownResistance.value() * transconductance);
        timing.bitlineLatency = physics::horowitz(tau, beta, wordlineRamp).delay;
        timing.readLatency = decoderLatency + timing.bitlineLatency + bitlineMux.timing().readLatency + senseAmpLatency
                + senseAmpMuxLevel1.timing().readLatency + senseAmpMuxLevel2.timing().readLatency;
        // Assume symmetric read and write bitline delay
        timing.writeLatency = timing.readLatency;
    } else if (gCell.memCellType == DRAM || gCell.memCellType == eDRAM) {
        const Farad cellCapacitance = _rc.cellAccessCapacitance + Farad(gCell.capDRAMCell);
        const Farad capacitance = cellCapacitance * (_rc.bitlineCapacitance + muxLoadCapacitance)
                / (cellCapacitance + _rc.bitlineCapacitance + muxLoadCapacitance);
        const Ohm resistance = _rc.bitlineResistance + _rc.cellAccessResistance;
        const Second tau = 2.3 * resistance * capacitance;
        timing.bitlineLatency = physics::horowitz(tau, 0, wordlineRamp).delay;

        // Refresh does not pass through the sense amp mux, and covers every row
        timing.refreshLatency = decoderLatency + timing.bitlineLatency + senseAmpLatency;
        timing.refreshLatency *= rows;
        timing.readLatency = decoderLatency + timing.bitlineLatency + senseAmpLatency
                + senseAmpMuxLevel1.timing().readLatency + senseAmpMuxLevel2.timing().readLatency;
        // Assume symmetric read and write bitline delay
        timing.writeLatency = timing.readLatency;
    } else if (gCell.memCellType == eDRAM3T || gCell.memCellType == eDRAM3T333) {
        const Farad writeCapacitance = _rc.cellWriteAccessCapacitance + Farad(gCell.capDRAMCell) + _rc.bitlineCapacitance + muxLoadCapacitance;
        const Second writeTau = 2.3 * (_rc.cellWriteAccessResistance * writeCapacitance
                + _rc.bitlineResistance * (muxLoadCapacitance + _rc.bitlineCapacitance / 2.0));
        const Farad readCapacitance = _rc.cellReadAccessCapacitance + Farad(gCell.capDRAMCell) + _rc.bitlineCapacitance + muxLoadCapacitance;
        const Second readTau = 2.3 * (_rc.cellReadAccessResistance * readCapacitance
                + _rc.bitlineResistance * (muxLoadCapacitance + _rc.bitlineCapacitance / 2.0));
        timing.bitlineWriteLatency = physics::horowitz(writeTau, 0, wordlineRamp).delay;
        timing.bitlineReadLatency = physics::horowitz(readTau, 0, wordlineRamp).delay;

        // Refresh does not pass through the sense amp mux, and covers every row
        timing.refreshLatency = decoderLatency + timing.bitlineWriteLatency + senseAmpLatency;
        timing.refreshLatency *= rows;
        timing.readLatency = decoderLatency + timing.bitlineReadLatency + senseAmpLatency
                + senseAmpMuxLevel1.timing().readLatency + senseAmpMuxLevel2.timing().readLatency;
        timing.writeLatency = decoderLatency + timing.bitlineWriteLatency;
    } else if (gCell.memCellType == MRAM || gCell.memCellType == PCRAM || gCell.memCellType == memristor || gCell.memCellType == FBRAM
            || gCell.memCellType == FeFET || gCell.memCellType == MLCFeFET || gCell.memCellType == MLCRRAM) {
        if (!gCell.readMode) { // Current sensing, ICCAD 2009 model
            const Second tau = _rc.bitlineResistance * _rc.bitlineCapacitance / 2 * (_rc.cellOffResistance + _rc.bitlineResistance / 3)
                    / (_rc.cellOffResistance + _rc.bitlineResistance);
            timing.bitlineLatency = physics::horowitz(tau, 0, wordlineRamp).delay;
        } else { // Voltage sensing
            // Current-in sensing uses the cell resistances, voltage divider sensing uses the equivalent resistances
            const bool currentIn = gCell.readVoltage == 0;
            const Ohm onResistance = currentIn ? _rc.cellOnResistance : _rc.equivalentOnResistance;
            const Ohm offResistance = currentIn ? _rc.cellOffResistance : _rc.equivalentOffResistance;

            const Second onTau = onResistance * (_rc.cellAccessCapacitance + _rc.bitlineCapacitance + muxLoadCapacitance)
                    + _rc.bitlineResistance * (muxLoadCapacitance + _rc.bitlineCapacitance / 2);
            timing.bitlineOnLatency = onTau * std::log((_rc.prechargeVoltage - _rc.cellOnVoltage)
                    / (_rc.prechargeVoltage - _rc.cellOnVoltage - _senseVoltage));
            const Second offTau = offResistance * (_rc.cellAccessCapacitance + _rc.bitlineCapacitance + muxLoadCapacitance)
                    + _rc.bitlineResistance * (muxLoadCapacitance + _rc.bitlineCapacitance / 2);
            timing.bitlineOffLatency = offTau * std::log((_rc.cellOffVoltage - _rc.prechargeVoltage)
                    / (_rc.cellOffVoltage - _rc.prechargeVoltage - _senseVoltage));
            timing.bitlineLatency = std::max(timing.bitlineOnLatency, timing.bitlineOffLatency);
        }
        timing.readLatency = decoderLatency + timing.bitlineLatency + bitlineMux.timing().readLatency + senseAmpLatency
                + senseAmpMuxLevel1.timing().readLatency + senseAmpMuxLevel2.timing().readLatency;

        const Second rowDecoderWriteLatency = rowDecoder.timing().writeLatency;
        const Second setPulse(gCell.setPulse);
        const Second resetPulse(gCell.resetPulse);
        const bool writeAndVerify = gCell.memCellType == PCRAM && gInputParameter.writeScheme == write_and_verify;
        // PCRAM and FBRAM always write through an access device
        const bool selectorlessAccess = gCell.memCellType != PCRAM && gCell.memCellType != FBRAM
                && (gCell.accessType == diode_access || gCell.accessType == none_access);

        if (writeAndVerify) {
            // TODO write and verify programming
        } else if (selectorlessAccess) {
            if (gInputParameter.writeScheme == erase_before_reset || gInputParameter.writeScheme == erase_before_set) {
                timing.writeLatency = std::max(rowDecoderWriteLatency, timing.chargeLatency);
            } else {
                timing.writeLatency = std::max(rowDecoderWriteLatency, timing.columnDecoderLatency + timing.chargeLatency);
            }
            timing.writeLatency += timing.chargeLatency;
            timing.writeLatency += resetPulse + setPulse;
        } else {
            // TODO why not use the precharger latency directly
            timing.writeLatency = std::max(rowDecoderWriteLatency, timing.columnDecoderLatency + timing.chargeLatency);
            timing.resetLatency = timing.writeLatency + resetPulse;
            timing.setLatency = timing.writeLatency + setPulse;
            timing.writeLatency += std::max(resetPulse, setPulse);
        }
    } else if (gCell.memCellType == SLCNAND || gCell.memCellType == CTT || gCell.memCellType == MLCCTT) {
        // A NAND string is the page count per block plus two select transistors, a CTT cell pulls down through two transistors
        const int stringLength = gCell.memCellType == SLCNAND ? gInputParameter.flashBlockSize / gInputParameter.pageSize + 2 : 2;
        const double transistorWidth = finFactor * gTech.featureSize;

        const Ohm pullDownResistance = Ohm(CalculateOnResistance(transistorWidth, CmosType::NMOS, gInputParameter.temperature, gTech)) * stringLength;
        Second tau = pullDownResistance * (_rc.cellAccessCapacitance + _rc.bitlineCapacitance + muxLoadCapacitance)
                + _rc.bitlineResistance * (muxLoadCapacitance + _rc.bitlineCapacitance / 2);
        // The bitline either stays put or drops from 0.6V to 0.4V
        tau *= std::log(_rc.prechargeVoltage / (_rc.prechargeVoltage - _senseVoltage));
        const double transconductance = CalculateTransconductance(transistorWidth, CmosType::NMOS, gTech); // Minimum size transistor
        const double beta = 1 / (pullDownResistance.value() * transconductance);
        timing.bitlineLatency = physics::horowitz(tau, beta, wordlineRamp).delay;
        // TODO revisit, corrects an unnecessary horowitz calculation
        timing.bitlineLatency = std::max(timing.bitlineLatency, tau * 20);
        timing.readLatency = decoderLatency + timing.bitlineLatency + bitlineMux.timing().readLatency + senseAmpLatency
                + senseAmpMuxLevel1.timing().readLatency + senseAmpMuxLevel2.timing().readLatency;

        // Erase is reset and programming is set
        const Second accessLatency = std::max(rowDecoder.timing().readLatency, timing.columnDecoderLatency + timing.chargeLatency);
        if (gCell.memCellType == SLCNAND) {
            timing.resetLatency = accessLatency + Second(gCell.flashEraseTime);
            timing.setLatency = accessLatency + Second(gCell.flashProgramTime);
        } else {
            timing.resetLatency = accessLatency;
            timing.setLatency = accessLatency;
        }
        // Programming latency is the write latency
        timing.writeLatency = timing.setLatency;
    } else { // MLC NAND
        // TODO
    }

    return timing;
}

mse::component::SubArrayPower mse::component::SubArray::calculatePower() const noexcept {
    using namespace mse::unit;

    SubArrayPower power;
    power.rowDecoderReadDynamicEnergy = rowDecoder.power().dynamicReadEnergy;
    power.rowDecoderWriteDynamicEnergy = rowDecoder.power().dynamicWriteEnergy;
    Joule rowDecoderSetDynamicEnergy;
    Joule rowDecoderResetDynamicEnergy;

    Second senseAmpLatency;
    BasicPower senseAmpPower;
    if (senseAmp) {
        senseAmpLatency = senseAmp->timing().readLatency;
        senseAmpPower = senseAmp->power();
    }

    const long long selectedColumns = columns / _bitlineMuxInputs / _senseAmpMuxLevel1Inputs / _senseAmpMuxLevel2Inputs;
    const Farad bitlineCapacitance = _rc.cellAccessCapacitance + _rc.bitlineCapacitance + bitlineMux.rc().powerLoadCapacitance;
    const Number finFactor = gTech.featureSize <= 14*1e-9 ? 2 : 1;
    const Volt vdd(gTech.vdd);

    if (gCell.memCellType == SRAM) {
        power.dynamicReadEnergy = bitlineCapacitance * _rc.prechargeVoltage * _rc.prechargeVoltage * columns;
        power.dynamicWriteEnergy = bitlineCapacitance * _rc.prechargeVoltage * _rc.prechargeVoltage * columns
                / _bitlineMuxInputs / _senseAmpMuxLevel1Inputs / _senseAmpMuxLevel2Inputs;
        // Two inverters per cell
        power.leakage = Ampere(CalculateGateLeakage(::GateType::INV, 1, finFactor * gCell.widthSRAMCellNMOS * gTech.featureSize,
                finFactor * gCell.widthSRAMCellPMOS * gTech.featureSize, gInputParameter.temperature, gTech)) * vdd * 2;
        // Two access NMOS, combined as one with vdd crossed
        power.leakage += Watt(Ampere(CalculateGateLeakage(::GateType::INV, 1, finFactor * gCell.widthAccessCMOS * gTech.featureSize, 0,
                gInputParameter.temperature, gTech)) * vdd);
        power.leakage *= rows * columns;
    } else if (gCell.memCellType == DRAM || gCell.memCellType == eDRAM) {
        power.dynamicReadEnergy = bitlineCapacitance * _senseVoltage * vdd * columns;
        // Should also equal the set voltage, Vdd for DRAM
        const Volt writeVoltage(gTech.vpp);
        power.dynamicWriteEnergy = (_rc.bitlineCapacitance + bitlineMux.rc().powerLoadCapacitance) * writeVoltage * writeVoltage * columns;
        power.refreshDynamicEnergy = power.dynamicReadEnergy + power.dynamicWriteEnergy;
        power.leakage = Ampere(CalculateGateLeakage(::GateType::INV, 1, finFactor * gCell.widthAccessCMOS * gTech.featureSize, 0,
                gInputParameter.temperature, gTech)) * vdd;
        power.leakage *= columns;
    } else if (gCell.memCellType == eDRAM3T || gCell.memCellType == eDRAM3T333) {
        power.dynamicReadEnergy = (_rc.cellReadAccessCapacitance + _rc.bitlineCapacitance + bitlineMux.rc().powerLoadCapacitance)
                * _senseVoltage * Volt(gTechR.vdd) * columns;
        // Should also equal the set voltage, Vdd for DRAM
        const Volt writeVoltage(gTechW.vpp);
        power.dynamicWriteEnergy = (_rc.bitlineCapacitance + bitlineMux.rc().powerLoadCapacitance) * writeVoltage * writeVoltage * columns;
        power.refreshDynamicEnergy = power.dynamicReadEnergy + power.dynamicWriteEnergy;
        power.leakage = Ampere(CalculateGateLeakage(::GateType::INV, 1, finFactor * gCell.widthAccessCMOS * gTech.featureSize, 0,
                gInputParameter.temperature, gTech)) * vdd;
        power.leakage *= columns;
    } else if (gCell.memCellType == MRAM || gCell.memCellType == PCRAM || gCell.memCellType == memristor || gCell.memCellType == FBRAM
            || gCell.memCellType == FeFET || gCell.memCellType == MLCFeFET || gCell.memCellType == MLCRRAM) {
        if (!gCell.readMode) { // Current sensing, ICCAD 2009 model
            const Ohm muxResistance = bitlineMux.rc().passResistance;
            const Volt readVoltage(gCell.readVoltage);
            const Volt minPrechargeVoltage = readVoltage * muxResistance / (muxResistance + _rc.bitlineResistance + _rc.cellOnResistance);
            const Volt maxPrechargeVoltage = readVoltage * (muxResistance + _rc.bitlineResistance)
                    / (muxResistance + _rc.bitlineResistance + _rc.cellOnResistance);
            power.dynamicReadEnergy = _rc.cellAccessCapacitance * maxPrechargeVoltage * maxPrechargeVoltage
                    + bitlineMux.rc().powerLoadCapacitance * minPrechargeVoltage * minPrechargeVoltage
                    + _rc.bitlineCapacitance * (maxPrechargeVoltage * maxPrechargeVoltage + minPrechargeVoltage * minPrechargeVoltage
                    + maxPrechargeVoltage * minPrechargeVoltage) / 3;
            power.dynamicReadEnergy *= columns;
        } else { // Voltage sensing
            power.dynamicReadEnergy = bitlineCapacitance * (_rc.prechargeVoltage * _rc.prechargeVoltage - _rc.cellOnVoltage * _rc.cellOnVoltage) * columns;
        }

        // Doubled for the reference cell
        const Watt cellReadPower(gCell.readPower == 0 ? gCell.CalculateReadPower() : gCell.readPower);
        power.cellReadEnergy = Joule(2 * cellReadPower.value() * senseAmpLatency.value());
        power.cellReadEnergy *= selectedColumns;

        // Ignore the dynamic transition during set and reset, and assume the cell stays high resistance for worst case power
        gCell.CalculateWriteEnergy();

        Joule resetEnergyPerBit(gCell.resetEnergy);
        Joule setEnergyPerBit(gCell.setEnergy);
        const Volt setVoltage = gCell.setMode ? Volt(gCell.setVoltage) : vdd;
        const Volt resetVoltage = gCell.resetMode ? Volt(gCell.resetVoltage) : vdd;
        setEnergyPerBit += Joule(bitlineCapacitance * setVoltage * setVoltage);
        resetEnergyPerBit += Joule(bitlineCapacitance * resetVoltage * resetVoltage);

        const bool selectorlessAccess = gCell.accessType == diode_access || gCell.accessType == none_access;
        const bool eraseFirst = gInputParameter.writeScheme == erase_before_reset || gInputParameter.writeScheme == erase_before_set;
        const auto selectedEnergy = [this] (Joule energyPerBit) {
            return energyPerBit * columns / _bitlineMuxInputs / _senseAmpMuxLevel1Inputs / _senseAmpMuxLevel2Inputs;
        };

        if (gCell.memCellType == PCRAM) {
            if (gInputParameter.writeScheme == write_and_verify) {
                // TODO write and verify programming
            } else {
                // Shaper inefficiency
                power.cellResetEnergy = selectedEnergy(resetEnergyPerBit) / constant::shaperEfficiencyConservative;
                power.cellSetEnergy = selectedEnergy(setEnergyPerBit) / constant::shaperEfficiencyConservative;
                power.dynamicWriteEnergy = std::max(power.cellResetEnergy, power.cellSetEnergy);
            }
        } else if (gCell.memCellType == FBRAM) {
            // Shaper inefficiency
            power.cellResetEnergy = selectedEnergy(resetEnergyPerBit) / constant::shaperEfficiencyAggressive;
            power.cellSetEnergy = selectedEnergy(setEnergyPerBit) / constant::shaperEfficiencyAggressive;
            power.dynamicWriteEnergy = std::max(power.cellResetEnergy, power.cellSetEnergy);
        } else { // MRAM and memristor
            power.cellResetEnergy = selectedEnergy(resetEnergyPerBit);
            power.cellSetEnergy = selectedEnergy(setEnergyPerBit);
            if (selectorlessAccess && eraseFirst) {
                // TODO this ignores the write pattern
                power.dynamicWriteEnergy = power.cellResetEnergy + power.cellSetEnergy;
            } else {
                power.dynamicWriteEnergy = std::max(power.cellResetEnergy, power.cellSetEnergy);
            }
            // Shaper inefficiency
            power.cellResetEnergy /= constant::shaperEfficiencyAggressive;
            power.cellSetEnergy /= constant::shaperEfficiencyAggressive;
            power.dynamicWriteEnergy /= constant::shaperEfficiencyAggressive;
        }
        // TODO the cell leaks during read and write
    } else if (gCell.memCellType == SLCNAND) {
        // The NAND string is the page count per block plus two select transistors
        const int pageCount = gInputParameter.flashBlockSize / gInputParameter.pageSize;
        const int stringLength = pageCount + 2;
        const long long blockWordlines = rows / pageCount * stringLength;
        const Volt passVoltage(gCell.flashPassVoltage);
        const Volt programVoltage(gCell.flashProgramVoltage);
        const Volt eraseVoltage(gCell.flashEraseVoltage);

        // Read: only the selected bitline is charged, to the precharge voltage
        power.dynamicReadEnergy = bitlineCapacitance * _rc.prechargeVoltage * _rc.prechargeVoltage * columns;
        // Unlike other cells, the select lines and unselected wordlines in a block charge to Vpass, and the selected wordline does not charge
        rowDecoderResetDynamicEnergy = power.rowDecoderReadDynamicEnergy;
        rowDecoderSetDynamicEnergy = power.rowDecoderReadDynamicEnergy;
        // Approximated by scaling the Vdd energy to Vpass
        Joule wordlineReadEnergy = power.rowDecoderReadDynamicEnergy / vdd / vdd * passVoltage * passVoltage;
        wordlineReadEnergy *= blockWordlines - 1; // Every wordline but the selected one
        power.rowDecoderReadDynamicEnergy = wordlineReadEnergy;

        // Program (set): the source line charges to Vdd and acts as part of the bitline
        power.setDynamicEnergy = bitlineCapacitance * programVoltage * programVoltage * columns;
        // Tunneling current, over every column since unselected bitlines also precharge to Vdd
        power.setDynamicEnergy += Joule(constant::floatingGateThresholdDelta.value() * constant::tunnelCurrentFlow.value() * gCell.area
                * gTech.featureSize * gTech.featureSize * gCell.flashProgramTime * columns);
        // The string select line precharges to Vdd like the row decoder energy, unselected wordlines in the block to Vpass, and the selected one to Vpgm
        Joule wordlineSetEnergy = rowDecoderSetDynamicEnergy;
        wordlineSetEnergy += Joule(rowDecoderSetDynamicEnergy / vdd / vdd * passVoltage * passVoltage * (blockWordlines - 1));
        wordlineSetEnergy += Joule(rowDecoderSetDynamicEnergy / vdd / vdd * programVoltage * programVoltage);
        rowDecoderSetDynamicEnergy = wordlineSetEnergy;

        // Erase (reset): every bitline and the source line precharge to Vera - Vbi
        const Volt bitlineEraseVoltage = eraseVoltage - Volt(gTech.buildInPotential);
        power.resetDynamicEnergy = bitlineCapacitance * bitlineEraseVoltage * bitlineEraseVoltage;
        power.resetDynamicEnergy *= columns + 1; // Plus the source line
        // The selected block shares one p-well, which precharges to Vera
        Farad wellJunctionCapacitance(gTech.capJunction * gCell.area * gTech.featureSize * gTech.featureSize);
        wellJunctionCapacitance *= gInputParameter.flashBlockSize;
        power.resetDynamicEnergy += Joule(wellJunctionCapacitance * eraseVoltage * eraseVoltage);
        // Wordlines and select lines in unselected blocks, and select lines in the selected block, precharge to Vera * beta
        const double beta = 0.8;
        Joule wordlineResetEnergy = rowDecoderResetDynamicEnergy / vdd / vdd * (eraseVoltage * beta) * (eraseVoltage * beta);
        wordlineResetEnergy *= blockWordlines - pageCount;
        rowDecoderResetDynamicEnergy = wordlineResetEnergy;

        // Write energy is the average energy per page
        power.rowDecoderWriteDynamicEnergy = (rowDecoderSetDynamicEnergy + rowDecoderResetDynamicEnergy / pageCount) / 2;
        power.dynamicWriteEnergy = (power.setDynamicEnergy + power.resetDynamicEnergy / pageCount) / 2;

        // Assume NAND flash cells do not leak
    } else { // MLC NAND
        // TODO
    }

    if (gInputParameter.designTarget == cache && gInputParameter.cacheAccessMode != sequential_access_mode) {
        power.cellResetEnergy /= gInputParameter.associativity;
        power.cellSetEnergy /= gInputParameter.associativity;
        power.dynamicWriteEnergy /= gInputParameter.associativity;
        power.resetDynamicEnergy /= gInputParameter.associativity;
        power.setDynamicEnergy /= gInputParameter.associativity;
    }

    power.dynamicReadEnergy += power.cellReadEnergy + power.rowDecoderReadDynamicEnergy + bitlineMuxDecoder.power().dynamicReadEnergy
            + senseAmpMuxLevel1Decoder.power().dynamicReadEnergy + senseAmpMuxLevel2Decoder.power().dynamicReadEnergy
            + precharger.power().dynamicReadEnergy + bitlineMux.power().dynamicReadEnergy + senseAmpPower.dynamicReadEnergy
            + senseAmpMuxLevel1.power().dynamicReadEnergy + senseAmpMuxLevel2.power().dynamicReadEnergy;
    power.dynamicWriteEnergy += power.rowDecoderWriteDynamicEnergy + bitlineMuxDecoder.power().dynamicWriteEnergy
            + senseAmpMuxLevel1Decoder.power().dynamicWriteEnergy + senseAmpMuxLevel2Decoder.power().dynamicWriteEnergy
            + bitlineMux.power().dynamicWriteEnergy + senseAmpPower.dynamicWriteEnergy
            + senseAmpMuxLevel1.power().dynamicWriteEnergy + senseAmpMuxLevel2.power().dynamicWriteEnergy;

    // Reading every column with the row decoder, sense amps and precharger refreshes one row
    power.refreshDynamicEnergy += power.rowDecoderReadDynamicEnergy + precharger.power().dynamicReadEnergy + senseAmpPower.dynamicReadEnergy;
    power.refreshDynamicEnergy *= rows;

    // Asymmetric set and reset only
    power.setDynamicEnergy += power.cellSetEnergy + rowDecoderSetDynamicEnergy + bitlineMuxDecoder.power().dynamicWriteEnergy
            + senseAmpMuxLevel1Decoder.power().dynamicWriteEnergy + senseAmpMuxLevel2Decoder.power().dynamicWriteEnergy
            + bitlineMux.power().dynamicWriteEnergy + senseAmpPower.dynamicWriteEnergy
            + senseAmpMuxLevel1.power().dynamicWriteEnergy + senseAmpMuxLevel2.power().dynamicWriteEnergy;
    power.resetDynamicEnergy += power.setDynamicEnergy + rowDecoderResetDynamicEnergy + bitlineMuxDecoder.power().dynamicWriteEnergy
            + senseAmpMuxLevel1Decoder.power().dynamicWriteEnergy + senseAmpMuxLevel2Decoder.power().dynamicWriteEnergy
            + bitlineMux.power().dynamicWriteEnergy + senseAmpPower.dynamicWriteEnergy
            + senseAmpMuxLevel1.power().dynamicWriteEnergy + senseAmpMuxLevel2.power().dynamicWriteEnergy;

    if (gCell.accessType == diode_access || gCell.accessType == none_access) {
        power.dynamicWriteEnergy += bitlineMux.power().dynamicWriteEnergy + senseAmpPower.dynamicWriteEnergy
                + senseAmpMuxLevel1.power().dynamicWriteEnergy + senseAmpMuxLevel2.power().dynamicWriteEnergy;
    }

    power.leakage += rowDecoder.power().leakage + bitlineMuxDecoder.power().leakage + senseAmpMuxLevel1Decoder.power().leakage
            + senseAmpMuxLevel2Decoder.power().leakage + precharger.power().leakage + bitlineMux.power().leakage
            + senseAmpPower.leakage + senseAmpMuxLevel1.power().leakage + senseAmpMuxLevel2.power().leakage
            + buffer.power().leakage + buffer.power().xorLeakage;

    return power;
}
