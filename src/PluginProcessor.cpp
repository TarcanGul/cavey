#include "PluginProcessor.h"

#include <utility>
#include "PluginEditor.h"
#include <cmath>
#include "controllers/OllamaController.h"

CaveyAudioProcessor::CaveyAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
#endif
{
    llm = std::make_unique<OllamaController>();

    logger.reset(juce::FileLogger::createDefaultAppLogger("Cavey", "cavey.log", "Welcome to Cavey!"));
    juce::Logger::setCurrentLogger(logger.get());
    juce::Logger::writeToLog("Audio processor is initiated.");
}

CaveyAudioProcessor::~CaveyAudioProcessor() {
    juce::Logger::setCurrentLogger(nullptr);
    logger.reset();
};

void CaveyAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    juce::Logger::writeToLog("Prepare to play called");

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

void CaveyAudioProcessor::addBackendParameter(const juce::String& parameterName, const std::map<Cavey::BaseEffect, float>& coefficients) {
    juce::Logger::writeToLog("Backend parameter" + parameterName.toStdString() + "is being added");
    auto * newBackendParameterValue = new juce::AudioParameterFloat(parameterName, parameterName, 0.0f, 1.0f, 0.0f);
    auto * newBackendParameter = new BackendParameter(newBackendParameterValue);
    newBackendParameter->setName(parameterName);
    newBackendParameter->setCharacteristicCoefficients({
        .volume = coefficients.at(Cavey::BaseEffect::VOLUME),
        .lowPass = coefficients.at(Cavey::BaseEffect::LOW_PASS),
        .highPass = coefficients.at(Cavey::BaseEffect::HIGH_PASS),
        .reverb = coefficients.at(Cavey::BaseEffect::REVERB),
        .distortion = coefficients.at(Cavey::BaseEffect::DISTORTION),
    });
    parameters.insert({ parameterName, newBackendParameter } );
    addParameter(newBackendParameterValue);
}

void CaveyAudioProcessor::setBackendParameterValue(const juce::String& parameterName, float value) {
    auto parameter = parameters.at(parameterName);
    if (parameter == nullptr) {
        juce::Logger::writeToLog(parameterName + " cannot be found in the parameters map!");
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
        if (auto lowPassValue = parameter->getBaseEffectValue(Cavey::BaseEffect::LOW_PASS)) {
            targetLowPassCutoffHz = *lowPassValue;
        }
        if (auto highPassValue = parameter->getBaseEffectValue(Cavey::BaseEffect::HIGH_PASS)) {
            targetHighPassCutoffHz = *highPassValue;
        }
        if (auto gainValue = parameter->getBaseEffectValue(Cavey::BaseEffect::VOLUME)) {
            targetGain = *gainValue;
        }
        if (auto reverbValue = parameter->getBaseEffectValue(Cavey::BaseEffect::REVERB)) {
            targetReverbWetLevel = *reverbValue;
        }
        if (auto distortionValue = parameter->getBaseEffectValue(Cavey::BaseEffect::DISTORTION)) {
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

void CaveyAudioProcessor::addCaveyParameter(const juce::String& prompt) {
    // Do this part async
    const juce::String response = this->llm->prompt(prompt);

    // Eval here.

    boost::system::error_code errorCode;
    const boost::json::value readResponse = boost::json::parse(response.toStdString(), errorCode);
    const boost::json::object parsedResponse = readResponse.as_object();
    juce::String parameterName = juce::String(parsedResponse.at("NAME").get_string().c_str());

    this->addBackendParameter( parameterName, {
            { Cavey::BaseEffect::VOLUME, parsedResponse.at("VOLUME").get_double() },
            { Cavey::BaseEffect::LOW_PASS, parsedResponse.at("LOW_PASS").get_double() },
            { Cavey::BaseEffect::HIGH_PASS, parsedResponse.at("HIGH_PASS").get_double() },
            { Cavey::BaseEffect::REVERB, parsedResponse.at("REVERB").get_double() },
            { Cavey::BaseEffect::DISTORTION, parsedResponse.at("DISTORTION").get_double()}
    });

    sendActionMessage(parameterName);
}

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CaveyAudioProcessor();
}
