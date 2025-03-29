#include "pch.h"
#include "PhysicsTask.h"



namespace Engine {

	PhysicsTask::PhysicsTask(int size, b2TaskCallback func, void* ctx)
		: enki::ITaskSet(size), taskContext(ctx), m_taskFunc(*func)
	{

	}


}
