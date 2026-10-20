#include "component/PredecodeBlock.hpp"
#include "global.hpp"

mse::component::PredecodeBlock::PredecodeBlock(int addressBits, unit::Farad loadCapacitance, unit::Ohm loadResistance)
        : _addressBits(addressBits),
          _loadCapacitance(loadCapacitance),
          _loadResistance(loadResistance) {}

bool mse::component::PredecodeBlock::setup() noexcept {
    using namespace mse::unit;

    if (_addressBits > 27) {
        fatal("Invalid number of address bits");
    }

    if (_addressBits == 0) {
        return true;
    }

    int decoders12 = 0;
    int decoders24 = 0;
    int decoders38 = 0;
    int outputs = 1 << _addressBits;
    if (_addressBits == 1) {
        decoders12 = 1;
    } else {
        int remainder = _addressBits % 3;
        if (remainder == 2) {
            decoders24 = 1;
        } else if (remainder == 1) {
            decoders24 = 2;
        }
        decoders38 = (_addressBits - 2 * decoders24) / 3;
    }

    auto buildRowDecoder = [this] (std::string name, std::optional<RowDecoder>& decoder, int rows, Farad loadCapacitance, Ohm loadResistance,
            bool multipleRowsPerSet) {
        return build(std::move(name), decoder, rows, loadCapacitance, loadResistance, multipleRowsPerSet, latency_first, Ampere());
    };

    // Stage 2 is built first since the stage 1 loads are its NAND inputs, and stage 1 is built before the basic decoders it loads
    int basicDecoders = decoders12 + decoders24 + decoders38;
    if (basicDecoders <= 1) {
        // No row decoders, a basic decoder drives the load directly
    } else if (basicDecoders <= 3) {
        _nandInputsStage1A = basicDecoders;
        if (!buildRowDecoder("Stage 1A", _rowDecoderStage1A, outputs, _loadCapacitance, _loadResistance, _nandInputsStage1A == 3)) {
            return false;
        }
    } else {
        int rowsStage1A = 1;
        int rowsStage1B = 1;
        int rowsStage1C = 1;
        if (basicDecoders <= 6) {
            if (!buildRowDecoder("Stage 2", _rowDecoderStage2, outputs, _loadCapacitance, _loadResistance, false)) {
                return false;
            }

            _nandInputsStage1B = basicDecoders / 2;
            _nandInputsStage1A = basicDecoders - _nandInputsStage1B;
            rowsStage1A <<= 3 * _nandInputsStage1A - decoders24;
            rowsStage1B <<= 3 * _nandInputsStage1B;
            const Farad loadStage1A = rowsStage1B * _rowDecoderStage2->nandInputCapacitance();
            const Farad loadStage1B = rowsStage1A * _rowDecoderStage2->nandInputCapacitance();
            if (!buildRowDecoder("Stage 1A", _rowDecoderStage1A, rowsStage1A, loadStage1A, Ohm(), _nandInputsStage1A == 3)) { // TODO
                return false;
            }

            if (!buildRowDecoder("Stage 1B", _rowDecoderStage1B, rowsStage1B, loadStage1B, Ohm(), _nandInputsStage1B == 3)) { // TODO
                return false;
            }
        } else if (basicDecoders <= 9) {
            if (!buildRowDecoder("Stage 2", _rowDecoderStage2, outputs, _loadCapacitance, _loadResistance, true)) {
                return false;
            }

            if (basicDecoders == 7) {
                _nandInputsStage1A = 3;
                _nandInputsStage1B = 2;
                _nandInputsStage1C = 2;
                rowsStage1B = 64;
                rowsStage1C = 64;
            } else if (basicDecoders == 8) {
                _nandInputsStage1A = 3;
                _nandInputsStage1B = 3;
                _nandInputsStage1C = 2;
                rowsStage1B = 512;
                rowsStage1C = 64;
            } else {
                _nandInputsStage1A = 3;
                _nandInputsStage1B = 3;
                _nandInputsStage1C = 3;
                rowsStage1B = 512;
                rowsStage1C = 512;
            }
            rowsStage1A <<= 3 * _nandInputsStage1A - decoders24;
            const Farad loadStage1A = rowsStage1B * rowsStage1C * _rowDecoderStage2->nandInputCapacitance();
            const Farad loadStage1B = rowsStage1A * rowsStage1C * _rowDecoderStage2->nandInputCapacitance();
            const Farad loadStage1C = rowsStage1A * rowsStage1B * _rowDecoderStage2->nandInputCapacitance();
            if (!buildRowDecoder("Stage 1A", _rowDecoderStage1A, rowsStage1A, loadStage1A, Ohm(), _nandInputsStage1A == 3)) { // TODO
                return false;
            }

            if (!buildRowDecoder("Stage 1B", _rowDecoderStage1B, rowsStage1B, loadStage1B, Ohm(), _nandInputsStage1B == 3)) { // TODO
                return false;
            }

            if (!buildRowDecoder("Stage 1C", _rowDecoderStage1C, rowsStage1C, loadStage1C, Ohm(), _nandInputsStage1C == 3)) { // TODO
                return false;
            }
        }
    }

    if (_rowDecoderStage1C) {
        if (!build("C", _basicDecoderC, 3, (_nandInputsStage1C == 2 ? 8 : 64) * _rowDecoderStage1C->nandInputCapacitance(), Ohm())) { // TODO
            return false;
        }
    }

    if (_rowDecoderStage1B) {
        if (!build("B", _basicDecoderB, 3, (_nandInputsStage1B == 2 ? 8 : 64) * _rowDecoderStage1B->nandInputCapacitance(), Ohm())) { // TODO
            return false;
        }
    }

    if (_rowDecoderStage1A) {
        const Farad nandCapacitanceStage1A = _rowDecoderStage1A->nandInputCapacitance();
        if (decoders24 == 0) {
            _basicDecodersA1 = _nandInputsStage1A;
            _basicDecodersA2 = 0;
            int loadsA1 = 1 << (3 * (_nandInputsStage1A - 1));

            return build("A1", _basicDecoderA1, 3, loadsA1 * nandCapacitanceStage1A, Ohm()); // TODO
        }

        if (decoders24 == 1) {
            _basicDecodersA1 = 1;
            _basicDecodersA2 = _nandInputsStage1A - _basicDecodersA1;
            int loadsA1 = 1 << (3 * _basicDecodersA2);
            int loadsA2 = 1 << (2 + 3 * (_basicDecodersA2 - 1));

            return build("A1", _basicDecoderA1, 2, loadsA1 * nandCapacitanceStage1A, Ohm()) // TODO
                    && build("A2", _basicDecoderA2, 3, loadsA2 * nandCapacitanceStage1A, Ohm()); // TODO
        }

        if (decoders24 == 2) {
            if (_nandInputsStage1A == 2) {
                _basicDecodersA1 = 2;
                _basicDecodersA2 = 0;

                return build("A1", _basicDecoderA1, 2, 4 * nandCapacitanceStage1A, Ohm()); // TODO
            }

            _basicDecodersA1 = 2;
            _basicDecodersA2 = 1;

            return build("A1", _basicDecoderA1, 2, 32 * nandCapacitanceStage1A, Ohm()) // TODO
                    && build("A2", _basicDecoderA2, 3, 16 * nandCapacitanceStage1A, Ohm()); // TODO
        }

        return true;
    }

    _basicDecodersA1 = 1;
    _basicDecodersA2 = 0;
    if (decoders12 == 1) {
        return build("A1", _basicDecoderA1, 1, _loadCapacitance, _loadResistance);
    }

    if (decoders24 == 1) {
        return build("A1", _basicDecoderA1, 2, _loadCapacitance, _loadResistance);
    }

    if (decoders38 == 1) {
        return build("A1", _basicDecoderA1, 3, _loadCapacitance, _loadResistance);
    }

    return true;
}

