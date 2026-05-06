#include "OpenAIController.h"

#include <regex>

#include "BinaryData.h"
#include "ProviderResponseParser.h"

namespace Cavey {
namespace {

std::string GetSystemPrompt() {
    int data_size = 0;
    const char* data = BinaryData::getNamedResource("SystemPrompt_md", data_size);
    if (data == nullptr || data_size <= 0) {
        throw std::runtime_error("System prompt could not be loaded.");
    }

    return std::string(data, static_cast<size_t>(data_size));
}

}  // namespace

OpenAIController::OpenAIController(
        std::shared_ptr<HttpTransport> http_transport,
        std::shared_ptr<CredentialStore> credential_store)
    : http_transport_(std::move(http_transport)),
      credential_store_(std::move(credential_store)) {}

juce::String OpenAIController::prompt(const juce::String& prompt) {
    if (prompt.trim().isEmpty()) {
        throw std::invalid_argument("Prompt cannot be empty.");
    }

    const auto api_key = loadApiKey();
    if (!api_key.has_value()) {
        throw std::runtime_error("Connect OpenAI before generating.");
    }

    const auto response = http_transport_->send({
        .method = "POST",
        .url = kResponsesUrl,
        .headers = makeHeaders(*api_key),
        .body = boost::json::serialize(makeRequestBody(prompt))
    });

    if (response.status_code < 200 || response.status_code >= 300) {
        throw std::runtime_error("OpenAI could not generate a response.");
    }

    boost::system::error_code error_code;
    const auto parsed = boost::json::parse(response.body.toStdString(), error_code);
    if (error_code) {
        throw std::runtime_error("OpenAI returned an invalid response.");
    }

    return ExtractCoefficientJson(ExtractOpenAIText(parsed));
}

ProviderConnectionResult OpenAIController::connect(
        const ProviderConnectionConfig& config) {
    const auto api_key = config.api_key.trim();
    if (api_key.isEmpty()) {
        return {
            .connected = false,
            .message = "OpenAI API key cannot be empty."
        };
    }

    const auto response = http_transport_->send({
        .method = "POST",
        .url = kResponsesUrl,
        .headers = makeHeaders(api_key),
        .body = boost::json::serialize(makeRequestBody("connection test"))
    });

    if (response.status_code < 200 || response.status_code >= 300) {
        return {
            .connected = false,
            .message = "Could not connect to OpenAI with that API key."
        };
    }

    juce::String error_message;
    if (!credential_store_->saveSecret(ToProviderId(AiProvider::kOpenAI),
                                       api_key,
                                       &error_message)) {
        return {
            .connected = false,
            .message = error_message.isNotEmpty()
                    ? error_message
                    : "Could not save OpenAI API key securely."
        };
    }

    return {
        .connected = true,
        .message = "Connected to OpenAI."
    };
}

ProviderMetadata OpenAIController::metadata() const {
    return {
        .provider = AiProvider::kOpenAI,
        .id = ToProviderId(AiProvider::kOpenAI),
        .display_name = ToProviderDisplayName(AiProvider::kOpenAI),
        .model = kModel,
        .requires_api_key = true
    };
}

bool OpenAIController::hasStoredCredential() const {
    return credential_store_->hasSecret(ToProviderId(AiProvider::kOpenAI));
}

std::optional<juce::String> OpenAIController::loadApiKey() const {
    return credential_store_->loadSecret(ToProviderId(AiProvider::kOpenAI));
}

juce::String OpenAIController::makeHeaders(const juce::String& api_key) const {
    return "Content-Type: application/json\r\nAuthorization: Bearer "
            + api_key + "\r\n";
}

boost::json::object OpenAIController::makeRequestBody(
        const juce::String& prompt) const {
    const auto actual_prompt = std::regex_replace(
            GetSystemPrompt(), std::regex(R"(\{\{ USER_PROMPT \}\})"),
            prompt.toStdString());

    boost::json::object body;
    body["model"] = kModel;
    body["input"] = actual_prompt;
    return body;
}

}  // namespace Cavey
