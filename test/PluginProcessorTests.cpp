#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <map>
#include <memory>

#include "PluginProcessor.h"

namespace {

class MockLLMController final : public LLMController {
public:
    explicit MockLLMController(juce::String response) : response_(std::move(response)) {}

    juce::String prompt(const juce::String& prompt) override {
        juce::ignoreUnused(prompt);
        return response_;
    }

    Cavey::ProviderConnectionResult connect(
            const Cavey::ProviderConnectionConfig& config) override {
        juce::ignoreUnused(config);
        return {
            .connected = true,
            .message = "Connected."
        };
    }

    Cavey::ProviderMetadata metadata() const override {
        return {
            .provider = Cavey::AiProvider::kNone,
            .id = "mock",
            .display_name = "Mock",
            .model = "mock",
            .requires_api_key = false
        };
    }

private:
    juce::String response_;
};

class FakeEnvironmentVariableProvider final
        : public Cavey::EnvironmentVariableProvider {
public:
    void set(const juce::String& name, const juce::String& value) {
        values_[name] = value;
    }

    std::optional<juce::String> getEnvironmentVariable(
            const juce::String& name) const override {
        const auto it = values_.find(name);
        if (it == values_.end() || it->second.trim().isEmpty()) {
            return std::nullopt;
        }

        return it->second;
    }

    Cavey::EnvironmentVariableWriteResult saveEnvironmentVariable(
            const juce::String& name,
            const juce::String& value) override {
        values_[name] = value;
        return {
            .saved = true,
            .message = name + " saved."
        };
    }

    Cavey::EnvironmentVariableWriteResult removeEnvironmentVariable(
            const juce::String& name) override {
        values_.erase(name);
        return {
            .saved = true,
            .message = name + " reset."
        };
    }

private:
    std::map<juce::String, juce::String> values_;
};

std::unique_ptr<MockLLMController> MakeMockController() {
    return std::make_unique<MockLLMController>(R"({
        "NAME": "Warmth",
        "VOLUME": 1.0,
        "LOW_PASS": 0.0,
        "HIGH_PASS": 0.0,
        "REVERB": 0.0,
        "DISTORTION": 0.0,
        "CHORUS": 0.0
    })");
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

TEST_CASE("Generate is rejected before AI setup", "[processor]") {
    CaveyAudioProcessor processor(
            std::make_shared<FakeEnvironmentVariableProvider>());

    REQUIRE_FALSE(processor.isAiProviderConnected());
    try {
        processor.addCaveyParameter("make it warm");
        FAIL("Expected generation before setup to throw.");
    } catch (const std::exception& exception) {
        REQUIRE(juce::String(exception.what())
                == "Set up your AI provider before generating.");
    }
}

TEST_CASE("Main AI provider is persisted and drives generation", "[processor]") {
    SECTION("Persisted provider is loaded on construction") {
        auto environment =
                std::make_shared<FakeEnvironmentVariableProvider>();
        environment->set("CAVEY_MAIN_AI_PROVIDER", "anthropic");

        CaveyAudioProcessor processor(environment);

        REQUIRE(processor.getMainAiProvider()
                == Cavey::AiProvider::kAnthropic);
        REQUIRE(processor.getMainAiProviderName() == "Anthropic");
        REQUIRE(processor.isAiProviderConnected());
    }

    SECTION("Saving the main provider updates persisted state") {
        auto environment =
                std::make_shared<FakeEnvironmentVariableProvider>();
        CaveyAudioProcessor processor(environment);

        const auto result = processor.saveMainAiProvider(
                Cavey::AiProvider::kOpenAI);

        INFO(result.message);
        REQUIRE(result.saved);
        REQUIRE(processor.getMainAiProvider() == Cavey::AiProvider::kOpenAI);
        REQUIRE(environment->getEnvironmentVariable(
                "CAVEY_MAIN_AI_PROVIDER").value() == "openai");
    }

    SECTION("Selected hosted provider fails at runtime without API key") {
        auto environment =
                std::make_shared<FakeEnvironmentVariableProvider>();
        environment->set("CAVEY_MAIN_AI_PROVIDER", "openai");
        CaveyAudioProcessor processor(environment);

        try {
            processor.addCaveyParameter("make it warm");
            FAIL("Expected selected provider to fail without an API key.");
        } catch (const std::exception& exception) {
            REQUIRE(juce::String(exception.what())
                    == "OPENAI_API_KEY environment variable is not set.");
        }
    }
}

TEST_CASE("Generated parameters are created from LLM responses", "[processor]") {
    CaveyAudioProcessor processor(MakeMockController());

    processor.addCaveyParameter("make it warm");

    REQUIRE(processor.hasGeneratedParameter());
    REQUIRE(processor.getGeneratedParameterName() == "Warmth");
    REQUIRE(processor.getValueTree().getParameter("Warmth") != nullptr);
}

TEST_CASE("Backend parameter values can be updated and validate names", "[processor]") {
    CaveyAudioProcessor processor(MakeMockController());
    processor.addCaveyParameter("make it warm");

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
    processor.addCaveyParameter("make it warm");
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
