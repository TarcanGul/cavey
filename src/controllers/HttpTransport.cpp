#include "HttpTransport.h"

namespace Cavey {

HttpResponse JuceHttpTransport::send(const HttpRequest& request) {
    if (request.url.isEmpty()) {
        throw std::invalid_argument("Request URL cannot be empty");
    }

    int status_code = 0;
    juce::URL url(request.url);
    if (request.body.isNotEmpty()) {
        url = url.withPOSTData(request.body);
    }

    auto options = juce::URL::InputStreamOptions(
                           juce::URL::ParameterHandling::inAddress)
                           .withHttpRequestCmd(request.method)
                           .withExtraHeaders(request.headers)
                           .withConnectionTimeoutMs(request.timeout_ms)
                           .withStatusCode(&status_code)
                           .withNumRedirectsToFollow(0);

    auto stream = url.createInputStream(options);
    if (stream == nullptr) {
        return {
            .status_code = status_code,
            .body = {}
        };
    }

    return {
        .status_code = status_code,
        .body = stream->readEntireStreamAsString()
    };
}

}  // namespace Cavey