void mse::component::PredecodeBlock::forEachSubcomponent(const SubcomponentVisitor& visit) noexcept {
    auto visitIfPresent = [&visit] (auto& decoder) { if (decoder) visit(*decoder); };

    visitIfPresent(_rowDecoderStage1A);
    visitIfPresent(_rowDecoderStage1B);
    visitIfPresent(_rowDecoderStage1C);
    visitIfPresent(_rowDecoderStage2);
    visitIfPresent(_basicDecoderA1);
    visitIfPresent(_basicDecoderA2);
    visitIfPresent(_basicDecoderB);
    visitIfPresent(_basicDecoderC);
}

void mse::component::PredecodeBlock::reset() noexcept {
    RampedComponent::reset();
    forEachSubcomponent([] (Component& subcomponent) noexcept { subcomponent.reset(); });
}

mse::Dimensions mse::component::PredecodeBlock::calculateDimensions() const noexcept {
    using namespace mse::unit;

    // Basic decoders are stacked, each one repeated once per address group it decodes
    Meter basicWidth;
    Meter basicHeight;
    const std::pair<const std::optional<BasicDecoder>*, int> basicDecoders[] = {
        {&_basicDecoderA1, _basicDecodersA1}, {&_basicDecoderA2, _basicDecodersA2},
        {&_basicDecoderB, _nandInputsStage1B}, {&_basicDecoderC, _nandInputsStage1C}
    };
    for (const auto& [decoder, count] : basicDecoders) {
        if (*decoder) {
            basicWidth = std::max(basicWidth, (*decoder)->dimensions().width);
            basicHeight += count * (*decoder)->dimensions().height;
        }
    }

    // Stage 1 row decoders are stacked, with the stage 2 row decoder beside them
    Meter rowWidth;
    Meter rowHeight;
    for (const std::optional<RowDecoder>* decoder : {&_rowDecoderStage1A, &_rowDecoderStage1B, &_rowDecoderStage1C}) {
        if (*decoder) {
            rowWidth = std::max(rowWidth, (*decoder)->dimensions().width);
            rowHeight += (*decoder)->dimensions().height;
        }
    }
    if (_rowDecoderStage2) {
        rowWidth += _rowDecoderStage2->dimensions().width;
        rowHeight = std::max(rowHeight, _rowDecoderStage2->dimensions().height);
    }

    return Dimensions{.width = basicWidth + rowWidth, .height = std::max(basicHeight, rowHeight)};
}

