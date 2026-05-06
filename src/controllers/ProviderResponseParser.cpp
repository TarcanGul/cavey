#include "ProviderResponseParser.h"

#include <boost/algorithm/string/trim.hpp>

namespace Cavey {
namespace {

std::optional<std::string> FindStringField(const boost::json::value& value,
                                           const char* field_name) {
    if (value.is_object()) {
        const auto& object = value.as_object();
        if (const auto it = object.find(field_name);
            it != object.end() && it->value().is_string()) {
            return std::string(it->value().as_string().c_str());
        }

        for (const auto& item : object) {
            if (auto found = FindStringField(item.value(), field_name)) {
                return found;
            }
        }
    }

    if (value.is_array()) {
        for (const auto& item : value.as_array()) {
            if (auto found = FindStringField(item, field_name)) {
                return found;
            }
        }
    }

    return std::nullopt;
}

}  // namespace

juce::String ExtractCoefficientJson(const juce::String& text) {
    static constexpr const char* kOpenTag = "<json>";
    static constexpr const char* kCloseTag = "</json>";

    auto response = text.toStdString();
    const auto start_raw = response.find(kOpenTag);
    if (start_raw != std::string::npos) {
        const auto start = start_raw + std::char_traits<char>::length(kOpenTag);
        const auto end = response.find(kCloseTag, start);
        if (end == std::string::npos) {
            throw std::runtime_error("AI response was missing a closing JSON tag.");
        }

        response = response.substr(start, end - start);
    }

    response = boost::algorithm::trim_copy(response);
    boost::system::error_code error_code;
    const auto parsed = boost::json::parse(response, error_code);
    if (error_code || !parsed.is_object()) {
        throw std::runtime_error("AI response did not include usable parameter JSON.");
    }

    if (!parsed.as_object().contains("NAME")) {
        throw std::runtime_error("AI response did not include a parameter name.");
    }

    return response;
}

juce::String ExtractOpenAIText(const boost::json::value& response) {
    if (auto output_text = FindStringField(response, "output_text")) {
        return juce::String(*output_text);
    }

    if (auto text = FindStringField(response, "text")) {
        return juce::String(*text);
    }

    throw std::runtime_error("OpenAI response did not include text content.");
}

juce::String ExtractAnthropicText(const boost::json::value& response) {
    if (auto text = FindStringField(response, "text")) {
        return juce::String(*text);
    }

    throw std::runtime_error("Anthropic response did not include text content.");
}

}  // namespace Cavey
