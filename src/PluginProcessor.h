#pragma once

#include <JuceHeader.h>
#include "components/Parameter.h"
#include "types/CaveyTypes.h"
#include "types/BackendParameter.h"
#include <stdexcept>

class CaveyAudioProcessor : public juce::AudioProcessor {
public:
    CaveyAudioProcessor();
    ~CaveyAudioProcessor() override;

    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // Editor
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // Basic info
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return JucePlugin_WantsMidiInput; }
    bool producesMidi() const override { return JucePlugin_ProducesMidiOutput; }
    bool isMidiEffect() const override { return JucePlugin_IsMidiEffect; }
    double getTailLengthSeconds() const override { return 0.0; }

    // Programs (not used)
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    // State
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void addBackendParameter(const juce::String& parameterName, std::map<BaseEffect, float>);

    void setBackendParameterValue(const juce::String& parameterName, float value);
private:
    std::map<juce::String, BackendParameter *> parameters;
    juce::ADSR adsrEnvelope;
    juce::ADSR::Parameters adsrParameters;
    // std::unique_ptr<juce::FileLogger> fileLogger;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CaveyAudioProcessor)
};

