#pragma once

#include "Task.h"
#include "Executor.h"
#include "IdGenerator.h"
#include "TaskSystem.h"

#include <map>
#include <functional>
#include <atomic>
#include <shared_mutex>
#include <queue>
#include <cassert>
#include <iostream>

namespace TaskSystem {

	/// <summary>
	/// Implementation of TaskSystemExecutor with locks
	/// </summary>
	class TaskSystemExecutorImpl : public TaskSystemExecutor {
	private:
		TaskSystemExecutorImpl() = delete;
		TaskSystemExecutorImpl(int threadCount) : TaskSystemExecutor(threadCount), threadCount(threadCount) {
			// Load callback executor shared library
			TS_LOAD_LIBARY("CallbackExecutor", *this);

			for (int i = 0; i < threadCount; i++) {
				threads.push_back(std::thread(&TaskSystemExecutorImpl::workerFun, this, i));
			}
		};
	public:
		// Non copy-consructable
		TaskSystemExecutorImpl(const TaskSystemExecutor&) = delete;
		// Non copyable
		TaskSystemExecutorImpl& operator=(const TaskSystemExecutor&) = delete;

		static void Init(int threadCount) {
			static std::mutex init_mutex;
			if (TaskSystemExecutor::self) {
				return;
			}
			std::lock_guard<std::mutex> TSInitLock(init_mutex);
			if (TaskSystemExecutor::self) {
				return;
			}
			TaskSystemExecutor::self = new TaskSystemExecutorImpl(threadCount);
		}

		/// <summary>
		/// Schedule task with given priority.
		/// Create executor instance and push task to task priority queue and task map.
		/// </summary>
		/// <returns></returns>
		TaskID ScheduleTask(std::unique_ptr<Task> task, int priority) override;

		/// <summary>
		/// Wait for task with given taskid to finish. A task is finished when callbacksComplete is true.
		/// </summary>
		/// <param name="task"></param>
		void WaitForTask(TaskID task) override;

		/// <summary>
		/// Register a callback to be called when normal task steps have been executed.
		/// </summary>
		/// <param name="task"></param>
		/// <param name="callback"></param>
		void OnTaskCompleted(TaskID task, std::function<void(TaskID)>&& callback) override;

		void Register(const std::string& executorName, ExecutorConstructor constructor) override;

		/// <summary>
		/// Set stop threads and wait for them to exit.
		/// Delete instance of TaskSystemExecutorImpl.
		/// </summary>
		void Terminate() override;

	private:
		/// <summary>
		/// Function executed by each started thread
		/// </summary>
		void workerFun(int tid);
	protected:
		struct TaskContext {
			TaskID id;
			std::shared_ptr<Executor> exec;

			/// <summary>
			/// Callbacks function that should be called on task complete
			/// </summary>
			std::vector<std::function<void(TaskID)>> onCompleteCallbacks;

			std::shared_ptr<std::atomic<bool>> taskComplete;
			std::shared_ptr<std::atomic<bool>> callbacksComplete;

			/// <summary>
			/// Mutex used to wait for task to finish execution (this includes task + callbacks)
			/// </summary>
			std::mutex waitMutex;

			/// <summary>
			/// Conditional variable used to wait for task to finish execution (this includes task + callbacks)
			/// </summary>
			std::condition_variable cv;

			int priority = 0;

			struct CMP_priority {
				bool operator() (const std::shared_ptr<TaskContext>& lhs, const std::shared_ptr<TaskContext>& rhs) const
				{
					return lhs->priority < rhs->priority;
				}

			};
		};

		struct CallbackTaskParams : Task {
			std::shared_ptr<TaskContext> tc;

			CallbackTaskParams(std::shared_ptr<TaskContext> tc) : tc(tc) {};

			virtual std::optional<void*> GetAnyParam(const std::string& name) const {
				if (name == "Context") {
					return (void*) &(*tc);
				}
				return std::nullopt;
			}
			virtual std::string GetExecutorName() const { return "callbackExecutor"; }
		};

		int threadCount;

		/// <summary>
		/// TaskContext map (Task Map) used for context lookup based on TaskID.
		/// </summary>
		std::map<TaskID, std::shared_ptr<TaskContext>, TaskID::IdCmp> idTaskMap;

		/// <summary>
		/// Mutex for TaskContext map. 
		/// </summary>
		std::shared_mutex TaskMapMutex;

		/// <summary>
		/// Unique id generator.
		/// </summary>
		IdGenerator idGen;

		/// <summary>
		// Task priority queue. Provide access to task with highest priority.
		/// </summary>
		std::priority_queue<std::shared_ptr<TaskContext>, std::vector<std::shared_ptr<TaskContext>>, TaskContext::CMP_priority> taskPQ;
		
		/// <summary>
		/// Mutex for Task Priority queue.
		/// </summary>
		std::shared_mutex taskPQMutex;

		std::vector<std::thread> threads;

		std::atomic<bool> terminateThreads = false;

		// State 0 = work
		// State 1 = sleep
		// State 3 = terminate
		/*std::atomic<int> threadState = 0;
		std::condition_variable */

		std::shared_ptr<TaskContext*> cur_executed_task;

		friend struct CallBackExecutor;
	};
};