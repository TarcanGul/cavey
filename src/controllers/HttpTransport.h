#pragma once

#include <JuceHeader.h>

namespace Cavey {

struct HttpRequest {
    juce::String method = "GET";
    juce::String url;
    juce::String headers;
    juce::String body;
    int timeout_ms = 15000;
};

struct HttpResponse {
    int status_code = 0;
    juce::String body;
};

class HttpTransport {
public:
    virtual ~HttpTransport() = default;
    virtual HttpResponse send(const HttpRequest& request) = 0;
};

class JuceHttpTransport final : public HttpTransport {
public:
    HttpResponse send(const HttpRequest& request) override;
};

}  // namespace Cavey
