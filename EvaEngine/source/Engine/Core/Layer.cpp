#include "pch.h"
#include "Layer.h"

namespace Engine {

	Layer::Layer(const std::string& debugName)
		: m_DebugName(debugName)
	{
	}

	Layer::~Layer()
	{
		EE_CORE_ERROR("~Layer() destructor");
	}

}