#pragma once

#include <JuceHeader.h>

#include <functional>
#include <map>
#include <optional>

namespace Cavey {

struct EnvironmentVariableWriteResult {
    bool saved = false;
    juce::String message;
};

class EnvironmentVariableProvider {
public:
    virtual ~EnvironmentVariableProvider() = default;
    virtual std::optional<juce::String> getEnvironmentVariable(
            const juce::String& name) const = 0;
    virtual EnvironmentVariableWriteResult saveEnvironmentVariable(
            const juce::String& name,
            const juce::String& value) = 0;
    virtual EnvironmentVariableWriteResult removeEnvironmentVariable(
            const juce::String& name) = 0;
};

class SystemEnvironmentVariableProvider final
        : public EnvironmentVariableProvider {
public:
    using ProcessEnvironmentReader = std::function<std::optional<juce::String>(
            const juce::String& name)>;

    SystemEnvironmentVariableProvider()
        : config_file_(getDefaultConfigFile()),
          process_environment_reader_(readProcessEnvironment) {}

    explicit SystemEnvironmentVariableProvider(juce::File config_file)
        : config_file_(std::move(config_file)),
          process_environment_reader_(readProcessEnvironment) {}

    SystemEnvironmentVariableProvider(
            juce::File config_file,
            ProcessEnvironmentReader process_environment_reader)
        : config_file_(std::move(config_file)),
          process_environment_reader_(std::move(process_environment_reader)) {}

    std::optional<juce::String> getEnvironmentVariable(
            const juce::String& name) const override {
        const auto process_value = process_environment_reader_(name);
        if (process_value.has_value() && process_value->trim().isNotEmpty()) {
            return process_value->trim();
        }

        const auto config_values = loadConfigValues();
        const auto it = config_values.find(name);
        if (it == config_values.end() || it->second.trim().isEmpty()) {
            return std::nullopt;
        }

        return it->second.trim();
    }

    EnvironmentVariableWriteResult saveEnvironmentVariable(
            const juce::String& name,
            const juce::String& value) override {
        if (!isSupportedVariableName(name)) {
            return {
                .saved = false,
                .message = "Unsupported environment variable."
            };
        }

        const auto trimmed_value = value.trim();
        if (trimmed_value.isEmpty()) {
            return {
                .saved = false,
                .message = name + " cannot be empty."
            };
        }

        auto config_values = loadConfigValues();
        config_values[name] = trimmed_value;

        if (!writeConfigValues(config_values)) {
            return {
                .saved = false,
                .message = "Could not save " + name + "."
            };
        }

        return {
            .saved = true,
            .message = name + " saved."
        };
    }

    EnvironmentVariableWriteResult removeEnvironmentVariable(
            const juce::String& name) override {
        if (!isSupportedVariableName(name)) {
            return {
                .saved = false,
                .message = "Unsupported environment variable."
            };
        }

        auto config_values = loadConfigValues();
        const bool had_config_value = config_values.erase(name) > 0;

        if (!writeConfigValues(config_values)) {
            return {
                .saved = false,
                .message = "Could not reset " + name + "."
            };
        }

        const auto process_value = process_environment_reader_(name);
        if (process_value.has_value() && process_value->trim().isNotEmpty()) {
            return {
                .saved = true,
                .message = name + " remains set by the host process."
            };
        }

        return {
            .saved = true,
            .message = had_config_value
                    ? name + " reset."
                    : name + " was not set in Cavey .env."
        };
    }

private:
    static std::optional<juce::String> readProcessEnvironment(
            const juce::String& name) {
        const auto value = juce::SystemStats::getEnvironmentVariable(name, {});
        if (value.trim().isEmpty()) {
            return std::nullopt;
        }

        return value.trim();
    }

    static juce::File getDefaultConfigFile() {
        return juce::File::getSpecialLocation(
                juce::File::userApplicationDataDirectory)
                .getChildFile("Cavey")
                .getChildFile(".env");
    }

    static bool isSupportedApiKeyName(const juce::String& name) {
        return name == "OPENAI_API_KEY" || name == "ANTHROPIC_API_KEY";
    }

    static bool isSupportedVariableName(const juce::String& name) {
        return isSupportedApiKeyName(name)
                || name == "CAVEY_MAIN_AI_PROVIDER";
    }

    static juce::String stripOptionalQuotes(juce::String value) {
        value = value.trim();
        if (value.length() >= 2
            && ((value.startsWithChar('"') && value.endsWithChar('"'))
                || (value.startsWithChar('\'') && value.endsWithChar('\'')))) {
            return value.substring(1, value.length() - 1);
        }

        return value;
    }

    std::map<juce::String, juce::String> loadConfigValues() const {
        std::map<juce::String, juce::String> values;
        if (!config_file_.existsAsFile()) {
            return values;
        }

        auto lines = juce::StringArray::fromLines(config_file_.loadFileAsString());
        for (auto line : lines) {
            line = line.trim();
            if (line.isEmpty() || line.startsWithChar('#')) {
                continue;
            }

            if (line.startsWith("export ")) {
                line = line.substring(7).trim();
            }

            const auto separator_index = line.indexOfChar('=');
            if (separator_index <= 0) {
                continue;
            }

            const auto key = line.substring(0, separator_index).trim();
            if (!isSupportedVariableName(key)) {
                continue;
            }

            values[key] = stripOptionalQuotes(
                    line.substring(separator_index + 1));
        }

        return values;
    }

    bool writeConfigValues(
            const std::map<juce::String, juce::String>& config_values) {
        const auto create_result = config_file_.create();
        if (create_result.failed()) {
            return false;
        }

        juce::String contents;
        for (const auto& [key, saved_value] : config_values) {
            if (isSupportedVariableName(key)
                && saved_value.trim().isNotEmpty()) {
                contents += key + "=" + saved_value.trim() + "\n";
            }
        }

        return config_file_.replaceWithText(contents);
    }

    juce::File config_file_;
    ProcessEnvironmentReader process_environment_reader_;
};

}  // namespace Cavey
