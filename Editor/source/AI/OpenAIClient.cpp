#include "OpenAIClient.h"
#include "curl/curl.h"
#include <Engine/Core/Log.h>

#include <sstream>
#include <Engine/Debug/Instrumentor.h>

using json = nlohmann::json;

namespace Engine
{

    namespace AI {

        OpenAIClient::OpenAIClient(const std::string& apiKey)
            : m_apiKey(apiKey)
		{
			
            if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0)
            {
                EE_CORE_ERROR("Failed to initialize libcurl globally.");
                // Optionally throw or set a valid state flag.
            }
        }

        // libcurl write callback
        static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) 
        {
            ((std::string*)userp)->append((char*)contents, size * nmemb);
            return size * nmemb;
        }

        std::string OpenAIClient::ChatCompletion(const json& requestBody)
        {
            if (m_apiKey.empty())
            {
                EE_CORE_ERROR("OpenAI API key is missing. Set the OPENAI_API_KEY environment variable.");
                return {};
            }

            CURL* curl = curl_easy_init();
            if (!curl)
            {
                EE_CORE_ERROR("Failed to initialize CURL");
                return {};
            }


            std::string readBuffer;
            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, ("Authorization: Bearer " + m_apiKey).c_str());
            headers = curl_slist_append(headers, "Content-Type: application/json");

            std::string body = requestBody.dump();

            curl_easy_setopt(curl, CURLOPT_URL, m_endpoint.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

            EE_CORE_TRACE("Sending OpenAI request: {}", body);

            CURLcode res = curl_easy_perform(curl);
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            if (res != CURLE_OK)
            {
                EE_CORE_ERROR("CURL error: {}", curl_easy_strerror(res));
                return {};
            }

            EE_CORE_TRACE("Received OpenAI response: {}", readBuffer);

           

            return readBuffer;
        }

        std::string OpenAIClient::CreateGameplayJSON(const std::string& prompt, nlohmann::json existingEntitie)
        {
            EE_PROFILE_FUNCTION();


            json req;
            
            req = {
                {"model", "gpt-3.5-turbo"},
                {"messages", json::array({
                    {{"role", "system"}, {"content",
                        "You are an ECS game engine assistant. Output ONLY valid JSON describing entities and their components."
                        "Output a top-level 'entities' array Each NEW entity must have a top - level 'id' field representing a 64 - bit unsigned integer UUID."
                        "Do not include id when creating new entities the engine will assign them."
                        "Only include an id if you're modifying an existing entity."
                        "Each entity must be an object with a 'components' array.Each component must be an object with a 'type' field(e.g., 'TransformComponent') and all relevant fields for that component."
                        "Do not use component types as top - level keys inside each entity."
                        "Supported components : TagComponent(string: Tag, add name for the entity), TransformComponent(Use this for most entities. vec3: Translation, Rotation, Scale), SpriteRendererComponent(Texture),"
                        "ProjectileComponent, BoxCollider2DComponent, CircleCollider2DComponent. NPCAIMovementComponent, NPCAIVisionComponent"
                        "HealthComponent(float: 'Health')"
                        "The 'SpriteRendererComponent'Texture must be one of : wall, enemy, enemy, 'player'"
                    }},
                })},
                {"temperature", 0.7},
                {"max_tokens", 1000}
            };

            req["messages"].push_back({
                {"role", "user"},
                {"content", prompt}
                });

            req["messages"].push_back({
                {"role", "user"},
                {"content", existingEntitie.dump()} // Convert the JSON to a string
                });
           
            std::string resp;
            try
            {
                resp = ChatCompletion(req);
            }
            catch (const std::exception& e)
            {
                EE_CORE_ERROR("ChatCompletion failed: {}", e.what());
                return {};
            }

            json parsedResp = json::parse(resp);

            EE_CORE_TRACE("OpenAI raw response:\n{}", parsedResp.dump(4));

            if (!parsedResp.contains("choices") || !parsedResp["choices"].is_array() || parsedResp["choices"].empty())
            {
                EE_CORE_WARN("Response is missing 'choices' or it's invalid:\n{}", parsedResp.dump(4));
                return {};
            }

            const auto& message = parsedResp["choices"][0]["message"];
            if (!message.contains("content") || !message["content"].is_string())
            {
                EE_CORE_WARN("'message.content' is missing or not a string.");
                return {};
            }

            std::string content = message["content"].get<std::string>();

            if (content.empty() || content.find_first_not_of(" \t\n\r") == std::string::npos)
            {
                EE_CORE_WARN("Response content is empty or whitespace.");
                return {};
            }

            // Try parsing the content as JSON
            try
            {
                json parsed = json::parse(content); // << key line
                if (!parsed.is_object() && !parsed.is_array())
                {
                    EE_CORE_WARN("Parsed JSON is not an object or array.");
                    return {};
                }

                // You could validate here that "entities" exists or "components" are arrays, etc.
            }
            catch (const std::exception& e)
            {
                EE_CORE_ERROR("Failed to parse content as JSON:\n{}\nError: {}", content, e.what());
                return {};
            }

            try
            {
                json parsed = json::parse(content);

                if (!parsed.contains("entities") || !parsed["entities"].is_array())
                {
                    EE_CORE_WARN("Parsed content does not contain a valid 'entities' array:\n{}", parsed.dump(4));
                    return {};
                }
            }
            catch (const std::exception& e)
            {
                EE_CORE_WARN("Failed to parse content as JSON: {}\nRaw content:\n{}", e.what(), content);
                return {};
            }

            return content;
        }






    }

}
