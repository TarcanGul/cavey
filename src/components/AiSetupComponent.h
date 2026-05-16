#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <functional>
#include <thread>

class CaveyAudioProcessor;

class AiSetupComponent final : public juce::Component {
public:
    AiSetupComponent(CaveyAudioProcessor& processor,
                     std::function<void()> selectionChangedCallback);
    ~AiSetupComponent() override;

    void resized() override;

private:
    void refreshModels();
    void populateModels(const juce::StringArray& models);
    void selectCurrentComboModel();
    void saveSelectedModel();
    void setStatus(const juce::String& status);

    CaveyAudioProcessor& processor_;
    std::function<void()> selectionChangedCallback_;
    juce::Label titleLabel_;
    juce::Label providerLabel_;
    juce::ComboBox modelComboBox_;
    juce::TextButton refreshButton_;
    juce::TextButton selectButton_;
    juce::Label statusLabel_;
    juce::StringArray models_;
    std::atomic<bool> isAlive_ {true};
    bool isPopulating_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AiSetupComponent)
};
