#include <catch2/catch_test_macros.hpp>

#include <map>
#include <memory>
#include <vector>

#include "controllers/AnthropicController.h"
#include "controllers/EnvironmentVariableProvider.h"
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

juce::File MakeTemporaryEnvironmentFile() {
    return juce::File::getCurrentWorkingDirectory()
            .getChildFile("cavey-env-test-" + juce::Uuid().toString())
            .getChildFile(".env");
}

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
    auto environment = std::make_shared<FakeEnvironmentVariableProvider>();
    environment->set("OPENAI_API_KEY", "sk-test");
    Cavey::OpenAIController controller(transport, environment);

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
    auto environment = std::make_shared<FakeEnvironmentVariableProvider>();
    environment->set("ANTHROPIC_API_KEY", "sk-ant-test");
    Cavey::AnthropicController controller(transport, environment);

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
        auto environment = std::make_shared<FakeEnvironmentVariableProvider>();
        environment->set("OPENAI_API_KEY", "secret-value");
        Cavey::OpenAIController controller(transport, environment);

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
        auto environment = std::make_shared<FakeEnvironmentVariableProvider>();
        environment->set("ANTHROPIC_API_KEY", "secret-value");
        Cavey::AnthropicController controller(transport, environment);

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

TEST_CASE("Hosted providers require environment API keys", "[providers]") {
    SECTION("OpenAI connect fails without OPENAI_API_KEY") {
        auto transport = std::make_shared<FakeHttpTransport>(
                std::vector<Cavey::HttpResponse>{});
        auto environment = std::make_shared<FakeEnvironmentVariableProvider>();
        Cavey::OpenAIController controller(transport, environment);

        const auto result = controller.connect({});

        REQUIRE_FALSE(result.connected);
        REQUIRE(result.message == "OPENAI_API_KEY environment variable is not set.");
        REQUIRE(transport->requests.empty());
    }

    SECTION("Anthropic connect fails without ANTHROPIC_API_KEY") {
        auto transport = std::make_shared<FakeHttpTransport>(
                std::vector<Cavey::HttpResponse>{});
        auto environment = std::make_shared<FakeEnvironmentVariableProvider>();
        Cavey::AnthropicController controller(transport, environment);

        const auto result = controller.connect({});

        REQUIRE_FALSE(result.connected);
        REQUIRE(result.message
                == "ANTHROPIC_API_KEY environment variable is not set.");
        REQUIRE(transport->requests.empty());
    }

    SECTION("OpenAI prompt fails without OPENAI_API_KEY") {
        auto transport = std::make_shared<FakeHttpTransport>(
                std::vector<Cavey::HttpResponse>{});
        auto environment = std::make_shared<FakeEnvironmentVariableProvider>();
        Cavey::OpenAIController controller(transport, environment);

        try {
            juce::ignoreUnused(controller.prompt("make it warm"));
            FAIL("Expected OpenAI prompt to throw.");
        } catch (const std::exception& exception) {
            REQUIRE(juce::String(exception.what())
                    == "OPENAI_API_KEY environment variable is not set.");
        }
    }

    SECTION("Anthropic prompt fails without ANTHROPIC_API_KEY") {
        auto transport = std::make_shared<FakeHttpTransport>(
                std::vector<Cavey::HttpResponse>{});
        auto environment = std::make_shared<FakeEnvironmentVariableProvider>();
        Cavey::AnthropicController controller(transport, environment);

        try {
            juce::ignoreUnused(controller.prompt("make it warm"));
            FAIL("Expected Anthropic prompt to throw.");
        } catch (const std::exception& exception) {
            REQUIRE(juce::String(exception.what())
                    == "ANTHROPIC_API_KEY environment variable is not set.");
        }
    }
}

