#include "Task.h"
#include "Executor.h"
#include "TaskSystemImpl.h"
#include "TaskSystem.h"
#include <thread>
namespace TaskSystem {
	typedef TaskSystemExecutor::TaskID TaskID;

	TaskID defaultId = { -1 };
	std::vector < std::function<void(TaskID)>> emptyCallbackList;

	/// <summary>
	/// Special executor. Each step executes a callbacks of an already finished task.
	/// A callbackExeutor task is only scheduled when a task has registered callbacks.
	/// A callbackExecutor task should not have callbacks. 
	/// </summary>
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
		/// <summary>
		/// Number of callbacks complete
		/// </summary>
		std::atomic<int> completeC = 0;
		/// <summary>
		/// Task Context of completed task
		/// TODO: Executor knows about TaskSystemExecutorImpl
		/// </summary>
		std::shared_ptr<TaskSystemExecutorImpl::TaskContext> tc;
		/// <summary>
		/// Total number of callbacks.
		/// </summary>
		unsigned int callbackCount = 0;

		void setCallbacksComplete() {
			{
				std::lock_guard<std::mutex> l(tc->waitMutex);
				tc->callbacksComplete->store(true);
			}
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