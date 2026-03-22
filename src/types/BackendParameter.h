//
// Created by Tarcan Gul on 11/16/25.
//

#pragma once

#include <JuceHeader.h>
#include "CaveyTypes.h"
#include "../effects/CaveyEffects.h"

/**
 * The actual parameter that determines the functionality
 */
class BackendParameter {
public:
    explicit BackendParameter(juce::AudioParameterFloat * parameterValue);
    ~BackendParameter();
    juce::String getName();
    juce::AudioParameterFloat * getParameterValue();
    void setParameterValue(float value);

    void setName(juce::String name);
    void setCharacteristicCoefficients(const Cavey::CoefficientGroupInitializer& init);

    std::optional<double> getBaseEffectValue(Cavey::BaseEffect effect);
private:
    constexpr static float EPSILON = 1e-9;

    // effects
    Cavey::Volume volume { 0 } ;
    Cavey::LowPass lowPass {0 };
    Cavey::HighPass highPass {0 };
    Cavey::Reverb reverb {0 };
    Cavey::Distortion distortion {0 };

    void calculateRanges();

    juce::String name;
    juce::AudioParameterFloat * parameterValue;
    std::map<Cavey::BaseEffect, float> characteristicCoefficients = {
            {Cavey::BaseEffect::VOLUME, 0.0f},
            {Cavey::BaseEffect::LOW_PASS, 0.0f},
            { Cavey::BaseEffect:: HIGH_PASS, 0.0f},
            {Cavey::BaseEffect::REVERB, 0.0f},
            { Cavey::BaseEffect::DISTORTION, 0.0f}
    };

    std::map<Cavey::BaseEffect, Cavey::Range> baseEffectRanges = {};
};