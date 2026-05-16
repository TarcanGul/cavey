#include "AiSetupComponent.h"

#include "../PluginProcessor.h"

namespace {

constexpr int kDialogWidth = 520;
constexpr int kDialogHeight = 320;
constexpr int kNavWidth = 130;
constexpr int kMargin = 16;
constexpr int kRowHeight = 28;

juce::String GetEnvironmentVariableName(Cavey::AiProvider provider) {
    switch (provider) {
        case Cavey::AiProvider::kOpenAI:
            return "OPENAI_API_KEY";
        case Cavey::AiProvider::kAnthropic:
            return "ANTHROPIC_API_KEY";
        case Cavey::AiProvider::kOllama:
        case Cavey::AiProvider::kNone:
            break;
    }

    return {};
}

}  // namespace

AiSetupComponent::AiSetupComponent(
        CaveyAudioProcessor& processor,
        CompletionCallback completion_callback)
    : processor_(processor),
      completion_callback_(std::move(completion_callback)) {
    setSize(kDialogWidth, kDialogHeight);

    for (auto* button : std::initializer_list<juce::Button*>{
             &openai_nav_button_,
             &anthropic_nav_button_,
             &ollama_nav_button_,
             &refresh_models_button_,
             &save_key_button_,
             &reset_key_button_,
             &main_provider_toggle_,
             &connect_button_}) {
        button->addListener(this);
        addAndMakeVisible(button);
    }

    title_label_.setFont(juce::Font(juce::FontOptions(18.0f, juce::Font::bold)));
    detail_label_.setColour(juce::Label::textColourId, juce::Colours::grey);
    stored_key_label_.setColour(juce::Label::textColourId,
                                juce::Colours::lightgreen);
    status_label_.setJustificationType(juce::Justification::centredLeft);

    api_key_editor_.setPasswordCharacter(0x2022);
    ollama_model_box_.addListener(this);

    for (auto* component : std::initializer_list<juce::Component*>{
             &title_label_,
             &detail_label_,
             &stored_key_label_,
             &api_key_editor_,
             &ollama_model_box_,
             &main_provider_toggle_,
             &status_label_}) {
        addAndMakeVisible(component);
    }

    refreshPane();
}

AiSetupComponent::~AiSetupComponent() {
    for (auto* button : std::initializer_list<juce::Button*>{
             &openai_nav_button_,
             &anthropic_nav_button_,
             &ollama_nav_button_,
             &refresh_models_button_,
             &save_key_button_,
             &reset_key_button_,
             &main_provider_toggle_,
             &connect_button_}) {
        button->removeListener(this);
    }

    ollama_model_box_.removeListener(this);
    api_key_editor_.clear();
}

void AiSetupComponent::paint(juce::Graphics& graphics) {
    graphics.fillAll(getLookAndFeel().findColour(
            juce::ResizableWindow::backgroundColourId));
    graphics.setColour(juce::Colours::darkgrey);
    graphics.drawVerticalLine(kNavWidth, 0.0f,
                              static_cast<float>(getHeight()));
}

void AiSetupComponent::resized() {
    auto bounds = getLocalBounds();
    auto nav_bounds = bounds.removeFromLeft(kNavWidth).reduced(kMargin);
    openai_nav_button_.setBounds(nav_bounds.removeFromTop(kRowHeight));
    nav_bounds.removeFromTop(8);
    anthropic_nav_button_.setBounds(nav_bounds.removeFromTop(kRowHeight));
    nav_bounds.removeFromTop(8);
    ollama_nav_button_.setBounds(nav_bounds.removeFromTop(kRowHeight));

    auto pane_bounds = bounds.reduced(kMargin);
    title_label_.setBounds(pane_bounds.removeFromTop(30));
    detail_label_.setBounds(pane_bounds.removeFromTop(24));
    pane_bounds.removeFromTop(12);
    stored_key_label_.setBounds(pane_bounds.removeFromTop(24));
    pane_bounds.removeFromTop(8);

    auto control_bounds = pane_bounds.removeFromTop(32);
    auto api_key_bounds = control_bounds;
    auto key_button_bounds = api_key_bounds.removeFromRight(96);
    api_key_editor_.setBounds(api_key_bounds.reduced(0, 1));
    save_key_button_.setBounds(key_button_bounds.reduced(6, 0));
    reset_key_button_.setBounds(key_button_bounds.reduced(6, 0));
    ollama_model_box_.setBounds(control_bounds);

    auto refresh_bounds = ollama_model_box_.getBounds().removeFromRight(96);
    refresh_models_button_.setBounds(refresh_bounds.reduced(4, 0));

    pane_bounds.removeFromTop(16);
    main_provider_toggle_.setBounds(pane_bounds.removeFromTop(28));
    pane_bounds.removeFromTop(8);
    connect_button_.setBounds(pane_bounds.removeFromTop(34).removeFromLeft(120));
    pane_bounds.removeFromTop(10);
    status_label_.setBounds(pane_bounds.removeFromTop(44));
}

