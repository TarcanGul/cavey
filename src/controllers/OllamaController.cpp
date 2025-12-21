//
// Created by Tarcan Gul on 12/20/25.
//

#include "OllamaController.h"

OllamaController::OllamaController() {
    // Could move this to a static factory
    net::io_context io_context;
    tcp::resolver resolver(io_context);
    beast::tcp_stream stream(io_context);

    auto const results = resolver.resolve(OLLAMA_HOST, OLLAMA_PORT);

    this->stream = stream;

    stream.connect(results);
}

String OllamaController::prompt(const juce::String &prompt) {
    http::request<http::string_body> req{http::verb::post, "/generate", HTTP_VERSION};
    return "This is the response for now.";
}
