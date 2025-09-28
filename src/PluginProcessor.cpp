#include "PluginProcessor.h"
#include "PluginEditor.h"

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
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that don't have input data
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* CaveyAudioProcessor::createEditor()
{
    return new CaveyAudioProcessorEditor(*this);
}

void CaveyAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::ignoreUnused(destData);
}

void CaveyAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    juce::ignoreUnused(data, sizeInBytes);
}

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CaveyAudioProcessor();
}

