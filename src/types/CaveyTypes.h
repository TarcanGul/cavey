//
// Created by Tarcan Gul on 11/16/25.
//

#pragma once
#include <map>
#include <JuceHeader.h>

enum class BaseEffect {
    VOLUME,
};

inline const std::map<BaseEffect, juce::String> BASE_EFFECT_STRINGS = {
        {BaseEffect::VOLUME, "VOLUME"},
};
