#pragma once

#include <JuceHeader.h>
#include <boost/json.hpp>

namespace Cavey {

juce::String ExtractCoefficientJson(const juce::String& text);
juce::String ExtractOpenAIText(const boost::json::value& response);
juce::String ExtractAnthropicText(const boost::json::value& response);

}  // namespace Cavey
