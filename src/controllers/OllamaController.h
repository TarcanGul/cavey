//
// Created by Tarcan Gul on 12/20/25.
//

#pragma once

#include <JuceHeader.h>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/json.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>
#include "LLMController.h"
#include "BinaryData.h"

namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>

namespace Cavey {
    static constexpr std::string_view API_BASE = "/api";
    static constexpr unsigned int HTTP_VERSION = 10;
}


class OllamaController : public LLMController {
private:
    // TODO: maybe parametrize
    static constexpr std::string_view OLLAMA_HOST = "localhost";
    static constexpr std::string_view OLLAMA_PORT = "11434";

    std::string systemPrompt {};
    juce::String selectedModel_;
public:
    explicit OllamaController();
    String prompt(String const& prompt) override;
    juce::StringArray fetchModels();
    juce::String getSelectedModel() const;
    void setSelectedModel(const juce::String& model);
    bool hasSelectedModel() const;
};
