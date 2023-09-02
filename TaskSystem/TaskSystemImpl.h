#pragma once

#include "Task.h"
#include "Executor.h"
#include "IdGenerator.h"
#include "TaskSystem.h"

#include <map>
#include <functional>
#include <atomic>
#include <shared_mutex>
#include<queue>
#include <cassert>
#include<iostream>
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
			delete TaskSystemExecutor::self;
			TaskSystemExecutor::self = new TaskSystemExecutorImpl(threadCount);
		}

		TaskID ScheduleTask(std::unique_ptr<Task> task, int priority) override;

		void WaitForTask(TaskID task) override;

		void OnTaskCompleted(TaskID task, std::function<void(TaskID)>&& callback) override;

		void Register(const std::string& executorName, ExecutorConstructor constructor) override;

		/// <summary>
		/// Set stop threads and wait for them to exit
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

			int priority;

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


		/// <summary>
		/// Each task has a corresponding task context.
		/// </summary>
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

		friend struct CallBackExecutor;
	};
};