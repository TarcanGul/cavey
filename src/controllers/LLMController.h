//
// Created by Tarcan Gul on 11/6/25.
//

#pragma once

#include <JuceHeader.h>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>

class LLMController {
private:
    beast::tcp_stream stream;
    const String OLLAMA_HOST = "localhost";
    const String OLLAMA_PORT = "11434";
public:
    explicit LLMController();
    ~LLMController();
    void sendPrompt(String const& prompt)
};
