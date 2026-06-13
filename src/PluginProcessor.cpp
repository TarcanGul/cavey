#include "PluginProcessor.h"

#include <cmath>
#include <optional>
#include <utility>

#include "PluginEditor.h"
#include "controllers/OllamaController.h"

namespace {

GenerateParameterResult MakeGenerationFailure(const juce::String& errorMessage) {
    juce::Logger::writeToLog("Parameter generation failed: " + errorMessage);
    return {
        .success = false,
        .parameterName = {},
        .errorMessage = errorMessage
    };
}

std::optional<float> ReadJsonNumber(const boost::json::value& value) {
    if (value.is_double()) {
        return static_cast<float>(value.as_double());
    }

    if (value.is_int64()) {
        return static_cast<float>(value.as_int64());
    }

    if (value.is_uint64()) {
        return static_cast<float>(value.as_uint64());
    }

    return std::nullopt;
}

std::optional<juce::String> ReadRequiredString(
    const boost::json::object& object,
    const char* fieldName,
    juce::String* errorMessage) {
    const auto field = object.find(fieldName);
    if (field == object.end()) {
        *errorMessage = juce::String("LLM response missing required field: ") + fieldName;
        return std::nullopt;
    }

    if (!field->value().is_string()) {
        *errorMessage = juce::String("LLM response field must be a string: ") + fieldName;
        return std::nullopt;
    }

    const juce::String stringValue(field->value().as_string().c_str());
    if (stringValue.trim().isEmpty()) {
        *errorMessage = juce::String("LLM response field cannot be empty: ") + fieldName;
        return std::nullopt;
    }

    return stringValue;
}

std::optional<float> ReadRequiredNumber(
    const boost::json::object& object,
    const char* fieldName,
    juce::String* errorMessage) {
    const auto field = object.find(fieldName);
    if (field == object.end()) {
        *errorMessage = juce::String("LLM response missing required field: ") + fieldName;
        return std::nullopt;
    }

    const auto number = ReadJsonNumber(field->value());
    if (!number.has_value()) {
        *errorMessage = juce::String("LLM response field must be numeric: ") + fieldName;
        return std::nullopt;
    }

    return number;
}

std::optional<float> ReadOptionalNumber(
    const boost::json::object& object,
    const char* fieldName,
    float defaultValue,
    juce::String* errorMessage) {
    const auto field = object.find(fieldName);
    if (field == object.end()) {
        return defaultValue;
    }

    const auto number = ReadJsonNumber(field->value());
    if (!number.has_value()) {
        *errorMessage = juce::String("LLM response field must be numeric: ") + fieldName;
        return std::nullopt;
    }

    return number;
}

}  // namespace

CaveyAudioProcessor::CaveyAudioProcessor()
    : CaveyAudioProcessor(std::make_unique<OllamaController>())
{
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

GenerateParameterResult CaveyAudioProcessor::addCaveyParameter(const juce::String& prompt) {
    try {
        const juce::String response = this->llm_->prompt(prompt);

        boost::system::error_code errorCode;
        const boost::json::value readResponse = boost::json::parse(response.toStdString(), errorCode);
        if (errorCode) {
            return MakeGenerationFailure(
                "LLM response is invalid JSON: " + juce::String(errorCode.message()));
        }

        if (!readResponse.is_object()) {
            return MakeGenerationFailure("LLM response must be a JSON object.");
        }

        const boost::json::object& parsedResponse = readResponse.as_object();
        juce::String errorMessage;
        const auto parameterName = ReadRequiredString(parsedResponse, "NAME", &errorMessage);
        if (!parameterName.has_value()) {
            return MakeGenerationFailure(errorMessage);
        }

        const auto volume = ReadRequiredNumber(parsedResponse, "VOLUME", &errorMessage);
        if (!volume.has_value()) {
            return MakeGenerationFailure(errorMessage);
        }

        const auto lowPass = ReadRequiredNumber(parsedResponse, "LOW_PASS", &errorMessage);
        if (!lowPass.has_value()) {
            return MakeGenerationFailure(errorMessage);
        }

        const auto highPass = ReadRequiredNumber(parsedResponse, "HIGH_PASS", &errorMessage);
        if (!highPass.has_value()) {
            return MakeGenerationFailure(errorMessage);
        }

        const auto reverb = ReadRequiredNumber(parsedResponse, "REVERB", &errorMessage);
        if (!reverb.has_value()) {
            return MakeGenerationFailure(errorMessage);
        }

        const auto distortion = ReadRequiredNumber(parsedResponse, "DISTORTION", &errorMessage);
        if (!distortion.has_value()) {
            return MakeGenerationFailure(errorMessage);
        }

        const auto chorus = ReadOptionalNumber(parsedResponse, "CHORUS", 0.0f, &errorMessage);
        if (!chorus.has_value()) {
            return MakeGenerationFailure(errorMessage);
        }

        this->addBackendParameter(*parameterName, {
            { Cavey::BaseEffect::VOLUME, *volume },
            { Cavey::BaseEffect::LOW_PASS, *lowPass },
            { Cavey::BaseEffect::HIGH_PASS, *highPass },
            { Cavey::BaseEffect::REVERB, *reverb },
            { Cavey::BaseEffect::DISTORTION, *distortion },
            { Cavey::BaseEffect::CHORUS, *chorus }
        });

        sendActionMessage(*parameterName);
        return {
            .success = true,
            .parameterName = *parameterName,
            .errorMessage = {}
        };
    } catch (const std::exception& exception) {
        return MakeGenerationFailure(exception.what());
    } catch (...) {
        return MakeGenerationFailure("Unknown generation error.");
    }
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

juce::StringArray CaveyAudioProcessor::fetchOllamaModels() {
    if (auto* ollamaController = dynamic_cast<OllamaController*>(llm_.get())) {
        return ollamaController->fetchModels();
    }

    return {};
}

juce::String CaveyAudioProcessor::getSelectedOllamaModel() const {
    if (const auto* ollamaController = dynamic_cast<const OllamaController*>(llm_.get())) {
        return ollamaController->getSelectedModel();
    }

    return {};
}

void CaveyAudioProcessor::setSelectedOllamaModel(const juce::String& model) {
    if (auto* ollamaController = dynamic_cast<OllamaController*>(llm_.get())) {
        ollamaController->setSelectedModel(model);
    }
}

bool CaveyAudioProcessor::hasSelectedOllamaModel() const {
    if (const auto* ollamaController = dynamic_cast<const OllamaController*>(llm_.get())) {
        return ollamaController->hasSelectedModel();
    }

    return true;
}

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CaveyAudioProcessor();
}
