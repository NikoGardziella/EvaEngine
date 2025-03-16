#include "pch.h"

#include "UUID.h"
#include <random>



namespace Engine {

	static std::random_device s_randomDevice;
	static std::mt19937_64 s_Engine(s_randomDevice());
	static std::uniform_int_distribution<uint64_t> s_uniformDistribution(0, UINT64_MAX);


	UUID::UUID()
		: m_UUID(s_uniformDistribution(s_Engine))
	{

	}

	UUID::UUID(uint64_t UUID)
		: m_UUID(UUID)
	{

	}

}