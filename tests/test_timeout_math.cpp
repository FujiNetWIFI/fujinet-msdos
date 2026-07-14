#include "doctest.h"

#include <cstdint>

namespace {

bool timeout_reached(std::uint16_t start_tick, std::uint16_t current_tick,
                     std::uint16_t timeout_ticks)
{
    return static_cast<std::uint16_t>(current_tick - start_tick) >= timeout_ticks;
}

} // namespace

TEST_CASE("16-bit BIOS tick timeout arithmetic works across wraparound")
{
    CHECK_FALSE(timeout_reached(1000, 1049, 50));
    CHECK(timeout_reached(1000, 1050, 50));

    CHECK_FALSE(timeout_reached(0xfff0, 0x000f, 32));
    CHECK(timeout_reached(0xfff0, 0x0010, 32));
}
