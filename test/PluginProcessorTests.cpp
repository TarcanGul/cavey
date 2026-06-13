#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <memory>
#include <stdexcept>
#include <utility>

#include "PluginProcessor.h"

namespace {

class MockLLMController final : public LLMController {
public:
    explicit MockLLMController(juce::String response) : response_(std::move(response)) {}

    String prompt(String const& prompt) override {
        juce::ignoreUnused(prompt);
        return response_;
    }

private:
    juce::String response_;
};

class ThrowingLLMController final : public LLMController {
public:
    explicit ThrowingLLMController(juce::String errorMessage)
        : errorMessage_(std::move(errorMessage)) {}

    String prompt(String const& prompt) override {
        juce::ignoreUnused(prompt);
        throw std::runtime_error(errorMessage_.toStdString());
    }

private:
    juce::String errorMessage_;
};

std::unique_ptr<MockLLMController> MakeMockController(
    juce::String response = R"({
        "NAME": "Warmth",
        "VOLUME": 1.0,
        "LOW_PASS": 0.0,
        "HIGH_PASS": 0.0,
        "REVERB": 0.0,
        "DISTORTION": 0.0,
        "CHORUS": 0.0
    })") {
    return std::make_unique<MockLLMController>(std::move(response));
}

bool BuffersMatch(const juce::AudioBuffer<float>& first,
                  const juce::AudioBuffer<float>& second) {
    if (first.getNumChannels() != second.getNumChannels()
        || first.getNumSamples() != second.getNumSamples()) {
        return false;
    }

    for (int channel = 0; channel < first.getNumChannels(); ++channel) {
        for (int sample = 0; sample < first.getNumSamples(); ++sample) {
            if (first.getSample(channel, sample) != second.getSample(channel, sample)) {
                return false;
            }
        }
    }

    return true;
}

bool HasFiniteSamples(const juce::AudioBuffer<float>& buffer) {
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            if (!std::isfinite(buffer.getSample(channel, sample))) {
                return false;
            }
        }
    }

    return true;
}

}  // namespace

TEST_CASE("Selected Ollama model is saved and read from settings", "[processor]") {
    CaveyAudioProcessor processor;
    processor.setSelectedOllamaModel({});

    REQUIRE(processor.getSelectedOllamaModel().isEmpty());

    processor.setSelectedOllamaModel("llama3.2:latest");

    CaveyAudioProcessor otherProcessor;
    REQUIRE(otherProcessor.getSelectedOllamaModel() == "llama3.2:latest");

    otherProcessor.setSelectedOllamaModel({});
}

TEST_CASE("Generate path uses the generic LLM prompt interface", "[processor]") {
    CaveyAudioProcessor processor(MakeMockController());

    const GenerateParameterResult result = processor.addCaveyParameter("make it warm");

    REQUIRE(result.success);
    REQUIRE(processor.hasGeneratedParameter());
}

TEST_CASE("Model selection errors are returned as generation failures", "[processor]") {
    CaveyAudioProcessor processor(
        std::make_unique<ThrowingLLMController>("Ollama model cannot be empty"));

    const GenerateParameterResult result = processor.addCaveyParameter("make it warm");

    REQUIRE_FALSE(result.success);
    REQUIRE(result.errorMessage.contains("model"));
    REQUIRE_FALSE(processor.hasGeneratedParameter());
}

TEST_CASE("Generated parameters are created from LLM responses", "[processor]") {
    CaveyAudioProcessor processor(MakeMockController());

    const GenerateParameterResult result = processor.addCaveyParameter("make it warm");

    REQUIRE(result.success);
    REQUIRE(result.parameterName == "Warmth");
    REQUIRE(processor.hasGeneratedParameter());
    REQUIRE(processor.getGeneratedParameterName() == "Warmth");
    REQUIRE(processor.getValueTree().getParameter("Warmth") != nullptr);
}

