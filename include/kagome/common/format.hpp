#pragma once

// Use fmt consistently across the project
// We already link against fmt via CMake, so use it everywhere for consistency

#include <fmt/format.h>

namespace kagome {
    using fmt::format;
}