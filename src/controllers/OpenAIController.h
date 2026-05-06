#pragma once

#include <JuceHeader.h>
#include <boost/json.hpp>

#include "CredentialStore.h"
#include "HttpTransport.h"
#include "LLMController.h"

namespace Cavey {

class OpenAIController final : public LLMController {
public:
    OpenAIController(
            std::shared_ptr<HttpTransport> http_transport =
                    std::make_shared<JuceHttpTransport>(),
            std::shared_ptr<CredentialStore> credential_store =
                    std::make_shared<SystemCredentialStore>());

    juce::String prompt(const juce::String& prompt) override;
    ProviderConnectionResult connect(
            const ProviderConnectionConfig& config) override;
    ProviderMetadata metadata() const override;
    bool hasStoredCredential() const override;

private:
    static constexpr const char* kModel = "gpt-4.1-mini";
    static constexpr const char* kResponsesUrl = "https://api.openai.com/v1/responses";

    std::optional<juce::String> loadApiKey() const;
    juce::String makeHeaders(const juce::String& api_key) const;
    boost::json::object makeRequestBody(const juce::String& prompt) const;

    std::shared_ptr<HttpTransport> http_transport_;
    std::shared_ptr<CredentialStore> credential_store_;
};

}  // namespace Cavey
