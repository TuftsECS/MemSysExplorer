#ifndef MSE_DIMENSIONS_HPP
#define MSE_DIMENSIONS_HPP

#include "unit.hpp"

#include <algorithm>
#include <iostream>

namespace mse {

struct Dimensions {
    unit::Meter width;
    unit::Meter height;

    unit::MeterExp<2> area() const {
        return width * height;
    }

    Dimensions beside(const Dimensions& other) const {
        return {width + other.width, std::max(height, other.height)};
    }

    Dimensions above(const Dimensions& other) const {
        return {std::max(width, other.width), height + other.height};
    }
};

inline std::ostream& operator<<(std::ostream& stream, const Dimensions& d) {
    return stream << d.area() << " (" << d.width << " x " << d.height << ")";
}

} // namespace mse

#endif
