#include "OpenAICostTracker.h"
#include <Engine/Core/Log.h>
// OpenAICostTracker.cpp


// Costs in USD per 1,000 tokens
constexpr double COST_PER_PROMPT_TOKEN = 0.0005 / 1000.0;
constexpr double COST_PER_RESPONSE_TOKEN = 0.0015 / 1000.0;

void OpenAICostTracker::AddRequest(int promptTokens, int responseTokens)
{
    m_totalPromptTokens += promptTokens;
    m_totalResponseTokens += responseTokens;

    double cost = (promptTokens * COST_PER_PROMPT_TOKEN) + (responseTokens * COST_PER_RESPONSE_TOKEN);
    EE_CORE_INFO("Estimated cost for this request: ${:.5f}", cost); // Adjust log format
}

void OpenAICostTracker::LogTotalCost() const
{
    double totalCost =
        (m_totalPromptTokens * COST_PER_PROMPT_TOKEN) +
        (m_totalResponseTokens * COST_PER_RESPONSE_TOKEN);

    EE_CORE_INFO("Total tokens used: prompt={}, response={}", m_totalPromptTokens, m_totalResponseTokens);
    EE_CORE_INFO("Estimated total OpenAI cost: ${:.5f}", totalCost);
}
