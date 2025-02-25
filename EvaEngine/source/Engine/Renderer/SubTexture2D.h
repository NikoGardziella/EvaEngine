#pragma once
#include <Engine/Platform/OpenGl/OpenGLTexture.h>
#include "glm/glm.hpp"

namespace Engine {

	class SubTexture2D
	{


	public:
		SubTexture2D(const Ref<Texture2D>& texture, const glm::vec2& min, const glm::vec2& max);
		const glm::vec2* GetTexCoords() const { return m_texCoords;  }
		const Ref<Texture2D> GetTexture() const { return m_texture;  }

		//  for packed tilesheet
		static Ref<SubTexture2D> CreateFromCoordinates( const Ref<Texture2D>& texture, const glm::vec2& coords, const glm::vec2& cellSize, const glm::vec2& spriteSize = { 1, 1});
		
		// for padded tilesheet
		static Ref<SubTexture2D> CreateFromCoordinates(const Ref<Texture2D>& texture, const glm::vec2& coords, const glm::vec2& spriteSize, const int padding);

	private:
		Ref<Texture2D> m_texture;
		glm::vec2 m_texCoords[4];

	};

}


