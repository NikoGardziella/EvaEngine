#pragma once

class OpenAICostTracker
{
public:
    void AddRequest(int promptTokens, int responseTokens);
    void LogTotalCost() const;

private:
    int m_totalPromptTokens = 0;
    int m_totalResponseTokens = 0;
};
