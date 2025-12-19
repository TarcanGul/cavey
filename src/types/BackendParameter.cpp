//
// Created by Tarcan Gul on 11/16/25.
//
#include "BackendParameter.h"
#include <optional>
#include <utility>

BackendParameter::BackendParameter(juce::AudioParameterFloat *parameterValue) : parameterValue(parameterValue) {
    // juce::File logFile("~/logs.txt");
    // fileLogger = std::make_unique<juce::FileLogger>(std::move(logFile), juce::String("BackendParameter"));
}

void BackendParameter::setName(juce::String pName) {
    this->name = std::move(pName);
}

void BackendParameter::setCharacteristicCoefficients(std::map<BaseEffect, float> coefficients) {
    this->characteristicCoefficients = std::move(coefficients);
    calculateRanges();
}

/**
 * This method will give the value for that effect given the current parameter value and coefficients.
 * Currently assumes first value is max,
 * @param effect
 * @return
 */
std::optional<float> BackendParameter::getBaseEffectValue(BaseEffect effect) {
    juce::String log1;
    if (!baseEffectRanges.contains(effect)) {
        return {};
    }

    float firstValue = baseEffectRanges[effect].first;
    float secondValue = baseEffectRanges[effect].second;

    // If values are equal
    if (std::abs(firstValue - secondValue) <= EPSILON) {
        return firstValue;
    }

    return firstValue - ((firstValue - secondValue) * parameterValue->get());
}

juce::String BackendParameter::getName() {
    return this->name;
}


juce::AudioParameterFloat *BackendParameter::getParameterValue() {
    return parameterValue;
}

BackendParameter::~BackendParameter() {
    delete parameterValue;
}

void BackendParameter::calculateRanges() {
    baseEffectRanges[BaseEffect::VOLUME] = getVolumeRange(characteristicCoefficients[BaseEffect::VOLUME]);
    baseEffectRanges[BaseEffect::LOW_PASS] = getLowPassRange(characteristicCoefficients[BaseEffect::LOW_PASS]);
    baseEffectRanges[BaseEffect::HIGH_PASS] = getHighPassRange(characteristicCoefficients[BaseEffect::HIGH_PASS]);
}

// coefficient range = 0-1
std::pair<float, float> BackendParameter::getVolumeRange(float coefficient) {
    // 0 -> {1.0 , 1.0}
    // 1 -> {0.0, 1.0}
    return {1.0 - coefficient, 1.0};
}

// This is going to give Hz.
std::pair<float, float> BackendParameter::getLowPassRange(float coefficient) {
    // 0 -> {20000 , 20000}
    // 1 -> {20000, 1000}
    return {20000, 20000 - (19900 * coefficient)};
}

// This is going to give Hz.
std::pair<float, float> BackendParameter::getHighPassRange(float coefficient) {
    // 0 -> {0 , 0}
    // 1 -> {0, 20000}
    return {0, (2000 * coefficient)};
}

void BackendParameter::setParameterValue(float value) {
     * parameterValue = value;
}
