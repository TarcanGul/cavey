#pragma once

#include <JuceHeader.h>
#include <boost/json.hpp>

#include "EnvironmentVariableProvider.h"
#include "HttpTransport.h"
#include "LLMController.h"

namespace Cavey {

class AnthropicController final : public LLMController {
public:
    AnthropicController(
            std::shared_ptr<HttpTransport> http_transport =
                    std::make_shared<JuceHttpTransport>(),
            std::shared_ptr<EnvironmentVariableProvider> environment =
                    std::make_shared<SystemEnvironmentVariableProvider>());

    juce::String prompt(const juce::String& prompt) override;
    ProviderConnectionResult connect(
            const ProviderConnectionConfig& config) override;
    ProviderMetadata metadata() const override;
    bool hasRequiredEnvironmentVariable() const override;

private:
    static constexpr const char* kApiKeyEnvironmentVariable = "ANTHROPIC_API_KEY";
    static constexpr const char* kModel = "claude-3-5-haiku-20241022";
    static constexpr const char* kMessagesUrl = "https://api.anthropic.com/v1/messages";
    static constexpr const char* kAnthropicVersion = "2023-06-01";

    std::optional<juce::String> loadApiKey() const;
    juce::String makeHeaders(const juce::String& api_key) const;
    boost::json::object makeRequestBody(const juce::String& prompt) const;

    std::shared_ptr<HttpTransport> http_transport_;
    std::shared_ptr<EnvironmentVariableProvider> environment_;
};

}  // namespace Cavey
