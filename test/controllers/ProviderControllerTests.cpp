#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <vector>

#include "controllers/AnthropicController.h"
#include "controllers/CredentialStore.h"
#include "controllers/HttpTransport.h"
#include "controllers/OllamaController.h"
#include "controllers/OpenAIController.h"

namespace {

constexpr const char* kEscapedCoefficientJson =
        R"({\"NAME\":\"Warmth\",\"VOLUME\":1.0,\"LOW_PASS\":0.0,\"HIGH_PASS\":0.0,\"REVERB\":0.0,\"DISTORTION\":0.0,\"CHORUS\":0.0})";

class FakeHttpTransport final : public Cavey::HttpTransport {
public:
    explicit FakeHttpTransport(std::vector<Cavey::HttpResponse> responses)
        : responses_(std::move(responses)) {}

    Cavey::HttpResponse send(const Cavey::HttpRequest& request) override {
        requests.push_back(request);
        if (responses_.empty()) {
            return {
                .status_code = 500,
                .body = "{}"
            };
        }

        auto response = responses_.front();
        responses_.erase(responses_.begin());
        return response;
    }

    std::vector<Cavey::HttpRequest> requests;

private:
    std::vector<Cavey::HttpResponse> responses_;
};

class FakeCredentialStore final : public Cavey::CredentialStore {
public:
    bool saveSecret(const juce::String& account,
                    const juce::String& secret,
                    juce::String* error_message) override {
        juce::ignoreUnused(error_message);
        secrets_[account] = secret;
        return true;
    }

    std::optional<juce::String> loadSecret(
            const juce::String& account) const override {
        const auto it = secrets_.find(account);
        if (it == secrets_.end()) {
            return std::nullopt;
        }

        return it->second;
    }

    bool hasSecret(const juce::String& account) const override {
        return secrets_.contains(account);
    }

private:
    std::map<juce::String, juce::String> secrets_;
};

}  // namespace

TEST_CASE("OpenAI response text normalizes to coefficient JSON", "[providers]") {
    auto transport = std::make_shared<FakeHttpTransport>(std::vector<Cavey::HttpResponse>{
        {
            .status_code = 200,
            .body = juce::String(R"({"output":[{"content":[{"type":"output_text","text":"<json>)")
                    + kEscapedCoefficientJson
                    + R"(</json>"}]}]})"
        }
    });
    auto credentials = std::make_shared<FakeCredentialStore>();
    juce::String error;
    REQUIRE(credentials->saveSecret("openai", "sk-test", &error));
    Cavey::OpenAIController controller(transport, credentials);

    const auto response = controller.prompt("make it warm");

    REQUIRE(response.contains("\"NAME\""));
    REQUIRE(response.contains("Warmth"));
    const bool leaked_secret = transport->requests.front().headers.contains("sk-test")
            && response.contains("sk-test");
    REQUIRE_FALSE(leaked_secret);
}

TEST_CASE("Anthropic response text normalizes to coefficient JSON", "[providers]") {
    auto transport = std::make_shared<FakeHttpTransport>(std::vector<Cavey::HttpResponse>{
        {
            .status_code = 200,
            .body = juce::String(R"({"content":[{"type":"text","text":"<json>)")
                    + kEscapedCoefficientJson
                    + R"(</json>"}]})"
        }
    });
    auto credentials = std::make_shared<FakeCredentialStore>();
    juce::String error;
    REQUIRE(credentials->saveSecret("anthropic", "sk-ant-test", &error));
    Cavey::AnthropicController controller(transport, credentials);

    const auto response = controller.prompt("make it warm");

    REQUIRE(response.contains("\"NAME\""));
    REQUIRE(response.contains("Warmth"));
}

TEST_CASE("Ollama model tags populate names", "[providers]") {
    auto transport = std::make_shared<FakeHttpTransport>(std::vector<Cavey::HttpResponse>{
        {
            .status_code = 200,
            .body = R"({"models":[{"name":"gemma3:1b"},{"name":"llama3.2"}]})"
        }
    });
    Cavey::OllamaController controller(transport);

    const auto models = controller.fetchModels();

    REQUIRE(models.size() == 2);
    REQUIRE(models.contains("gemma3:1b"));
    REQUIRE(models.contains("llama3.2"));
}

TEST_CASE("Provider failures use safe error messages", "[providers]") {
    SECTION("OpenAI non-2xx") {
        auto transport = std::make_shared<FakeHttpTransport>(
                std::vector<Cavey::HttpResponse>{{.status_code = 401, .body = "{}"}});
        auto credentials = std::make_shared<FakeCredentialStore>();
        juce::String error;
        REQUIRE(credentials->saveSecret("openai", "secret-value", &error));
        Cavey::OpenAIController controller(transport, credentials);

        try {
            juce::ignoreUnused(controller.prompt("make it warm"));
            FAIL("Expected OpenAI prompt to throw.");
        } catch (const std::exception& exception) {
            REQUIRE(juce::String(exception.what())
                    == "OpenAI could not generate a response.");
        }
    }

    SECTION("Anthropic missing text") {
        auto transport = std::make_shared<FakeHttpTransport>(
                std::vector<Cavey::HttpResponse>{{.status_code = 200, .body = "{}"}});
        auto credentials = std::make_shared<FakeCredentialStore>();
        juce::String error;
        REQUIRE(credentials->saveSecret("anthropic", "secret-value", &error));
        Cavey::AnthropicController controller(transport, credentials);

        try {
            juce::ignoreUnused(controller.prompt("make it warm"));
            FAIL("Expected Anthropic prompt to throw.");
        } catch (const std::exception& exception) {
            REQUIRE(juce::String(exception.what())
                    == "Anthropic response did not include text content.");
        }
    }

    SECTION("Ollama missing model list") {
        auto transport = std::make_shared<FakeHttpTransport>(
                std::vector<Cavey::HttpResponse>{{.status_code = 200, .body = "{}"}});
        Cavey::OllamaController controller(transport);

        try {
            juce::ignoreUnused(controller.fetchModels());
            FAIL("Expected Ollama model fetch to throw.");
        } catch (const std::exception& exception) {
            REQUIRE(juce::String(exception.what())
                    == "Ollama response did not include downloaded models.");
        }
    }
}
