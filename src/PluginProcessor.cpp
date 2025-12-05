#include "PluginProcessor.h"

#include <utility>
#include "PluginEditor.h"
#include <cmath>

CaveyAudioProcessor::CaveyAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
#endif
{
}

CaveyAudioProcessor::~CaveyAudioProcessor() = default;

void CaveyAudioProcessor::prepareToPlay(double, int) {}

void CaveyAudioProcessor::releaseResources() {}

void CaveyAudioProcessor::addBackendParameter(const juce::String& parameterName, std::map<BaseEffect, float> coefficients) {
    auto * newBackendParameterValue = new juce::AudioParameterFloat(parameterName, parameterName, 0.0f, 1.0f, 0.0f);
    auto * newBackendParameter = new BackendParameter(newBackendParameterValue);
    newBackendParameter->setName(parameterName);
    newBackendParameter->setCharacteristicCoefficients(std::move(coefficients));
    parameters.insert({ parameterName, newBackendParameter } );
    addParameter(newBackendParameterValue);
}

void CaveyAudioProcessor::setBackendParameterValue(const juce::String& parameterName, float value) {
    auto parameter = parameters.at(parameterName);
    if (parameter == nullptr) {
        PRINT(parameterName + " cannot be found in the parameters map!");
        throw std::invalid_argument(parameterName.toStdString() + " cannot be found in the parameters map!");
    }
    parameter->setParameterValue(value);
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool CaveyAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Only allow mono or stereo, and ensure the input/output layout match
    const auto& mainOut = layouts.getMainOutputChannelSet();
    const auto& mainIn  = layouts.getMainInputChannelSet();

    if (mainOut.isDisabled())
        return false;

    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (mainOut != mainIn)
        return false;
   #endif

    return true;
}
#endif

void CaveyAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    for (const auto& parameterValue : parameters) {
        const juce::String parameterName = parameterValue.first;
        BackendParameter * parameter = parameterValue.second;

        std::optional<float> gainValue = parameter->getBaseEffectValue(BaseEffect::VOLUME);
        if (gainValue.has_value()) {
            buffer.applyGain(gainValue.value());
        } else {
            buffer.applyGain(1.0);
        }

        std::optional<float> pitchValue = parameter->getBaseEffectValue(BaseEffect::PITCH);
        if (pitchValue.has_value()) {
            // Do nothing for now, implement later
        }

        juce::ADSR::Parameters adsrParameters;

        std::optional<float> attackValue = parameter->getBaseEffectValue(BaseEffect::ATTACK);
        if (attackValue.has_value()) {
            adsrParameters.attack = attackValue.value();
        }

        std::optional<float> decayValue = parameter->getBaseEffectValue(BaseEffect::DECAY);
        if (decayValue.has_value()) {
            adsrParameters.decay = decayValue.value();
        }

        std::optional<float> sustainValue = parameter->getBaseEffectValue(BaseEffect::SUSTAIN);
        if (sustainValue.has_value()) {
            adsrParameters.sustain = sustainValue.value();
        }

        std::optional<float> releaseValue = parameter->getBaseEffectValue(BaseEffect::RELEASE);
        if (releaseValue.has_value()) {
            adsrParameters.release = releaseValue.value();
        }

        juce::ADSR adsrEnvelope;
        adsrEnvelope.reset();
        adsrEnvelope.setSampleRate(getSampleRate());
        adsrEnvelope.setParameters(adsrParameters);
        adsrEnvelope.noteOn();
        adsrEnvelope.applyEnvelopeToBuffer(buffer, 0, buffer.getNumSamples());
    }
}

juce::AudioProcessorEditor* CaveyAudioProcessor::createEditor()
{
    return new CaveyAudioProcessorEditor(*this);
}

// To save state to disk
void CaveyAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::ignoreUnused(destData);
}

// To retrieve state from disk
void CaveyAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    juce::ignoreUnused(data, sizeInBytes);
}

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CaveyAudioProcessor();
}
