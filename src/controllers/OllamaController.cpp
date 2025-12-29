//
// Created by Tarcan Gul on 12/20/25.
//

#include "OllamaController.h"
#include <string>
#include <regex>
#include <boost/algorithm/string/trim.hpp>

OllamaController::OllamaController()
    : stream(ioc), systemPromptMarkdown("../Resources/SystemPrompt.md", std::ios::in) {
    // Could move this to a static factory
    tcp::resolver resolver(ioc);
    auto const results = resolver.resolve(OLLAMA_HOST, OLLAMA_PORT);
    stream.connect(results);

    if (systemPromptMarkdown.is_open()) {
        std::string line;
        while(std::getline(systemPromptMarkdown, line)) {
            systemPrompt += (line + '\n');
        }
        systemPromptMarkdown.close();
    } else {
        std::cout << "File not opening." << std::endl;
    }
}

String OllamaController::prompt(const juce::String &prompt) {
    if (prompt.isEmpty()) {
        throw std::invalid_argument("Prompt cannot be empty");
    }

    http::request<http::string_body> req{http::verb::post, std::string(API_BASE).append("/generate"), HTTP_VERSION};
    req.set(http::field::host, OLLAMA_HOST);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(http::field::content_type, "application/json");

    // Inject the system prompt
    std::string actualPrompt = std::regex_replace(systemPrompt, std::regex(R"(\{\{ USER_PROMPT \}\})"), prompt.toStdString());

    boost::json::object jsonBody;
    jsonBody["model"] = OLLAMA_MODEL;
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

    std::cout << jsonPayload << std::endl;
    return { jsonPayload };
}
