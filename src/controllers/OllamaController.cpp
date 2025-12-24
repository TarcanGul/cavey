//
// Created by Tarcan Gul on 12/20/25.
//

#include "OllamaController.h"
#include <string>
#include <regex>

OllamaController::OllamaController()
    : stream(ioc), systemPromptMarkdown("../Resources/SystemPrompt.md", std::ios::in) {
    // Could move this to a static factory
    tcp::resolver resolver(ioc);
    auto const results = resolver.resolve(OLLAMA_HOST, OLLAMA_PORT);
    stream.connect(results);

    std::ostringstream output_str;

    if (systemPromptMarkdown.is_open()) {
        std::string line;
        while(std::getline(systemPromptMarkdown, line)) {
            systemPrompt += (line + '\n');
        }
        systemPromptMarkdown.close();
    } else {
        std::cout << "File not opening?" << std::endl;
    }

    std::cout << "System Prompt: " << systemPrompt << std::endl;
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
    std::string actualPrompt = std::regex_replace(systemPrompt, std::regex("\\{\\{ USER_PROMPT \\}\\}"), prompt.toStdString());

    std::cout << "Actual Prompt: " << actualPrompt << std::endl;

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

    std::cout << response << std::endl;

    // Read in the 'response' field. The http response will be JSON.
    const std::string body = beast::buffers_to_string(response.body().data());
    const auto parsed = boost::json::parse(body);
    const auto& obj = parsed.as_object();
    const auto it = obj.find("response");
    if (it == obj.end() || !it->value().is_string()) {
        throw std::runtime_error("Response JSON missing 'response' string field");
    }
    const auto& responseText = it->value().as_string();
    return juce::String(responseText.c_str());
}