mse::component::RampedTiming mse::component::PredecodeBlock::calculateTiming(physics::HorowitzRamp inputRamp) const noexcept {
    using namespace mse::unit;
    using physics::HorowitzRamp;

    RampedTiming timing;
    timing.outputRamp = inputRamp;

    if (_addressBits == 0) {
        return timing;
    }

    struct Path {
        Second delay;
        HorowitzRamp ramp;

        Path& through(const RampedComponent<>& decoder) {
            decoder.setInputRamp(ramp);
            RampedTiming timing = decoder.timing();
            delay += timing.readLatency;
            ramp = timing.outputRamp;

            return *this;
        }
    };

    // The block is as slow as its slowest path and as sharp as its slowest output ramp
    Second latency;
    HorowitzRamp outputRamp;
    auto finish = [&latency, &outputRamp] (const Path& path) {
        latency = std::max(latency, path.delay);
        outputRamp = std::max(outputRamp, path.ramp);
    };

    if (_basicDecoderA1) {
        Path path{Second(), inputRamp};
        path.through(*_basicDecoderA1);
        if (_rowDecoderStage1A) {
            path.through(*_rowDecoderStage1A);
            if (_rowDecoderStage2) {
                path.through(*_rowDecoderStage2);
            }
        }
        finish(path);
    }
    if (_basicDecoderA2) {
        // The original model counts basic decoder A1 latency on this path
        _basicDecoderA2->setInputRamp(inputRamp);
        Path path{_basicDecoderA1->timing().readLatency, _basicDecoderA2->timing().outputRamp};
        path.through(*_rowDecoderStage1A);
        if (_rowDecoderStage2) {
            path.through(*_rowDecoderStage2);
        }
        finish(path);
    }
    if (_basicDecoderB) {
        finish(Path{Second(), inputRamp}.through(*_basicDecoderB).through(*_rowDecoderStage1B).through(*_rowDecoderStage2));
    }
    if (_basicDecoderC) {
        finish(Path{Second(), inputRamp}.through(*_basicDecoderC).through(*_rowDecoderStage1C).through(*_rowDecoderStage2));
    }

    timing.readLatency = timing.writeLatency = latency;
    timing.outputRamp = outputRamp;

    return timing;
}

mse::component::BasicPower mse::component::PredecodeBlock::calculatePower() const noexcept {
    using namespace mse::unit;

    BasicPower power;
    auto add = [&power] (const RampedComponent<>& decoder) {
        power.leakage += decoder.power().leakage;
        power.dynamicReadEnergy += decoder.power().dynamicReadEnergy;
    };
    for (const std::optional<BasicDecoder>* decoder : {&_basicDecoderA1, &_basicDecoderA2, &_basicDecoderB, &_basicDecoderC}) {
        if (*decoder) add(**decoder);
    }
    // The original model never calculated the stage 2 row decoder's power, so it adds nothing
    for (const std::optional<RowDecoder>* decoder : {&_rowDecoderStage1A, &_rowDecoderStage1B, &_rowDecoderStage1C}) {
        if (*decoder) add(**decoder);
    }
    power.dynamicWriteEnergy = power.dynamicReadEnergy;

    return power;
}
