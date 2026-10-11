#include "component/Buffer.hpp"

#include "unit.hpp"
#include "global.hpp"

struct BufferTable {
    mse::unit::Meter cellWidth;
    mse::unit::Meter cellHeight;
    mse::unit::Meter xorHeight;

    mse::unit::Second latency;
    mse::unit::Second xorLatency;

    mse::unit::Joule dynamicEnergy;
    mse::unit::Joule xorDynamicEnergy;

    mse::unit::Watt leakage;
    mse::unit::Watt xorLeakage;
};

static constexpr BufferTable node45 {
    .cellWidth = mse::unit::Meter(1.54375e-6),
    .cellHeight = mse::unit::Meter(1.52e-6),
    .xorHeight  = mse::unit::Meter(2.166e-6),

    .latency = mse::unit::Second(0.043e-9),
    .xorLatency = mse::unit::Second(0.381e-9),

    .dynamicEnergy = mse::unit::Joule(0.0002e-18),
    .xorDynamicEnergy = mse::unit::Joule(0.0034e-18),

    .leakage = mse::unit::Watt(19.7536e-9),
    .xorLeakage = mse::unit::Watt(45.1492e-9)
};

// Area scaled from 45nm, timing and power copied from 45nm
static constexpr BufferTable node32 {
    .cellWidth = mse::unit::Meter(1.098e-6),
    .cellHeight = mse::unit::Meter(1.081e-6),
    .xorHeight = mse::unit::Meter(1.54e-6),

    .latency = node45.latency,
    .xorLatency = node45.xorLatency,

    .dynamicEnergy = node45.dynamicEnergy,
    .xorDynamicEnergy = node45.xorDynamicEnergy,

    .leakage = node45.leakage,
    .xorLeakage = node45.xorLeakage
};

// Scaled from 45nm
static constexpr BufferTable node22 {
    .cellWidth = mse::unit::Meter(0.755e-6),
    .cellHeight = mse::unit::Meter(0.743e-6),
    .xorHeight  = mse::unit::Meter(1.059e-6),

    .latency = mse::unit::Second(0.024e-9),
    .xorLatency = mse::unit::Second(0.21e-9),

    .dynamicEnergy = mse::unit::Joule(0.00006e-18),
    .xorDynamicEnergy = mse::unit::Joule(0.001e-18),

    .leakage = mse::unit::Watt(45.796e-9),
    .xorLeakage = mse::unit::Watt(104.67e-9)
};

static const BufferTable& selectTable() {
    if (gTech.featureSizeInNano >= 44) {
        return node45;
    } else if (gTech.featureSizeInNano >= 31) {
        return node32;
    }
    return node22;
}

void mse::component::Buffer::report(Reporter& reporter) const noexcept {
    BasicComponent::report(reporter);

    const BufferTiming& t = timing();
    reporter.report(ReportGroup::Timing, "XOR Latency", t.xorLatency);

    const BufferPower& p = power();
    reporter.report(ReportGroup::Power, "XOR Dynamic Energy", p.xorDynamicEnergy);
    reporter.report(ReportGroup::Power, "XOR Leakage", p.xorLeakage);
}

mse::Dimensions mse::component::Buffer::calculateDimensions() const noexcept {
    const BufferTable& t = selectTable();

    // Columns counts data bits and control signals
    return Dimensions{
        .width = t.cellWidth * _columns,
        .height = (t.cellHeight + t.xorHeight) * _rows
    };
}

mse::component::BufferTiming mse::component::Buffer::calculateTiming() const noexcept {
    const BufferTable& t = selectTable();

    BufferTiming timing;
    timing.readLatency = timing.writeLatency = t.latency;
    timing.xorLatency = t.xorLatency;

    return timing;
}

mse::component::BufferPower mse::component::Buffer::calculatePower() const noexcept {
    const BufferTable& t = selectTable();

    BufferPower power;
    power.dynamicReadEnergy = power.dynamicWriteEnergy = t.dynamicEnergy * _columns;
    power.leakage = t.leakage * _columns;
    power.xorDynamicEnergy = t.xorDynamicEnergy * _columns;
    power.xorLeakage = t.xorLeakage * _columns;

    return power;
}
