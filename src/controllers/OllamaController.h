//
// Created by Tarcan Gul on 12/20/25.
//

#pragma once

#include <JuceHeader.h>
#include <boost/json.hpp>
#include <string>

#include "HttpTransport.h"
#include "LLMController.h"
#include "BinaryData.h"

namespace Cavey {

class OllamaController : public LLMController {
public:
    explicit OllamaController(
            std::shared_ptr<HttpTransport> http_transport =
                    std::make_shared<JuceHttpTransport>());

    juce::String prompt(const juce::String& prompt) override;
    ProviderConnectionResult connect(
            const ProviderConnectionConfig& config) override;
    ProviderMetadata metadata() const override;
    juce::StringArray fetchModels() override;
    AiProvider getProvider() override;

private:
    static constexpr const char* kBaseUrl = "http://localhost:11434";

    std::shared_ptr<HttpTransport> http_transport_;
    std::string system_prompt_;
    juce::String selected_model_;
};

}  // namespace Cavey
