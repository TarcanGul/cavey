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
#include <cstdlib>
#include <iostream>
#include <string>
#include "LLMController.h"

namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>


class OllamaController : public LLMController {
private:
    static constexpr std::string_view OLLAMA_HOST = "localhost";
    static constexpr std::string_view OLLAMA_PORT = "11434";
    static constexpr std::string_view API_BASE = "/api";
    static constexpr std::string_view HTTP_VERSION = "1.0";
    boost::beast::tcp_stream stream;
public:
    explicit OllamaController();
    String prompt(String const& prompt) override;
};


