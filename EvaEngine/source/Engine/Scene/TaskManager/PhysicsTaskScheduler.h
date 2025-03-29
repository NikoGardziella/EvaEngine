#pragma once
#include "Engine/Scene/TaskManager/PhysicsTask.h"


#include <TaskScheduler.h>
#include <box2d/types.h>


namespace Engine {

	class PhysicsTaskScheduler
	{
	public:
		enki::TaskScheduler taskScheduler;

		PhysicsTaskScheduler()
		{
			taskScheduler.Initialize(std::thread::hardware_concurrency());  // Use max threads
		}



		static void* EnqueueTask(b2TaskCallback task, int itemCount, int minRange, void* taskContext, void* userContext)
		{
			auto* scheduler = static_cast<PhysicsTaskScheduler*>(userContext);
			auto* physicsTask = new PhysicsTask(itemCount, task, taskContext);
			scheduler->taskScheduler.AddTaskSetToPipe(physicsTask);
			return physicsTask;
		}

		static void FinishTask(void* taskContext, void* userContext)
		{
			auto* scheduler = static_cast<PhysicsTaskScheduler*>(userContext);
			scheduler->taskScheduler.WaitforAll();
		}
	};
}
