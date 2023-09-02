#include "Task.h"
#include "Executor.h"
#include "TaskSystemImpl.h"
#include "TaskSystem.h"
#include <thread>
namespace TaskSystem {
	typedef TaskSystemExecutor::TaskID TaskID;

	TaskID defaultId = { -1 };
	std::vector < std::function<void(TaskID)>> emptyCallbackList;

	struct CallBackExecutor : TaskSystem::Executor {
		CallBackExecutor(
			std::unique_ptr<TaskSystem::Task> taskToExecute)
			: Executor(std::move(taskToExecute)) {



			tc = std::shared_ptr< TaskSystemExecutorImpl::TaskContext>(
				(TaskSystemExecutorImpl::TaskContext*)(task->GetAnyParam("Context").value())
			);
			if (!tc) {
				throw std::exception("No task context passed to CallBackExecutor");
			}
			callbackCount = tc->onCompleteCallbacks.size();
			
		}

		virtual ~CallBackExecutor() {};
		virtual ExecStatus ExecuteStep(int threadIndex, int threadCount);

	private:
		std::atomic<int> completeC = 0;
		/// <summary>
		/// Task Context of completed task
		/// TODO: Executor knows about TaskSystemExecutorImpl
		/// </summary>
		std::shared_ptr<TaskSystemExecutorImpl::TaskContext> tc;
		unsigned int callbackCount = 0;

		void setCallbacksComplete() {
			tc->callbacksComplete->store(true);
			tc->cv.notify_all();
		}
	};

	TaskSystem::Executor* ExecutorConstructorImpl(std::unique_ptr<TaskSystem::Task> taskToExecute) {
		return new CallBackExecutor(std::move(taskToExecute));
	}

	IMPLEMENT_ON_INIT() {

		ts.Register("callbackExecutor", &ExecutorConstructorImpl);
	}
}