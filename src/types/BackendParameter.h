//
// Created by Tarcan Gul on 11/16/25.
//

#pragma once

#include <JuceHeader.h>
#include "CaveyTypes.h"

/**
 * The actual parameter that determines the functionality
 */
class BackendParameter {
public:
    explicit BackendParameter(juce::AudioParameterFloat * parameterValue);
    ~BackendParameter();
    juce::String getName();
    juce::AudioParameterFloat * getParameterValue();

    void setName(juce::String name);
    void setCharacteristicCoefficients(std::map<BaseEffect, float> coefficients);

    std::optional<float> getBaseEffectValue(BaseEffect effect);
private:
    void calculateRanges();

    std::pair<float, float> getVolumeRange(float actualValue);
    std::pair<float, float> getPitchRange(float actualValue);

    juce::String name;
    juce::AudioParameterFloat * parameterValue;
    std::map<BaseEffect, float> characteristicCoefficients = {
            {BaseEffect::VOLUME, 0.0f},
            {BaseEffect::PITCH, 0.0f},
            {BaseEffect::ATTACK, 0.0f},
            {BaseEffect::DECAY, 0.0f},
            {BaseEffect::SUSTAIN, 0.0f},
            {BaseEffect::RELEASE, 0.0f},
    };

    std::map<BaseEffect, std::pair<float, float>> baseEffectRanges = {};
};