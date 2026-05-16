#include "LLMController.h"

namespace Cavey {


juce::String ToProviderId(AiProvider provider) {
    switch (provider) {
        case AiProvider::kOpenAI:
            return "openai";
        case AiProvider::kAnthropic:
            return "anthropic";
        case AiProvider::kOllama:
            return "ollama";
        case AiProvider::kNone:
            return "none";
    }

    return "none";
}

juce::String ToProviderDisplayName(AiProvider provider) {
    switch (provider) {
        case AiProvider::kOpenAI:
            return "OpenAI";
        case AiProvider::kAnthropic:
            return "Anthropic";
        case AiProvider::kOllama:
            return "Ollama";
        case AiProvider::kNone:
            return "No provider";
    }

    return "No provider";
}

AiProvider ToAiProvider(const juce::String& provider_id) {
    const auto normalized_provider_id = provider_id.trim().toLowerCase();
    if (normalized_provider_id == "openai") {
        return AiProvider::kOpenAI;
    }
    if (normalized_provider_id == "anthropic") {
        return AiProvider::kAnthropic;
    }
    if (normalized_provider_id == "ollama") {
        return AiProvider::kOllama;
    }

    return AiProvider::kNone;
}

}  // namespace Cavey

Cavey::AiProvider LLMController::getProvider() {
    return Cavey::AiProvider::kNone;
}
