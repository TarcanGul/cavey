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
 * @param effect
 * @return
 */
std::optional<float> BackendParameter::getBaseEffectValue(BaseEffect effect) {
    juce::String log1;
    // fileLogger->logMessage(log1 << "Calculating: " << BASE_EFFECT_STRINGS.at(effect));
    if (!baseEffectRanges.contains(effect)) {
        return {};
    }

    float firstValue = baseEffectRanges[effect].first;
    float secondValue = baseEffectRanges[effect].second;

    juce::String log2;
    // fileLogger->logMessage(log2 << "First value is: " << firstValue << '\n');
    // fileLogger->logMessage(log2 << "Second value is: " << secondValue << '\n');

    // If values are equal
    if (firstValue <= secondValue + EPSILON && firstValue >= secondValue - EPSILON) {
        juce::String log3;
        // fileLogger->logMessage("approximately equal");
        return firstValue;
    }

    float max = std::max(firstValue, secondValue);
    float min = std::min(firstValue, secondValue);

    return min + ((max - min) * parameterValue->get());
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
}

// coefficient range = 0-1
std::pair<float, float> BackendParameter::getVolumeRange(float coefficient) {
    // 0 -> {1.0 , 1.0}
    // 1 -> {0.0, 1.0}
    return {1.0 - coefficient, 1.0};
}

void BackendParameter::setParameterValue(float value) {
    * parameterValue = value;
}
