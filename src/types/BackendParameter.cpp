//
// Created by Tarcan Gul on 11/16/25.
//
#include "BackendParameter.h"
#include <optional>

void BackendParameter::setName(juce::String pName) {
    this->name = pName;
}

void BackendParameter::setCharacteristicCoefficients(std::map<BaseEffect, float> coefficients) {
    this->characteristicCoefficients = std::move(coefficients);
    calculateRanges();
}

/**
 * This method will give the value for that effect given the current parameter value and coefficients.
 * @param effect
 * @return
 */
std::optional<float> BackendParameter::getBaseEffectValue(BaseEffect effect) {
    if (!baseEffectRanges.contains(effect)) {
        return {};
    }

    float min = baseEffectRanges[effect].first;
    float max = baseEffectRanges[effect].second;

    return min + (parameterValue->get() * (max - min));
}

juce::String BackendParameter::getName() {
    return this->name;
}


juce::AudioParameterFloat *BackendParameter::getParameterValue() {
    return parameterValue;
}

BackendParameter::BackendParameter(juce::AudioParameterFloat *parameterValue) : parameterValue(parameterValue) {}

BackendParameter::~BackendParameter() {
    delete parameterValue;
}

void BackendParameter::calculateRanges() {
    baseEffectRanges[BaseEffect::VOLUME] = getVolumeRange(characteristicCoefficients[BaseEffect::VOLUME]);
    baseEffectRanges[BaseEffect::PITCH] = getPitchRange(characteristicCoefficients[BaseEffect::PITCH]);
}

std::pair<float, float> BackendParameter::getVolumeRange(float actualValue) {
    return {1 - actualValue / 2, 1 + actualValue / 2};
}

std::pair<float, float> BackendParameter::getPitchRange(float actualValue) {
    // Semitones
    return {0, 24 * actualValue};
}
