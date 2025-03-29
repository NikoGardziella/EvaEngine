#pragma once
#include <TaskScheduler.h>
#include <box2d/types.h>
#include <LockLessMultiReadPipe.h>


namespace Engine {


	class PhysicsTask : public enki::ITaskSet
	{
		public:

		PhysicsTask(int size, b2TaskCallback func, void* ctx);

		void ExecuteRange(enki::TaskSetPartition range_, uint32_t threadNum) override
		{
			if (m_taskFunc)
			{
				m_taskFunc(range_.start, range_.end, threadNum, taskContext);
			}
		}

	public:
		b2TaskCallback& m_taskFunc;
		void* taskContext;

	};


}

