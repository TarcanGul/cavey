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
    explicit BackendParameter(const juce::AudioParameterFloat& parameterValue);
    ~BackendParameter() = default;
    juce::String getName() const;

    void setName(const juce::String& name);
    void setCharacteristicCoefficients(const Cavey::CoefficientGroupInitializer& init);

    std::optional<float> getBaseEffectValue(Cavey::BaseEffect effect);
    const juce::AudioParameterFloat& getParameter();
private:
    // effects
    Cavey::Volume volume { 0 } ;
    Cavey::LowPass lowPass {0 };
    Cavey::HighPass highPass {0 };
    Cavey::Reverb reverb {0 };
    Cavey::Distortion distortion {0 };
    Cavey::Chorus chorus {0};

    void calculateRanges();

    juce::String name {};
    const juce::AudioParameterFloat& parameterValue;

    std::map<Cavey::BaseEffect, Cavey::Range> baseEffectRanges{};
};
