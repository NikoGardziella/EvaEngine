#pragma once

#include "json.hpp"
#include "OpenAICostTracker.h"

namespace Engine {

    namespace AI {

        class OpenAIClient {
        public:
            // Initialize with your API key (you can load from env var or config file)
            explicit OpenAIClient(const std::string& apiKey);

            // Sends a chat?completion request and returns the raw JSON response
            std::string ChatCompletion(const nlohmann::json& requestBody);

            // High?level helper: send gameplay prompt and return parsed JSON
            std::string CreateGameplayJSON(const std::string& prompt, nlohmann::json existingEntitie);

        private:
            std::string m_apiKey;
            std::string m_endpoint = "https://api.openai.com/v1/chat/completions";

            OpenAICostTracker g_costTracker;
        private:
        };

    }
}