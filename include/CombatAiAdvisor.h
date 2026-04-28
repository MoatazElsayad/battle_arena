// Gameplay-facing AI helper.
// This file should:
// - receive CombatSnapshot
// - build a short prompt
// - call OpenRouterClient
// - convert the reply into AiRecommendation
// Suggested functions:
// - constructor
// - buildSystemPrompt()
// - buildUserPrompt(...)
// - requestRecommendation(...)
// - parseRecommendation(...)
// - fallbackRecommendation(...)


#include <iostream>
#include <OpenRouterClient.h>
#include <Enums.h>
using namespace std;

QString build_prompt(const CombatSnapshot& snapshot);
void CombatAiAdvisor::requestAdvice(const CombatSnapshot& snapshot);
void CombatAiAdvisor::applyRecommendation(const AiRecommendation& recommendation);
AiRecommendaiton CombatAiAdvisor::ParseResponse(QString content);
