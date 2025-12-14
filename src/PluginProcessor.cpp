#include "PluginProcessor.h"

#include <utility>
#include "PluginEditor.h"
#include <cmath>
#include <format>

CaveyAudioProcessor::CaveyAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
#endif
{
    juce::File logFile("~/logs.txt");
    // fileLogger = std::make_unique<juce::FileLogger>(std::move(logFile), juce::String("Logs are active"));
}

CaveyAudioProcessor::~CaveyAudioProcessor() = default;

void CaveyAudioProcessor::prepareToPlay(double, int) {
    PRINT("Prepare to play called");
    adsrEnvelope.setSampleRate(getSampleRate());
    adsrEnvelope.setParameters(adsrParameters);
}

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
            juce::String log;
            buffer.applyGain(gainValue.value());
        } else {
            juce::String log;
            buffer.applyGain(1.0);
        }
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
