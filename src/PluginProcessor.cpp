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
{}

CaveyAudioProcessor::~CaveyAudioProcessor() = default;

void CaveyAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    DBG("Prepare to play called");

    juce::dsp::ProcessSpec spec = {
            .sampleRate = sampleRate,
            .maximumBlockSize = static_cast<unsigned int>(samplesPerBlock),
            .numChannels = static_cast<unsigned int>(getTotalNumOutputChannels())
    };

    processorChain.prepare(spec);

    auto& lowPassFilter = processorChain.get<lowPassFilterIndex>();
    lowPassFilter.setMode(juce::dsp::LadderFilterMode::LPF24);
    lowPassFilter.setCutoffFrequencyHz(20000);
    lowPassFilter.setResonance (0.7f);

    auto& highPassFilter = processorChain.get<highPassFilterIndex>();
    highPassFilter.setMode(juce::dsp::LadderFilterMode::HPF24);
    highPassFilter.setCutoffFrequencyHz(20);
    highPassFilter.setResonance (0.7f);

    auto& distortion = processorChain.get<distortionIndex>();
    distortion.functionToUse = [](float x) { return std::tanh(x); };

    processorChain.reset();
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
    // Collect target cutoff and gain from parameters (use first/only for now)
    float targetLowPassCutoffHz = lastCutoffHz;
    float targetHighPassCutoffHz = 20.0f;
    float targetGain = lastTargetGain;
    float targetReverbWetLevel = 0;
    float targetDistortionLevel = 0;
    for (const auto& parameterValue : parameters) {
        BackendParameter* parameter = parameterValue.second;
        if (auto lowPassValue = parameter->getBaseEffectValue(BaseEffect::LOW_PASS)) {
            targetLowPassCutoffHz = *lowPassValue;
        }
        if (auto highPassValue = parameter->getBaseEffectValue(BaseEffect::HIGH_PASS)) {
            targetHighPassCutoffHz = *highPassValue;
        }
        if (auto gainValue = parameter->getBaseEffectValue(BaseEffect::VOLUME)) {
            targetGain = *gainValue;
        }
        if (auto reverbValue = parameter->getBaseEffectValue(BaseEffect::REVERB)) {
            targetReverbWetLevel = *reverbValue;
        }
        if (auto distortionValue = parameter->getBaseEffectValue(BaseEffect::DISTORTION)) {
            targetDistortionLevel = *distortionValue;
        }
    }

    // Apply parameter updates before processing
    lastCutoffHz = targetLowPassCutoffHz;
    lastTargetGain = targetGain;
    auto& lowPassFilter = processorChain.get<lowPassFilterIndex>();
    lowPassFilter.setCutoffFrequencyHz(targetLowPassCutoffHz);

    auto& highPassFilter = processorChain.get<highPassFilterIndex>();
    highPassFilter.setCutoffFrequencyHz(targetHighPassCutoffHz);

    auto& gain = processorChain.get<gainIndex>();
    gain.setGainLinear(targetGain);

    auto& reverb = processorChain.get<reverbIndex>();
    reverb.setParameters({
        .wetLevel = targetReverbWetLevel
    });

    auto& drive = processorChain.get<driveIndex>();
    drive.setGainLinear(juce::Decibels::decibelsToGain(targetDistortionLevel * 24.0f));

    juce::dsp::AudioBlock<float> audioBlock(buffer);
    juce::dsp::ProcessContextReplacing<float> context(audioBlock);
    processorChain.process(context);
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