TEST_CASE("LLM prompt exceptions are returned as generation failures", "[processor]") {
    CaveyAudioProcessor processor(std::make_unique<ThrowingLLMController>("LLM request failed"));
    GenerateParameterResult result;

    REQUIRE_NOTHROW(result = processor.addCaveyParameter("make it warm"));

    REQUIRE_FALSE(result.success);
    REQUIRE(result.errorMessage.contains("LLM request failed"));
    REQUIRE_FALSE(processor.hasGeneratedParameter());
}

TEST_CASE("Malformed LLM JSON is rejected safely", "[processor]") {
    CaveyAudioProcessor processor(MakeMockController("not json"));

    const GenerateParameterResult result = processor.addCaveyParameter("make it warm");

    REQUIRE_FALSE(result.success);
    REQUIRE(result.errorMessage.contains("invalid JSON"));
    REQUIRE_FALSE(processor.hasGeneratedParameter());
}

TEST_CASE("LLM responses missing required fields are rejected safely", "[processor]") {
    CaveyAudioProcessor processor(MakeMockController(R"({
        "NAME": "Warmth",
        "VOLUME": 1.0,
        "LOW_PASS": 0.0,
        "HIGH_PASS": 0.0,
        "REVERB": 0.0
    })"));

    const GenerateParameterResult result = processor.addCaveyParameter("make it warm");

    REQUIRE_FALSE(result.success);
    REQUIRE(result.errorMessage.contains("DISTORTION"));
    REQUIRE_FALSE(processor.hasGeneratedParameter());
}

TEST_CASE("LLM responses may omit optional chorus", "[processor]") {
    CaveyAudioProcessor processor(MakeMockController(R"({
        "NAME": "Warmth",
        "VOLUME": 1.0,
        "LOW_PASS": 0.0,
        "HIGH_PASS": 0.0,
        "REVERB": 0.0,
        "DISTORTION": 0.0
    })"));

    const GenerateParameterResult result = processor.addCaveyParameter("make it warm");

    REQUIRE(result.success);
    REQUIRE(result.parameterName == "Warmth");
    REQUIRE(processor.hasGeneratedParameter());
}

TEST_CASE("Backend parameter values can be updated and validate names", "[processor]") {
    CaveyAudioProcessor processor(MakeMockController());
    REQUIRE(processor.addCaveyParameter("make it warm").success);

    processor.setBackendParameterValue("Warmth", 0.75f);

    REQUIRE(processor.getValueTree().getParameter("Warmth")->getValue()
            == Catch::Approx(0.75f));
    REQUIRE_THROWS_AS(processor.setBackendParameterValue("Missing", 0.5f),
                      std::invalid_argument);
}

TEST_CASE("Processing is unchanged when no generated parameter exists", "[processor]") {
    CaveyAudioProcessor processor(MakeMockController());
    juce::AudioBuffer<float> buffer(2, 8);
    juce::MidiBuffer midiBuffer;

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            buffer.setSample(channel, sample, 0.1f * static_cast<float>(sample + 1));
        }
    }

    juce::AudioBuffer<float> original;
    original.makeCopyOf(buffer);

    processor.processBlock(buffer, midiBuffer);

    REQUIRE(BuffersMatch(buffer, original));
}

TEST_CASE("Generated parameters apply finite deterministic processing", "[processor]") {
    CaveyAudioProcessor processor(MakeMockController());
    REQUIRE(processor.addCaveyParameter("make it warm").success);
    processor.setBackendParameterValue("Warmth", 0.5f);
    processor.prepareToPlay(44100.0, 32);

    juce::AudioBuffer<float> buffer(2, 32);
    juce::MidiBuffer midiBuffer;
    buffer.clear();
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            buffer.setSample(channel, sample, 0.25f);
        }
    }

    juce::AudioBuffer<float> original;
    original.makeCopyOf(buffer);

    processor.processBlock(buffer, midiBuffer);

    REQUIRE(HasFiniteSamples(buffer));
    REQUIRE_FALSE(BuffersMatch(buffer, original));
}
