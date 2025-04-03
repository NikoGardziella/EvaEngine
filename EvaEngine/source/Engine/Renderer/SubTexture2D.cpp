#include "pch.h"
#include "SubTexture2D.h"

namespace Engine {



	SubTexture2D::SubTexture2D(const Ref<Texture2D>& texture, const glm::vec2& min, const glm::vec2& max)
		: m_texture(texture)
	{
		m_texCoords[0] = { min.x, min.y };
		m_texCoords[1] = { max.x, min.y };
		m_texCoords[2] = { max.x, max.y };
		m_texCoords[3] = { min.x, max.y };
	}

	Ref<SubTexture2D> SubTexture2D::CreateFromCoordinates(const Ref<Texture2D>& texture, const glm::vec2& coords, const glm::vec2& cellSize, const glm::vec2& spriteSize)
	{

		glm::vec2 min = { coords.x * (cellSize.x ) / texture->GetWidth(), (coords.y * (cellSize.y )) / texture->GetHeight() };
		glm::vec2 max = { (coords.x + spriteSize.x) * (cellSize.x ) / texture->GetWidth(), (coords.y + spriteSize.y) * (cellSize.y) / texture->GetHeight() };

		return std::make_shared<SubTexture2D>(texture, min, max);
	
	}

	Ref<SubTexture2D> SubTexture2D::CreateFromCoordinates(const Ref<Texture2D>& texture, const glm::vec2& coords, const glm::vec2& spriteSize, int padding)
	{
		float texWidth = texture->GetWidth();
		float texHeight = texture->GetHeight();

		glm::vec2 min = {
			(coords.x * (spriteSize.x + padding)) / texWidth,
			(coords.y * (spriteSize.y + padding)) / texHeight
		};

		glm::vec2 max = {
			((coords.x + 1) * (spriteSize.x + padding) - padding) / texWidth,
			((coords.y + 1) * (spriteSize.y + padding) - padding) / texHeight
		};

		return std::make_shared<SubTexture2D>(texture, min, max);
	}
}