//
// Created by Tarcan Gul on 11/16/25.
//
#include "BackendParameter.h"
#include <optional>
#include <utility>

BackendParameter::BackendParameter(juce::AudioParameterFloat *parameterValue) : parameterValue(parameterValue) {}

void BackendParameter::setName(juce::String pName) {
    this->name = std::move(pName);
}

void BackendParameter::setCharacteristicCoefficients(const Cavey::CoefficientGroupInitializer& init) {

    this->volume = { init.volume };
    this->lowPass = { init.lowPass }; ;
    this->highPass = { init.highPass }; ;
    this->reverb = { init.reverb };
    this->distortion = { init.distortion };
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

    float firstValue = baseEffectRanges[effect].low;
    float secondValue = baseEffectRanges[effect].high;

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
    baseEffectRanges[Cavey::BaseEffect::VOLUME] = volume.getRange();
    baseEffectRanges[Cavey::BaseEffect::LOW_PASS] = lowPass.getRange();
    baseEffectRanges[Cavey::BaseEffect::HIGH_PASS] = highPass.getRange();
    baseEffectRanges[Cavey::BaseEffect::REVERB] = reverb.getRange();
    baseEffectRanges[Cavey::BaseEffect::DISTORTION] = distortion.getRange();
}

void BackendParameter::setParameterValue(float value) {
     * parameterValue = value;
}
