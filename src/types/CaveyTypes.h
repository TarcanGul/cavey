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
        double low {0.0};
        double high {0.0};

        Range(const double& low, const double& high) : low(low), high(high) {}
    };
}
