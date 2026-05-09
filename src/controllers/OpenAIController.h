#pragma once

#include <JuceHeader.h>
#include <boost/json.hpp>

#include "EnvironmentVariableProvider.h"
#include "HttpTransport.h"
#include "LLMController.h"

namespace Cavey {

class OpenAIController final : public LLMController {
public:
    OpenAIController(
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
    static constexpr const char* kApiKeyEnvironmentVariable = "OPENAI_API_KEY";
    static constexpr const char* kModel = "gpt-4.1-mini";
    static constexpr const char* kResponsesUrl = "https://api.openai.com/v1/responses";

    std::optional<juce::String> loadApiKey() const;
    juce::String makeHeaders(const juce::String& api_key) const;
    boost::json::object makeRequestBody(const juce::String& prompt) const;

    std::shared_ptr<HttpTransport> http_transport_;
    std::shared_ptr<EnvironmentVariableProvider> environment_;
};

}  // namespace Cavey
