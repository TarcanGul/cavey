//
// Created by Tarcan Gul on 12/20/25.
//

#include "OllamaController.h"
#include <string>
#include <regex>
#include <boost/algorithm/string/trim.hpp>

namespace {

constexpr const char* kOllamaModelSettingKey = "ollamaModel";

juce::ApplicationProperties& GetApplicationProperties() {
    static juce::ApplicationProperties properties;
    static bool isConfigured = false;

    if (!isConfigured) {
        juce::PropertiesFile::Options options;
        options.applicationName = "Cavey";
        options.filenameSuffix = "settings";
        options.folderName = "Cavey";
        options.osxLibrarySubFolder = "Application Support";
        options.storageFormat = juce::PropertiesFile::storeAsXML;
        properties.setStorageParameters(options);
        isConfigured = true;
    }

    return properties;
}

juce::PropertiesFile* GetUserSettings() {
    return GetApplicationProperties().getUserSettings();
}

}  // namespace

OllamaController::OllamaController() {
    int dataSize = 0;
    const char * data = BinaryData::getNamedResource("SystemPrompt_md", dataSize);
    if (data && dataSize > 0) {
        systemPrompt = data;
        // systemPrompt.assign(data,  static_cast<size_t>(dataSize));
    } else {
        juce::Logger::writeToLog("Cannot read system prompt");
    }

    if (const auto* settings = GetUserSettings()) {
        selectedModel_ = settings->getValue(kOllamaModelSettingKey).trim();
    }
}

String OllamaController::prompt(const juce::String &prompt) {
    if (prompt.isEmpty()) {
        throw std::invalid_argument("Prompt cannot be empty");
    }
    if (selectedModel_.isEmpty()) {
        throw std::invalid_argument("Ollama model cannot be empty");
    }

    // Recreate connection every time (http 1.0). More reliable
    net::io_context ioc;
    boost::beast::tcp_stream stream {ioc};
    tcp::resolver resolver(ioc);
    auto const results = resolver.resolve(OLLAMA_HOST, OLLAMA_PORT);
    stream.connect(results);

    juce::Logger::writeToLog("Processing prompt");
    juce::Logger::writeToLog(prompt.toStdString());

    http::request<http::string_body> req{http::verb::post, std::string(Cavey::API_BASE).append("/generate"), Cavey::HTTP_VERSION};
    req.set(http::field::host, OLLAMA_HOST);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(http::field::content_type, "application/json");
    req.keep_alive(true);

    // Inject the system prompt
    std::string actualPrompt = std::regex_replace(systemPrompt, std::regex(R"(\{\{ USER_PROMPT \}\})"), prompt.toStdString());

    boost::json::object jsonBody;
    jsonBody["model"] = selectedModel_.toStdString();
    jsonBody["prompt"] = actualPrompt;
    jsonBody["stream"] = false;

    req.body() = boost::json::serialize(jsonBody);
    req.prepare_payload();

    http::write(stream, req);

    beast::flat_buffer responseBuffer;
    http::response<http::dynamic_body> response;

    http::read(stream, responseBuffer, response);

    // Read in the 'response' field. The http response will be JSON.
    const std::string body = beast::buffers_to_string(response.body().data());
    const auto parsed = boost::json::parse(body);
    const auto& obj = parsed.as_object();
    const auto it = obj.find("response");
    if (it == obj.end() || !it->value().is_string()) {
        throw std::runtime_error("Response JSON missing 'response' string field");
    }

    const auto& responseText = it->value().as_string();

    juce::Logger::writeToLog("Response is: ");
    juce::Logger::writeToLog(responseText.c_str());
    // Get the string within <json>...</json> tags and return only the inner JSON text.
    static constexpr const char* OPEN_TAG = "<json>";
    static constexpr const char* CLOSE_TAG = "</json>";
    const std::string respStr(responseText.c_str());

    const auto startPosRaw = respStr.find(OPEN_TAG);
    if (startPosRaw == std::string::npos) {
        throw std::runtime_error("Response missing <json> tag");
    }
    const auto startPos = startPosRaw + std::char_traits<char>::length(OPEN_TAG);
    const auto endPos = respStr.find(CLOSE_TAG, startPos);
    if (endPos == std::string::npos) {
        throw std::runtime_error("Response missing </json> tag");
    }

    auto jsonPayload = respStr.substr(startPos, endPos - startPos);
    jsonPayload = boost::algorithm::trim_copy(jsonPayload);
    stream.close();
    return { jsonPayload };
}

juce::StringArray OllamaController::fetchModels() {
    net::io_context ioc;
    boost::beast::tcp_stream stream {ioc};
    tcp::resolver resolver(ioc);
    auto const results = resolver.resolve(OLLAMA_HOST, OLLAMA_PORT);
    stream.connect(results);

    http::request<http::empty_body> req{
        http::verb::get,
        std::string(Cavey::API_BASE).append("/tags"),
        Cavey::HTTP_VERSION
    };
    req.set(http::field::host, OLLAMA_HOST);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.keep_alive(false);

    http::write(stream, req);

    beast::flat_buffer responseBuffer;
    http::response<http::dynamic_body> response;
    http::read(stream, responseBuffer, response);
    stream.close();

    if (response.result() != http::status::ok) {
        throw std::runtime_error("Ollama tags request failed");
    }

    const std::string body = beast::buffers_to_string(response.body().data());
    boost::system::error_code errorCode;
    const auto parsed = boost::json::parse(body, errorCode);
    if (errorCode || !parsed.is_object()) {
        throw std::runtime_error("Ollama tags response is invalid JSON");
    }

    const auto& object = parsed.as_object();
    const auto modelsIt = object.find("models");
    if (modelsIt == object.end() || !modelsIt->value().is_array()) {
        throw std::runtime_error("Ollama tags response is missing models");
    }

    juce::StringArray models;
    for (const auto& modelValue : modelsIt->value().as_array()) {
        if (!modelValue.is_object()) {
            continue;
        }

        const auto& modelObject = modelValue.as_object();
        const auto nameIt = modelObject.find("name");
        if (nameIt != modelObject.end() && nameIt->value().is_string()) {
            models.add(juce::String(nameIt->value().as_string().c_str()));
        }
    }

    models.removeEmptyStrings();
    models.removeDuplicates(false);
    return models;
}

juce::String OllamaController::getSelectedModel() const {
    return selectedModel_;
}

void OllamaController::setSelectedModel(const juce::String& model) {
    selectedModel_ = model.trim();
    if (auto* settings = GetUserSettings()) {
        settings->setValue(kOllamaModelSettingKey, selectedModel_);
        settings->saveIfNeeded();
    }
}

bool OllamaController::hasSelectedModel() const {
    return selectedModel_.isNotEmpty();
}
