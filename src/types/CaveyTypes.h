//
// Created by Tarcan Gul on 11/16/25.
//

#pragma once
#include <map>
#include <JuceHeader.h>
#include <utility>

namespace Cavey {
    enum class BaseEffect {
        VOLUME,
        LOW_PASS,
        HIGH_PASS,
        REVERB,
        DISTORTION
    };

    struct Range {
        float low {0.0f};
        float high {0.0f};

        Range(const float low, const float high) : low(low), high(high) {}
    };
}
