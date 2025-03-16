#pragma once

#include <xhash>

namespace Engine {


	class UUID
	{
	public:
		UUID();
		UUID(uint64_t UUID);
		UUID(const UUID&) = default;


		operator uint64_t() const { return m_UUID;  }

	private:

		uint64_t m_UUID;

	};
}

namespace std {

	template<>
	struct hash<Engine::UUID>
	{
		std::size_t operator()(const Engine::UUID& uuid) const
		{
			// Ensure UUID has a conversion method to uint64_t.
			return std::hash<uint64_t>{}(static_cast<uint64_t>(uuid));
		}
	};

} 
