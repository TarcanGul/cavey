//
// Created by Tarcan Gul on 11/16/25.
//
#include "BackendParameter.h"
#include <optional>
#include <utility>

void BackendParameter::setName(juce::String pName) {
    this->name = std::move(pName);
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

    float firstValue = baseEffectRanges[effect].first;
    float secondValue = baseEffectRanges[effect].second;

    // If values are equal
    if (firstValue <= secondValue + EPSILON && firstValue >= secondValue - EPSILON) {
        return firstValue;
    }

    return (firstValue + secondValue) * parameterValue->get();
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
    baseEffectRanges[BaseEffect::ATTACK] = getAttackRange(characteristicCoefficients[BaseEffect::ATTACK]);
    baseEffectRanges[BaseEffect::DECAY] = getDecayRange(characteristicCoefficients[BaseEffect::DECAY]);
    baseEffectRanges[BaseEffect::SUSTAIN] = getSustainRange(characteristicCoefficients[BaseEffect::SUSTAIN]);
    baseEffectRanges[BaseEffect::RELEASE] = getReleaseRange(characteristicCoefficients[BaseEffect::RELEASE]);
}

// coefficient range = 0-1
std::pair<float, float> BackendParameter::getVolumeRange(float coefficient) {
    // 0 -> {1.0 , 1.0}
    // 1 -> {0.0, 1.0}
    return {1.0 - coefficient, 1.0};
}

std::pair<float, float> BackendParameter::getPitchRange(float coefficient) {
    // Semitones
    // Next step, go through scale instead of every integer
    return {0, static_cast<int>(24 * coefficient)};
}

std::pair<float, float> BackendParameter::getAttackRange(float coefficient) {
    // Default value is 0.1
    // So 0 -> { 0.1, 0.1 }
    // 1 should map to {0.1, 1}
    return { 0.1 ,  0.1 + (coefficient * 100) };
}

std::pair<float, float> BackendParameter::getDecayRange(float coefficient) {
    // Default value is 0.1
    // So 0 -> { 0.1, 0.1 }
    // 1 should map to {0.1, 1}
    return {0.1 , coefficient + 0.1 };
}

std::pair<float, float> BackendParameter::getSustainRange(float coefficient) {
    // Default value is 1.0 for sustain
    // So 0 -> { 1.0, 1.0 }
    // 1 should map to {1.0, 5}
    return { 1.0 , 1 + (coefficient * 100) };
}

std::pair<float, float> BackendParameter::getReleaseRange(float coefficient) {
    // Default value is 0.1
    // So 0 -> { 0.1, 0.1 }
    // 1 should map to {0.1, 1}
    return {0.1 , coefficient + 0.1 };
}

void BackendParameter::setParameterValue(float value) {
    * parameterValue = value;
}
