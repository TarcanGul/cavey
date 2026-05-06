#pragma once

#include <JuceHeader.h>

namespace Cavey {

class CredentialStore {
public:
    virtual ~CredentialStore() = default;
    virtual bool saveSecret(const juce::String& account,
                            const juce::String& secret,
                            juce::String* error_message) = 0;
    virtual std::optional<juce::String> loadSecret(
            const juce::String& account) const = 0;
    virtual bool hasSecret(const juce::String& account) const = 0;
};

class SystemCredentialStore final : public CredentialStore {
public:
    bool saveSecret(const juce::String& account,
                    const juce::String& secret,
                    juce::String* error_message) override;
    std::optional<juce::String> loadSecret(
            const juce::String& account) const override;
    bool hasSecret(const juce::String& account) const override;

private:
    static constexpr const char* kServiceName = "com.tarcangul.Cavey.ai";
};

}  // namespace Cavey
