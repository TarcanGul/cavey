//
// Created by Tarcan Gul on 11/16/25.
//
#include "BackendParameter.h"

void BackendParameter::setName(juce::String name) {
    this->name = name;
}

void BackendParameter::setParameterValue(juce::AudioParameterFloat * parameterValue) {
    this->parameterValue = parameterValue;
}

void BackendParameter::setCharacteristicCoefficients(std::map<BaseEffect, float> coefficients) {
    this->characteristicCoefficients = coefficients;
}

/**
 * This method will give the value for that effect given the current parameter value and coefficients.
 * @param effect
 * @return
 */
float BackendParameter::getBaseEffectValue(BaseEffect effect) {
    // TODO: ???
}

juce::String BackendParameter::getName() {
    return this->name;
}


juce::AudioParameterFloat *BackendParameter::getParameterValue() {
    return parameterValue;
}
