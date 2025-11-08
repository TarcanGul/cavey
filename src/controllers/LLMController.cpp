//
// Created by Tarcan Gul on 11/6/25.
//

#include "LLMController.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>

/**
 * Assumptions:
 * - Assume ollama is already running in port PORT.
 *
 * Steps:
 * - Make sure ollama is running in PORT
 * - Send the prompt to ollama via RPC with system prompt and context.
 * - Get the response back.
 * - Create a parameter with that.
 *
 * Spike parts:
 * - What to actually generate to make changes is sound
 * - The system prompt
 *
 * Use Boost.Asio
 */
LLMController::LLMController() {
    // TODO: Construct in initializer list
    net::io_context io_context;
    tcp::resolver resolver(io_context);
    beast::tcp_stream stream(io_context);

    // auto const address = resolver.resolve(host, port);
    // stream.connect(address);
    // http::request<http::string_body> req{http::verb::post, target, version};
    // tcp::resolver::results_type endpoints = resolver.resolve(OLLAMA_HOST, "ollama");

    // tcp::socket socket(io_context);
    // boost::asio::connect(socket, endpoints)
}

LLMController::~LLMController() {

}

void LLMController::sendPrompt(const juce::String &prompt) {

}