TEST_CASE("System environment provider persists Cavey API keys", "[providers]") {
    const auto config_file = MakeTemporaryEnvironmentFile();
    const auto no_process_environment = [](
            const juce::String& name) -> std::optional<juce::String> {
        juce::ignoreUnused(name);
        return std::nullopt;
    };

    SECTION("Saved OpenAI key can be read back") {
        Cavey::SystemEnvironmentVariableProvider environment(
                config_file,
                no_process_environment);

        const auto result = environment.saveEnvironmentVariable(
                "OPENAI_API_KEY",
                "sk-from-file");

        INFO(result.message);
        REQUIRE(result.saved);
        REQUIRE(environment.getEnvironmentVariable("OPENAI_API_KEY").value()
                == "sk-from-file");
    }

    SECTION("Saving one key preserves the other") {
        Cavey::SystemEnvironmentVariableProvider environment(
                config_file,
                no_process_environment);

        const auto openai_result = environment.saveEnvironmentVariable(
                "OPENAI_API_KEY",
                "sk-openai");
        INFO(openai_result.message);
        REQUIRE(openai_result.saved);
        const auto anthropic_result = environment.saveEnvironmentVariable(
                "ANTHROPIC_API_KEY",
                "sk-anthropic");
        INFO(anthropic_result.message);
        REQUIRE(anthropic_result.saved);

        REQUIRE(environment.getEnvironmentVariable("OPENAI_API_KEY").value()
                == "sk-openai");
        REQUIRE(environment.getEnvironmentVariable("ANTHROPIC_API_KEY").value()
                == "sk-anthropic");
    }

    SECTION("Saving an existing OpenAI key overwrites its file value") {
        Cavey::SystemEnvironmentVariableProvider environment(
                config_file,
                no_process_environment);

        REQUIRE(environment.saveEnvironmentVariable(
                "OPENAI_API_KEY",
                "sk-openai-original").saved);

        const auto result = environment.saveEnvironmentVariable(
                "OPENAI_API_KEY",
                "sk-openai-replacement");

        INFO(result.message);
        REQUIRE(result.saved);
        REQUIRE(environment.getEnvironmentVariable("OPENAI_API_KEY").value()
                == "sk-openai-replacement");
    }

    SECTION("Saving an existing Anthropic key overwrites its file value") {
        Cavey::SystemEnvironmentVariableProvider environment(
                config_file,
                no_process_environment);

        REQUIRE(environment.saveEnvironmentVariable(
                "ANTHROPIC_API_KEY",
                "sk-anthropic-original").saved);

        const auto result = environment.saveEnvironmentVariable(
                "ANTHROPIC_API_KEY",
                "sk-anthropic-replacement");

        INFO(result.message);
        REQUIRE(result.saved);
        REQUIRE(environment.getEnvironmentVariable("ANTHROPIC_API_KEY").value()
                == "sk-anthropic-replacement");
    }

    SECTION("Resetting OpenAI removes only OPENAI_API_KEY") {
        Cavey::SystemEnvironmentVariableProvider environment(
                config_file,
                no_process_environment);

        REQUIRE(environment.saveEnvironmentVariable(
                "OPENAI_API_KEY",
                "sk-openai").saved);
        REQUIRE(environment.saveEnvironmentVariable(
                "ANTHROPIC_API_KEY",
                "sk-anthropic").saved);

        const auto result = environment.removeEnvironmentVariable(
                "OPENAI_API_KEY");

        INFO(result.message);
        REQUIRE(result.saved);
        REQUIRE_FALSE(environment.getEnvironmentVariable(
                "OPENAI_API_KEY").has_value());
        REQUIRE(environment.getEnvironmentVariable("ANTHROPIC_API_KEY").value()
                == "sk-anthropic");
    }

    SECTION("Resetting Anthropic removes only ANTHROPIC_API_KEY") {
        Cavey::SystemEnvironmentVariableProvider environment(
                config_file,
                no_process_environment);

        REQUIRE(environment.saveEnvironmentVariable(
                "OPENAI_API_KEY",
                "sk-openai").saved);
        REQUIRE(environment.saveEnvironmentVariable(
                "ANTHROPIC_API_KEY",
                "sk-anthropic").saved);

        const auto result = environment.removeEnvironmentVariable(
                "ANTHROPIC_API_KEY");

        INFO(result.message);
        REQUIRE(result.saved);
        REQUIRE(environment.getEnvironmentVariable("OPENAI_API_KEY").value()
                == "sk-openai");
        REQUIRE_FALSE(environment.getEnvironmentVariable(
                "ANTHROPIC_API_KEY").has_value());
    }

    SECTION("Resetting a missing key is not an error") {
        Cavey::SystemEnvironmentVariableProvider environment(
                config_file,
                no_process_environment);

        const auto result = environment.removeEnvironmentVariable(
                "OPENAI_API_KEY");

        INFO(result.message);
        REQUIRE(result.saved);
        REQUIRE_FALSE(environment.getEnvironmentVariable(
                "OPENAI_API_KEY").has_value());
    }

    SECTION("Blank saved values are rejected") {
        Cavey::SystemEnvironmentVariableProvider environment(
                config_file,
                no_process_environment);

        const auto result = environment.saveEnvironmentVariable(
                "OPENAI_API_KEY",
                "   ");

        REQUIRE_FALSE(result.saved);
        REQUIRE_FALSE(environment.getEnvironmentVariable("OPENAI_API_KEY").has_value());
    }

    SECTION("Process environment wins over file values") {
        const auto process_environment = [](
                const juce::String& name) -> std::optional<juce::String> {
            if (name == "OPENAI_API_KEY") {
                return "sk-from-process";
            }

            return std::nullopt;
        };
        Cavey::SystemEnvironmentVariableProvider environment(
                config_file,
                process_environment);

        const auto result = environment.saveEnvironmentVariable(
                "OPENAI_API_KEY",
                "sk-from-file");
        INFO(result.message);
        REQUIRE(result.saved);

        REQUIRE(environment.getEnvironmentVariable("OPENAI_API_KEY").value()
                == "sk-from-process");
    }

    SECTION("Process environment still wins after reset") {
        const auto process_environment = [](
                const juce::String& name) -> std::optional<juce::String> {
            if (name == "OPENAI_API_KEY") {
                return "sk-from-process";
            }

            return std::nullopt;
        };
        Cavey::SystemEnvironmentVariableProvider environment(
                config_file,
                process_environment);

        REQUIRE(environment.saveEnvironmentVariable(
                "OPENAI_API_KEY",
                "sk-from-file").saved);

        const auto result = environment.removeEnvironmentVariable(
                "OPENAI_API_KEY");

        INFO(result.message);
        REQUIRE(result.saved);
        REQUIRE(environment.getEnvironmentVariable("OPENAI_API_KEY").value()
                == "sk-from-process");

        Cavey::SystemEnvironmentVariableProvider file_only_environment(
                config_file,
                no_process_environment);
        REQUIRE_FALSE(file_only_environment.getEnvironmentVariable(
                "OPENAI_API_KEY").has_value());
    }

    config_file.deleteFile();
    config_file.getParentDirectory().deleteRecursively();
}
