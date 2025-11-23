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
    // TODO: do parameter id yourself
    auto * newBackendParameterValue = new juce::AudioParameterFloat(parameterName, parameterName, 0.0f, 1.0f, 0.8f);
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
    *parameter->getParameterValue() = value;
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
        }

        std::optional<float> pitchValue = parameter->getBaseEffectValue(BaseEffect::PITCH);
        if (pitchValue.has_value()) {
            const int numChannels = buffer.getNumChannels();
            const int numSamples  = buffer.getNumSamples();
            if (numSamples > 0) {
                // Interpret pitchValue as semitones, convert to playback rate ratio
                const float semitones = pitchValue.value();
                const float ratio = std::pow(2.0f, semitones / 12.0f);

                // Copy input to a temp buffer for safe in-place processing
                juce::AudioBuffer<float> source(numChannels, numSamples);
                for (int ch = 0; ch < numChannels; ++ch)
                    source.copyFrom(ch, 0, buffer, ch, 0, numSamples);

                for (int ch = 0; ch < numChannels; ++ch) {
                    float readPos = 0.0f;
                    for (int n = 0; n < numSamples; ++n) {
                        const int i0 = static_cast<int>(readPos);
                        const int i1 = (i0 + 1) % numSamples; // wrap for simple continuity
                        const float frac = readPos - static_cast<float>(i0);

                        const float s0 = source.getSample(ch, i0);
                        const float s1 = source.getSample(ch, i1);
                        const float out = s0 + (s1 - s0) * frac; // linear interpolation

                        buffer.setSample(ch, n, out);

                        readPos += ratio;
                        // wrap to stay within the source buffer bounds
                        while (readPos >= static_cast<float>(numSamples))
                            readPos -= static_cast<float>(numSamples);
                    }
                }
            }
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
