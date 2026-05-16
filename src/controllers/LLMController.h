//
// Created by Tarcan Gul on 11/6/25.
//

#pragma once

#include <JuceHeader.h>

namespace Cavey {

enum class AiProvider {
    kNone,
    kOpenAI,
    kAnthropic,
    kOllama
};

struct ProviderMetadata {
    AiProvider provider = AiProvider::kNone;
    juce::String id;
    juce::String display_name;
    juce::String model;
    bool requires_api_key = false;
};

struct ProviderConnectionConfig {
    juce::String ollama_model;
};

struct ProviderConnectionResult {
    bool connected = false;
    juce::String message;
};

juce::String ToProviderId(AiProvider provider);
juce::String ToProviderDisplayName(AiProvider provider);
AiProvider ToAiProvider(const juce::String& provider_id);

}  // namespace Cavey

class LLMController {
public:
    virtual juce::String prompt(const juce::String& prompt) = 0;
    virtual Cavey::AiProvider getProvider();
    virtual Cavey::ProviderConnectionResult connect(
            const Cavey::ProviderConnectionConfig& config) = 0;
    virtual Cavey::ProviderMetadata metadata() const = 0;
    virtual bool hasRequiredEnvironmentVariable() const { return false; }
    virtual juce::StringArray fetchModels() { return {}; }
    virtual ~LLMController() = default;
};
