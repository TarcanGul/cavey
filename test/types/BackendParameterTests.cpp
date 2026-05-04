#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "types/BackendParameter.h"

TEST_CASE("Backend parameter name can be set and retrieved", "[types]") {
    juce::AudioParameterFloat parameter("generated", "generated", 0.0f, 1.0f, 0.0f);
    BackendParameter backendParameter(parameter);

    backendParameter.setName("Warmth");

    REQUIRE(backendParameter.getName() == "Warmth");
}

TEST_CASE("Backend parameter interpolates base effect values", "[types]") {
    juce::AudioParameterFloat parameter("generated", "generated", 0.0f, 1.0f, 0.0f);
    BackendParameter backendParameter(parameter);
    backendParameter.setCharacteristicCoefficients({
        .volume = 0.4f,
        .lowPass = 0.5f,
        .highPass = 0.5f,
        .reverb = 0.6f,
        .distortion = 0.8f,
        .chorus = 0.2f,
    });

    parameter.setValueNotifyingHost(0.5f);

    REQUIRE(backendParameter.getBaseEffectValue(Cavey::BaseEffect::VOLUME).value()
            == Catch::Approx(0.8f));
    REQUIRE(backendParameter.getBaseEffectValue(Cavey::BaseEffect::LOW_PASS).value()
            == Catch::Approx(15025.0f));
    REQUIRE(backendParameter.getBaseEffectValue(Cavey::BaseEffect::HIGH_PASS).value()
            == Catch::Approx(510.0f));
    REQUIRE(backendParameter.getBaseEffectValue(Cavey::BaseEffect::REVERB).value()
            == Catch::Approx(0.3f));
    REQUIRE(backendParameter.getBaseEffectValue(Cavey::BaseEffect::DISTORTION).value()
            == Catch::Approx(0.4f));
    REQUIRE(backendParameter.getBaseEffectValue(Cavey::BaseEffect::CHORUS).value()
            == Catch::Approx(0.1f));
}
