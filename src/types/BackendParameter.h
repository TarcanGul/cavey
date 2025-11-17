//
// Created by Tarcan Gul on 11/16/25.
//

#pragma once

#include <JuceHeader.h>
#include "CaveyTypes.h"

class BackendParameter {
public:
    BackendParameter() = default;
    juce::String getName();
    juce::AudioParameterFloat * getParameterValue();

    void setName(juce::String name);
    void setParameterValue(juce::AudioParameterFloat *  parameterValue);
    void setCharacteristicCoefficients(std::map<BaseEffect, float> coefficients);

    float getBaseEffectValue(BaseEffect effect);
private:
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
};