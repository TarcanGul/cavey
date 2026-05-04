#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "effects/CaveyEffects.h"

TEST_CASE("Volume coefficient maps to current gain range", "[effects]") {
    const auto range = Cavey::Volume(0.25f).getRange();

    REQUIRE(range.low == Catch::Approx(0.75f));
    REQUIRE(range.high == Catch::Approx(1.0f));
}

TEST_CASE("Low-pass coefficient maps to current cutoff range", "[effects]") {
    const auto range = Cavey::LowPass(0.5f).getRange();

    REQUIRE(range.low == Catch::Approx(20000.0f));
    REQUIRE(range.high == Catch::Approx(10050.0f));
}

TEST_CASE("High-pass coefficient maps to current cutoff range", "[effects]") {
    const auto range = Cavey::HighPass(0.5f).getRange();

    REQUIRE(range.low == Catch::Approx(20.0f));
    REQUIRE(range.high == Catch::Approx(1000.0f));
}

TEST_CASE("Reverb coefficient maps to current wet-level range", "[effects]") {
    const auto range = Cavey::Reverb(0.6f).getRange();

    REQUIRE(range.low == Catch::Approx(0.0f));
    REQUIRE(range.high == Catch::Approx(0.6f));
}

TEST_CASE("Distortion coefficient maps to current drive range", "[effects]") {
    const auto range = Cavey::Distortion(0.7f).getRange();

    REQUIRE(range.low == Catch::Approx(0.0f));
    REQUIRE(range.high == Catch::Approx(0.7f));
}

TEST_CASE("Chorus coefficient maps to current mix range", "[effects]") {
    const auto range = Cavey::Chorus(0.8f).getRange();

    REQUIRE(range.low == Catch::Approx(0.0f));
    REQUIRE(range.high == Catch::Approx(0.8f));
}
