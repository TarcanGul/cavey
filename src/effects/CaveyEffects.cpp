//
// Created by Tarcan Gul on 2/7/26.
//

#include "CaveyEffects.h"


Cavey::Range Cavey::Volume::getRange() {
    // 0 -> {1.0 , 1.0}
    // 1 -> {0.0, 1.0}
    return {1.0 - this->coefficient, 1.0};
}

Cavey::Range Cavey::LowPass::getRange() {
    // 0 -> {20000 , 20000}
    // 1 -> {20000, 1000}
    return {20000, 20000 - (19900 * coefficient)};
}

Cavey::Range Cavey::HighPass::getRange() {
    // 0 -> {20 , 0}
    // 1 -> {20, 20000}
    return {20, (2000 * coefficient)};
}

Cavey::Range Cavey::Reverb::getRange() {
    return { 0, coefficient };
}

Cavey::Range Cavey::Distortion::getRange() {
    return {0 , coefficient };
}
