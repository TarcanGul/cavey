#pragma once

#include <JuceHeader.h>
#include <functional>

#include "../controllers/LLMController.h"

class CaveyAudioProcessor;

class AiSetupComponent final : public juce::Component,
                               private juce::Button::Listener,
                               private juce::ComboBox::Listener {
public:
    using CompletionCallback = std::function<void(bool connected,
                                                  const juce::String& message)>;

    AiSetupComponent(CaveyAudioProcessor& processor,
                     CompletionCallback completion_callback);
    ~AiSetupComponent() override;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

private:
    void buttonClicked(juce::Button* button) override;
    void comboBoxChanged(juce::ComboBox* combo_box) override;

    void selectProvider(Cavey::AiProvider provider);
    void refreshPane();
    void refreshOllamaModels();
    void connectSelectedProvider();
    void saveSelectedProviderKey();
    void resetSelectedProviderKey();
    void setBusy(bool should_be_busy);
    void setStatus(const juce::String& status, bool is_error);

    CaveyAudioProcessor& processor_;
    CompletionCallback completion_callback_;
    Cavey::AiProvider selected_provider_ = Cavey::AiProvider::kOpenAI;
    juce::String displayed_environment_variable_;

    juce::TextButton openai_nav_button_ {"OpenAI"};
    juce::TextButton anthropic_nav_button_ {"Anthropic"};
    juce::TextButton ollama_nav_button_ {"Ollama"};

    juce::Label title_label_;
    juce::Label detail_label_;
    juce::Label stored_key_label_;
    juce::TextEditor api_key_editor_;
    juce::TextButton save_key_button_ {"Save"};
    juce::TextButton reset_key_button_ {"Reset"};
    juce::ComboBox ollama_model_box_;
    juce::TextButton refresh_models_button_ {"Refresh"};
    juce::TextButton connect_button_ {"Connect"};
    juce::Label status_label_;

    bool is_busy_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AiSetupComponent)
};
