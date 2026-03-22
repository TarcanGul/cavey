//
// Created by Tarcan Gul on 2/7/26.
//

#pragma once

#include <utility>
#include "../types/CaveyTypes.h"

namespace Cavey {

    struct CoefficientGroupInitializer {
        float volume = 0.0f;
        float lowPass = 0.0f;
        float highPass = 0.0f;
        float reverb = 0.0f;
        float distortion = 0.0f;
    };

    struct Volume {
        float coefficient {0.0f};
        [[nodiscard]] Range getRange() const;

        explicit Volume (float coefficient) : coefficient(coefficient) {}
    };

    struct LowPass {
        float coefficient {0.0f};
        [[nodiscard]] Range getRange() const;

        explicit LowPass (float coefficient) : coefficient(coefficient) {}
    };

    struct HighPass {
        float coefficient {0.0f};
        [[nodiscard]] Range getRange() const;

        explicit HighPass (float coefficient) : coefficient(coefficient) {}
    };

    struct Reverb {
        float coefficient {0.0f};
        [[nodiscard]] Range getRange() const;

        explicit Reverb (float coefficient) : coefficient(coefficient) {}
    };

    struct Distortion {
        float coefficient {0.0f};
        [[nodiscard]] Range getRange() const;

        explicit Distortion (float coefficient) : coefficient(coefficient) {}
    };
}