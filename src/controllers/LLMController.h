//
// Created by Tarcan Gul on 11/6/25.
//

#pragma once

#include <JuceHeader.h>

/**
 * Interface for LLM communication
 */
struct LLMController {
    virtual String prompt(String const& prompt) = 0;
    virtual ~LLMController() = default;
};
