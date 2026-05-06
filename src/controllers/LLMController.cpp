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
        case AiProvider::kCustom:
            return "custom";
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
        case AiProvider::kCustom:
            return "Custom";
        case AiProvider::kNone:
            return "No provider";
    }

    return "No provider";
}

}  // namespace Cavey