void AiSetupComponent::buttonClicked(juce::Button* button) {
    if (button == &openai_nav_button_) {
        selectProvider(Cavey::AiProvider::kOpenAI);
    } else if (button == &anthropic_nav_button_) {
        selectProvider(Cavey::AiProvider::kAnthropic);
    } else if (button == &ollama_nav_button_) {
        selectProvider(Cavey::AiProvider::kOllama);
    } else if (button == &refresh_models_button_) {
        refreshOllamaModels();
    } else if (button == &save_key_button_) {
        saveSelectedProviderKey();
    } else if (button == &reset_key_button_) {
        resetSelectedProviderKey();
    } else if (button == &main_provider_toggle_) {
        updateMainProviderSelection();
    } else if (button == &connect_button_) {
        connectSelectedProvider();
    }
}

void AiSetupComponent::comboBoxChanged(juce::ComboBox* combo_box) {
    juce::ignoreUnused(combo_box);
}

void AiSetupComponent::selectProvider(Cavey::AiProvider provider) {
    selected_provider_ = provider;
    refreshPane();
}

void AiSetupComponent::refreshPane() {
    openai_nav_button_.setToggleState(selected_provider_ == Cavey::AiProvider::kOpenAI,
                                      juce::dontSendNotification);
    anthropic_nav_button_.setToggleState(
            selected_provider_ == Cavey::AiProvider::kAnthropic,
            juce::dontSendNotification);
    ollama_nav_button_.setToggleState(selected_provider_ == Cavey::AiProvider::kOllama,
                                      juce::dontSendNotification);

    ollama_model_box_.setVisible(selected_provider_ == Cavey::AiProvider::kOllama);
    refresh_models_button_.setVisible(
            selected_provider_ == Cavey::AiProvider::kOllama);

    title_label_.setText(Cavey::ToProviderDisplayName(selected_provider_),
                         juce::dontSendNotification);
    main_provider_toggle_.setVisible(selected_provider_
                                     != Cavey::AiProvider::kNone);
    main_provider_toggle_.setToggleState(
            processor_.getMainAiProvider() == selected_provider_,
            juce::dontSendNotification);

    if (selected_provider_ == Cavey::AiProvider::kOllama) {
        detail_label_.setText("Use a model already downloaded in Ollama.",
                              juce::dontSendNotification);
        stored_key_label_.setColour(juce::Label::textColourId,
                                    ollama_model_box_.getText().isNotEmpty()
                                            ? juce::Colours::lightgreen
                                            : juce::Colours::grey);
        stored_key_label_.setText(ollama_model_box_.getText().isNotEmpty()
                                          ? "Model selected"
                                          : "Select a local model",
                                  juce::dontSendNotification);
        if (ollama_model_box_.getNumItems() == 0) {
            refreshOllamaModels();
        }
        api_key_editor_.setVisible(false);
        save_key_button_.setVisible(false);
        reset_key_button_.setVisible(false);
        displayed_environment_variable_.clear();
    } else {
        const auto environment_variable = GetEnvironmentVariableName(
                selected_provider_);
        if (displayed_environment_variable_ != environment_variable) {
            displayed_environment_variable_ = environment_variable;
            api_key_editor_.clear();
        }
        api_key_editor_.setTextToShowWhenEmpty(environment_variable,
                                               juce::Colours::grey);
        api_key_editor_.repaint();
        detail_label_.setText("Set " + environment_variable,
                              juce::dontSendNotification);
        const bool has_environment_variable =
                processor_.hasRequiredEnvironmentVariable();
        stored_key_label_.setColour(juce::Label::textColourId,
                                    has_environment_variable
                                            ? juce::Colours::lightgreen
                                            : juce::Colours::orange);
        stored_key_label_.setText(
                has_environment_variable
                        ? environment_variable + " is set"
                        : environment_variable + " is not set",
                juce::dontSendNotification);
        api_key_editor_.setVisible(true);
        save_key_button_.setVisible(!has_environment_variable);
        reset_key_button_.setVisible(has_environment_variable);
    }

    connect_button_.setButtonText("Connect");
    resized();
}

