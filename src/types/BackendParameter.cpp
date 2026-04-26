//
// Created by Tarcan Gul on 11/16/25.
//
#include "BackendParameter.h"
#include <optional>
#include <utility>

BackendParameter::BackendParameter(const juce::AudioParameterFloat& parameterValue) : parameterValue(parameterValue) {}

void BackendParameter::setName(const juce::String& pName) {
    this->name = pName;
}

void BackendParameter::setCharacteristicCoefficients(const Cavey::CoefficientGroupInitializer& init) {

    this->volume.coefficient = init.volume;
    this->lowPass.coefficient = init.lowPass;
    this->highPass.coefficient = init.highPass;
    this->reverb.coefficient = init.reverb;
    this->distortion.coefficient = init.distortion;
    calculateRanges();
}

/**
 * This method will give the value for that effect given the current parameter value and coefficients.
 * Currently assumes first value is max,
 * @param effect
 * @return
 */
std::optional<float> BackendParameter::getBaseEffectValue(Cavey::BaseEffect effect) {
    if (!baseEffectRanges.contains(effect)) {
        return {};
    }

    float firstValue = baseEffectRanges.at(effect).low;
    float secondValue = baseEffectRanges.at(effect).high;

    // If values are equal
    if (juce::approximatelyEqual(firstValue, secondValue)) {
        return firstValue;
    }

    return firstValue - ((firstValue - secondValue) * parameterValue);
}

juce::String BackendParameter::getName() {
    return this->name;
}

void BackendParameter::calculateRanges() {
    baseEffectRanges.insert_or_assign(Cavey::BaseEffect::VOLUME, volume.getRange());
    baseEffectRanges.insert_or_assign(Cavey::BaseEffect::LOW_PASS, lowPass.getRange());
    baseEffectRanges.insert_or_assign(Cavey::BaseEffect::HIGH_PASS, highPass.getRange());
    baseEffectRanges.insert_or_assign(Cavey::BaseEffect::REVERB, reverb.getRange());
    baseEffectRanges.insert_or_assign(Cavey::BaseEffect::DISTORTION, distortion.getRange());
}

const juce::AudioParameterFloat& BackendParameter::getParameter() {
    return parameterValue;
}
