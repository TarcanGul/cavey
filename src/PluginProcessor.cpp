#include "PluginProcessor.h"

#include <utility>
#include "PluginEditor.h"
#include <cmath>
#include "controllers/AnthropicController.h"
#include "controllers/OllamaController.h"
#include "controllers/OpenAIController.h"

CaveyAudioProcessor::CaveyAudioProcessor()
    :
#ifndef JucePlugin_PreferredChannelConfigurations
        AudioProcessor(BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
        apvts_(*this, nullptr, juce::Identifier("Cavey"), {})
#endif
{
    logger_.reset(juce::FileLogger::createDefaultAppLogger("Cavey", "cavey.log", "Welcome to Cavey!"));
    juce::Logger::setCurrentLogger(logger_.get());
    juce::Logger::writeToLog("Audio processor is initiated.");
}

CaveyAudioProcessor::CaveyAudioProcessor(std::unique_ptr<LLMController> llmController)
    :
#ifndef JucePlugin_PreferredChannelConfigurations
        AudioProcessor(BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
        apvts_(*this, nullptr, juce::Identifier("Cavey"), {})
#endif
{
    llm_ = std::move(llmController);
    activeProvider_ = Cavey::AiProvider::kCustom;
    isProviderConnected_ = llm_ != nullptr;

    logger_.reset(juce::FileLogger::createDefaultAppLogger("Cavey", "cavey.log", "Welcome to Cavey!"));
    juce::Logger::setCurrentLogger(logger_.get());
    juce::Logger::writeToLog("Audio processor is initiated.");
}

CaveyAudioProcessor::~CaveyAudioProcessor() {
    juce::Logger::setCurrentLogger(nullptr);
    logger_.reset();
};

void CaveyAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    juce::Logger::writeToLog("Prepare to play called");

    juce::dsp::ProcessSpec spec = {
            .sampleRate = sampleRate,
            .maximumBlockSize = static_cast<unsigned int>(samplesPerBlock),
            .numChannels = static_cast<unsigned int>(getTotalNumOutputChannels())
    };

    processorChain_.prepare(spec);

    auto& lowPassFilter = processorChain_.get<lowPassFilterIndex>();
    lowPassFilter.setMode(juce::dsp::LadderFilterMode::LPF24);
    lowPassFilter.setCutoffFrequencyHz(20000);
    lowPassFilter.setResonance (0.7f);

    auto& highPassFilter = processorChain_.get<highPassFilterIndex>();
    highPassFilter.setMode(juce::dsp::LadderFilterMode::HPF24);
    highPassFilter.setCutoffFrequencyHz(20);
    highPassFilter.setResonance (0.7f);

    auto& distortion = processorChain_.get<distortionIndex>();
    distortion.functionToUse = [](float x) { return std::tanh(x); };

    auto& chorus = processorChain_.get<chorusIndex>();
    chorus.setCentreDelay(10);
    chorus.setRate(1);
    chorus.setDepth(0.5);
    chorus.setFeedback(0.0);

    processorChain_.reset();
}

void CaveyAudioProcessor::releaseResources() {}

void CaveyAudioProcessor::addBackendParameter(const juce::String& parameterName, const std::map<Cavey::BaseEffect, float>& coefficients) {
    juce::Logger::writeToLog("Backend parameter" + parameterName.toStdString() + "is being added");
    auto newBackendParameterValue = std::make_unique<juce::AudioParameterFloat>(parameterName, parameterName, 0.0f, 1.0f, 0.0f);
    apvts_.createAndAddParameter(std::move(newBackendParameterValue));
    const juce::AudioParameterFloat * paramRef = dynamic_cast<juce::AudioParameterFloat *>(apvts_.getParameter(parameterName));
    if (paramRef == nullptr) {
        throw std::runtime_error("paramRef is null");
    }
    auto newBackendParameter = std::make_unique<BackendParameter>(*paramRef);

    newBackendParameter->setName(parameterName);
    newBackendParameter->setCharacteristicCoefficients({
        .volume = coefficients.at(Cavey::BaseEffect::VOLUME),
        .lowPass = coefficients.at(Cavey::BaseEffect::LOW_PASS),
        .highPass = coefficients.at(Cavey::BaseEffect::HIGH_PASS),
        .reverb = coefficients.at(Cavey::BaseEffect::REVERB),
        .distortion = coefficients.at(Cavey::BaseEffect::DISTORTION),
        .chorus = coefficients.at(Cavey::BaseEffect::CHORUS)
    });

    generatedParameter_ = std::move(newBackendParameter);
}

void CaveyAudioProcessor::setBackendParameterValue(const juce::String& parameterName, float value) {
    auto parameter = apvts_.getParameter(parameterName);
    if (parameter == nullptr) {
        juce::Logger::writeToLog(parameterName + " cannot be found in the generatedParameter_ map!");
        throw std::invalid_argument(parameterName.toStdString() + " cannot be found in the generatedParameter_ map!");
    }
    parameter->setValue(value);
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
    if (!generatedParameter_) {
        return;
    }
    BackendParameter* parameter = generatedParameter_.get();
    float targetLowPassCutoffHz = lastCutoffHz_;
    float targetHighPassCutoffHz = 20.0f;
    float targetGain = lastTargetGain_;
    float targetReverbWetLevel {0.0f};
    float targetDistortionLevel {0.0f};
    float targetChorus {0.0f};

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
    if (auto chorusValue = parameter->getBaseEffectValue(Cavey::BaseEffect::CHORUS)) {
        targetChorus = *chorusValue;
    }

    lastCutoffHz_ = targetLowPassCutoffHz;
    lastTargetGain_ = targetGain;

    auto& lowPassFilter = processorChain_.get<lowPassFilterIndex>();
    lowPassFilter.setCutoffFrequencyHz(targetLowPassCutoffHz);

    auto& highPassFilter = processorChain_.get<highPassFilterIndex>();
    highPassFilter.setCutoffFrequencyHz(targetHighPassCutoffHz);

    auto& gain = processorChain_.get<gainIndex>();
    gain.setGainLinear(targetGain);

    auto& reverb = processorChain_.get<reverbIndex>();
    reverb.setParameters({
        .wetLevel = targetReverbWetLevel
    });

    auto& drive = processorChain_.get<driveIndex>();
    drive.setGainLinear(juce::Decibels::decibelsToGain(targetDistortionLevel * 24.0f));

    auto& chorus = processorChain_.get<chorusIndex>();
    chorus.setMix(targetChorus);

    juce::dsp::AudioBlock<float> audioBlock(buffer);
    juce::dsp::ProcessContextReplacing<float> context(audioBlock);
    processorChain_.process(context);
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
    if (!isProviderConnected_ || llm_ == nullptr) {
        throw std::runtime_error("Set up your AI provider before generating.");
    }

    // Do this part async
    const juce::String response = this->llm_->prompt(prompt);

    boost::system::error_code errorCode;
    const boost::json::value readResponse = boost::json::parse(response.toStdString(), errorCode);
    if (errorCode || !readResponse.is_object()) {
        throw std::runtime_error("AI response did not include usable parameter JSON.");
    }

    const boost::json::object parsedResponse = readResponse.as_object();
    juce::String parameterName = juce::String(parsedResponse.at("NAME").get_string().c_str());

    this->addBackendParameter( parameterName, {
            { Cavey::BaseEffect::VOLUME, parsedResponse.at("VOLUME").get_double() },
            { Cavey::BaseEffect::LOW_PASS, parsedResponse.at("LOW_PASS").get_double() },
            { Cavey::BaseEffect::HIGH_PASS, parsedResponse.at("HIGH_PASS").get_double() },
            { Cavey::BaseEffect::REVERB, parsedResponse.at("REVERB").get_double() },
            { Cavey::BaseEffect::DISTORTION, parsedResponse.at("DISTORTION").get_double()},
            { Cavey::BaseEffect::CHORUS, parsedResponse.contains("CHORUS") ? parsedResponse.at("CHORUS").get_double() : 0.0}
    });

    sendActionMessage(parameterName);
}

juce::AudioProcessorValueTreeState& CaveyAudioProcessor::getValueTree() {
    return apvts_;
}

bool CaveyAudioProcessor::hasGeneratedParameter() const noexcept {
    return generatedParameter_ != nullptr;
}

juce::String CaveyAudioProcessor::getGeneratedParameterName() const {
    if (generatedParameter_ == nullptr) {
        return {};
    }

    return generatedParameter_->getName();
}

void CaveyAudioProcessor::clearGeneratedParameter() noexcept {
    generatedParameter_.reset();
}

Cavey::ProviderConnectionResult CaveyAudioProcessor::connectAiProvider(
        Cavey::AiProvider provider,
        const Cavey::ProviderConnectionConfig& config) {
    auto controller = makeProviderController(provider);
    if (controller == nullptr) {
        return {
            .connected = false,
            .message = "Unknown AI provider."
        };
    }

    auto result = controller->connect(config);
    if (!result.connected) {
        return result;
    }

    llm_ = std::move(controller);
    activeProvider_ = provider;
    isProviderConnected_ = true;
    sendActionMessage("AI_PROVIDER_CONNECTED");
    return result;
}

juce::StringArray CaveyAudioProcessor::fetchOllamaModels() {
    auto controller = makeProviderController(Cavey::AiProvider::kOllama);
    if (controller == nullptr) {
        return {};
    }

    return controller->fetchModels();
}

bool CaveyAudioProcessor::isAiProviderConnected() const noexcept {
    return isProviderConnected_;
}

Cavey::AiProvider CaveyAudioProcessor::getActiveProvider() const noexcept {
    return activeProvider_;
}

juce::String CaveyAudioProcessor::getActiveProviderName() const {
    return Cavey::ToProviderDisplayName(activeProvider_);
}

bool CaveyAudioProcessor::hasStoredCredential(Cavey::AiProvider provider) const {
    auto controller = makeProviderController(provider);
    return controller != nullptr && controller->hasStoredCredential();
}

std::unique_ptr<LLMController> CaveyAudioProcessor::makeProviderController(
        Cavey::AiProvider provider) const {
    switch (provider) {
        case Cavey::AiProvider::kOpenAI:
            return std::make_unique<Cavey::OpenAIController>();
        case Cavey::AiProvider::kAnthropic:
            return std::make_unique<Cavey::AnthropicController>();
        case Cavey::AiProvider::kOllama:
            return std::make_unique<Cavey::OllamaController>();
        case Cavey::AiProvider::kCustom:
        case Cavey::AiProvider::kNone:
            break;
    }

    return nullptr;
}

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CaveyAudioProcessor();
}
