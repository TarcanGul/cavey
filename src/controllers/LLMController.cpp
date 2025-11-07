//
// Created by Tarcan Gul on 11/6/25.
//

#include "LLMController.h"

/**
 * Assumptions:
 * - Assume ollama is already running in port PORT.
 *
 * Steps:
 * - Make sure ollama is running in PORT
 * - Send the prompt to ollama via RPC with system prompt and context.
 * - Get the response back.
 * - Create a parameter with that.
 *
 * Spike parts:
 * - What to actually generate to make changes is sound
 * - The system prompt
 */
LLMController::LLMController() {}