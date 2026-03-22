//
// Created by Tarcan Gul on 2/7/26.
//

#pragma once

#include <utility>
#include "../types/CaveyTypes.h"

namespace Cavey {

    struct CoefficientGroupInitializer {
        double volume = 0;
        double lowPass = 0;
        double highPass = 0;
        double reverb = 0;
        double distortion = 0;
    };

    struct Volume {
        double coefficient {0.0};
        Range getRange();

        Volume (double coefficient) : coefficient(coefficient) {}
    };

    struct LowPass {
        double coefficient {0.0};
        Range getRange();

        LowPass (double coefficient) : coefficient(coefficient) {}
    };

    struct HighPass {
        double coefficient {0.0};
        Range getRange();

        HighPass (double coefficient) : coefficient(coefficient) {}
    };

    struct Reverb {
        double coefficient {0.0};
        Range getRange();

        Reverb (double coefficient) : coefficient(coefficient) {}
    };

    struct Distortion {
        double coefficient {0.0};
        Range getRange();

        Distortion (double coefficient) : coefficient(coefficient) {}
    };
}