void AiSetupComponent::refreshOllamaModels() {
    setBusy(true);
    setStatus({}, false);

    const juce::Component::SafePointer<AiSetupComponent> safe_this(this);
    CaveyAudioProcessor& processor = processor_;
    std::thread([safe_this, &processor] {
        try {
            const auto models = processor.fetchOllamaModels();
            juce::MessageManager::callAsync([safe_this, models] {
                if (safe_this == nullptr) {
                    return;
                }

                safe_this->ollama_model_box_.clear();
                for (int i = 0; i < models.size(); ++i) {
                    safe_this->ollama_model_box_.addItem(models[i], i + 1);
                }
                if (models.size() > 0) {
                    safe_this->ollama_model_box_.setSelectedId(1);
                }
                safe_this->setBusy(false);
                safe_this->refreshPane();
            });
        } catch (const std::exception& exception) {
            juce::MessageManager::callAsync([safe_this,
                                             message = juce::String(exception.what())] {
                if (safe_this == nullptr) {
                    return;
                }

                safe_this->setBusy(false);
                safe_this->setStatus(message, true);
            });
        }
    }).detach();
}

void AiSetupComponent::saveSelectedProviderKey() {
    const auto result = processor_.saveProviderEnvironmentVariable(
            selected_provider_,
            api_key_editor_.getText());
    setStatus(result.message, !result.saved);
    if (result.saved) {
        api_key_editor_.clear();
        refreshPane();
    }
}

void AiSetupComponent::resetSelectedProviderKey() {
    const auto result = processor_.saveProviderEnvironmentVariable(
            selected_provider_,
            api_key_editor_.getText());
    setStatus(result.message, !result.saved);
    if (result.saved) {
        api_key_editor_.clear();
        refreshPane();
    }
}

void AiSetupComponent::updateMainProviderSelection() {
    const auto current_main_provider = processor_.getMainAiProvider();
    if (main_provider_toggle_.getToggleState()
        && current_main_provider != Cavey::AiProvider::kNone
        && current_main_provider != selected_provider_) {
        main_provider_toggle_.setToggleState(false, juce::dontSendNotification);
        setStatus(processor_.getMainAiProviderName()
                          + " is the main provider. Disable it first.",
                  true);
        return;
    }

    const auto provider_to_save = main_provider_toggle_.getToggleState()
            ? selected_provider_
            : Cavey::AiProvider::kNone;
    const auto result = processor_.saveMainAiProvider(provider_to_save);
    setStatus(result.message, !result.saved);
    refreshPane();
    if (completion_callback_) {
        completion_callback_(result.saved, result.message);
    }
}

void AiSetupComponent::connectSelectedProvider() {
    Cavey::ProviderConnectionConfig config;
    if (selected_provider_ == Cavey::AiProvider::kOllama) {
        config.ollama_model = ollama_model_box_.getText();
        juce::Logger::writeToLog("ollama_model is " + config.ollama_model);
    }

    setBusy(true);
    setStatus({}, false);
    const juce::Component::SafePointer<AiSetupComponent> safe_this(this);
    CaveyAudioProcessor& processor = processor_;

    std::thread([safe_this, &processor, config] {
        auto result = processor.connectAiProvider(config);
        juce::MessageManager::callAsync([safe_this, result] {
            if (safe_this == nullptr) {
                return;
            }

            safe_this->setBusy(false);
            safe_this->setStatus(result.message, !result.connected);
            safe_this->refreshPane();
            if (safe_this->completion_callback_) {
                safe_this->completion_callback_(result.connected, result.message);
            }
        });
    }).detach();
}

void AiSetupComponent::setBusy(bool should_be_busy) {
    is_busy_ = should_be_busy;
    connect_button_.setEnabled(!is_busy_);
    refresh_models_button_.setEnabled(!is_busy_);
    save_key_button_.setEnabled(!is_busy_);
    reset_key_button_.setEnabled(!is_busy_);
    main_provider_toggle_.setEnabled(!is_busy_);
}

void AiSetupComponent::setStatus(const juce::String& status, bool is_error) {
    status_label_.setText(status, juce::dontSendNotification);
    status_label_.setColour(juce::Label::textColourId,
                            is_error ? juce::Colours::orange
                                     : juce::Colours::lightgreen);
}
