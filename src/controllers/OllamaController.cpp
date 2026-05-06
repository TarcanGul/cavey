//
// Created by Tarcan Gul on 12/20/25.
//

#include "OllamaController.h"

#include <string>
#include <regex>

#include "ProviderResponseParser.h"

namespace Cavey {

OllamaController::OllamaController(std::shared_ptr<HttpTransport> http_transport)
    : http_transport_(std::move(http_transport)) {
    int dataSize = 0;
    const char * data = BinaryData::getNamedResource("SystemPrompt_md", dataSize);
    if (data && dataSize > 0) {
        system_prompt_.assign(data, static_cast<size_t>(dataSize));
    } else {
        juce::Logger::writeToLog("Cannot read system prompt");
    }
}

juce::String OllamaController::prompt(const juce::String& prompt) {
    if (prompt.isEmpty()) {
        throw std::invalid_argument("Prompt cannot be empty");
    }

    if (selected_model_.isEmpty()) {
        throw std::runtime_error("Choose an Ollama model before generating.");
    }

    juce::Logger::writeToLog("Processing prompt");

    const std::string actual_prompt = std::regex_replace(
            system_prompt_, std::regex(R"(\{\{ USER_PROMPT \}\})"),
            prompt.toStdString());

    boost::json::object jsonBody;
    jsonBody["model"] = selected_model_.toStdString();
    jsonBody["prompt"] = actual_prompt;
    jsonBody["stream"] = false;

    const auto response = http_transport_->send({
        .method = "POST",
        .url = juce::String(kBaseUrl) + "/api/generate",
        .headers = "Content-Type: application/json\r\n",
        .body = boost::json::serialize(jsonBody)
    });

    if (response.status_code < 200 || response.status_code >= 300) {
        throw std::runtime_error("Ollama could not generate a response.");
    }

    const auto parsed = boost::json::parse(response.body.toStdString());
    const auto& obj = parsed.as_object();
    const auto it = obj.find("response");
    if (it == obj.end() || !it->value().is_string()) {
        throw std::runtime_error("Ollama response did not include text content.");
    }

    const auto& responseText = it->value().as_string();
    return ExtractCoefficientJson(juce::String(responseText.c_str()));
}

ProviderConnectionResult OllamaController::connect(
        const ProviderConnectionConfig& config) {
    const juce::String model = config.ollama_model.trim();
    if (model.isEmpty()) {
        return {
            .connected = false,
            .message = "Choose a downloaded Ollama model."
        };
    }

    const auto models = fetchModels();
    if (!models.contains(model)) {
        return {
            .connected = false,
            .message = "Selected Ollama model is not available locally."
        };
    }

    selected_model_ = model;
    return {
        .connected = true,
        .message = "Connected to Ollama."
    };
}

ProviderMetadata OllamaController::metadata() const {
    return {
        .provider = AiProvider::kOllama,
        .id = ToProviderId(AiProvider::kOllama),
        .display_name = ToProviderDisplayName(AiProvider::kOllama),
        .model = selected_model_,
        .requires_api_key = false
    };
}

juce::StringArray OllamaController::fetchModels() {
    const auto response = http_transport_->send({
        .method = "GET",
        .url = juce::String(kBaseUrl) + "/api/tags"
    });

    if (response.status_code < 200 || response.status_code >= 300) {
        throw std::runtime_error("Ollama is not reachable at localhost:11434.");
    }

    boost::system::error_code error_code;
    const auto parsed = boost::json::parse(response.body.toStdString(), error_code);
    if (error_code || !parsed.is_object()) {
        throw std::runtime_error("Ollama returned an invalid model list.");
    }

    const auto& object = parsed.as_object();
    const auto models_it = object.find("models");
    if (models_it == object.end() || !models_it->value().is_array()) {
        throw std::runtime_error("Ollama response did not include downloaded models.");
    }

    juce::StringArray models;
    for (const auto& model : models_it->value().as_array()) {
        if (!model.is_object()) {
            continue;
        }

        const auto& model_object = model.as_object();
        const auto name_it = model_object.find("name");
        if (name_it != model_object.end() && name_it->value().is_string()) {
            models.add(name_it->value().as_string().c_str());
        }
    }

    if (models.isEmpty()) {
        throw std::runtime_error("No downloaded Ollama models were found.");
    }

    return models;
}

}  // namespace Cavey
