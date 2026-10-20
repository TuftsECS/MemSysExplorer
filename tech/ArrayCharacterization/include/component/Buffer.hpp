#ifndef MSE_COMPONENT_BUFFER_HPP
#define MSE_COMPONENT_BUFFER_HPP

#include "component/BasicComponent.hpp"
#include "component/BasicValues.hpp"
#include "Dimensions.hpp"
#include "unit.hpp"

namespace mse::component {

struct BufferTiming : BasicTiming {
    unit::Second xorLatency;
};

struct BufferPower : BasicPower {
    unit::Joule xorDynamicEnergy;
    unit::Watt xorLeakage;
};

class Buffer final : public BasicComponent<BufferTiming, BufferPower> {
private:

    long long _rows = 0;
    long long _columns = 0;

public:

    Buffer() = default;

    Buffer(long long rows, long long columns)
            : _rows(rows),
              _columns(columns) {}

    constexpr std::string_view componentName() const noexcept override { return "Buffer"; }

    void report(Reporter& reporter) const noexcept override;

    Dimensions calculateDimensions() const noexcept override;
    BufferTiming calculateTiming() const noexcept override;
    BufferPower calculatePower() const noexcept override;
};

} // namespace mse::component

#endif
