#pragma once

#include "Task.h"
#include "Executor.h"
#include "IdGenerator.h"

#include <map>
#include <functional>
#include <atomic>
#include <shared_mutex>
#include<queue>
#include <cassert>
#include<iostream>
namespace TaskSystem {
	/// <summary>
	/// Blocking logging function.
	/// </summary>
	/// <param name="msg"></param>
	/// <param name="tid"></param>
	static void logThread_(std::string msg, int tid) {
		static std::mutex m;
		std::unique_lock<std::mutex> printLock(m);
		std::cout << "I :" << tid << " " << msg << std::endl;
	};
//#define LOGGING 1
#ifdef LOGGING
#define logThread(msg, tid) logThread_(msg, tid);
#else
#define logThread(msg, tid) ;
#endif

	void TS_LOAD_LIBARY(const std::string& libName, TaskSystem::TaskSystemExecutor& ts);

	/**
	 * @brief The task system main class that can accept tasks to be scheduled and execute them on multiple threads
	 *
	 */
	struct TaskSystemExecutor {
	protected:
		TaskSystemExecutor(int threadCount) {};
	public:
		TaskSystemExecutor(const TaskSystemExecutor&) = delete;
		TaskSystemExecutor& operator=(const TaskSystemExecutor&) = delete;

		static TaskSystemExecutor& GetInstance();

		/**
		 * @brief Initialisation called once by the main application to allocate needed resources and start threads
		 *
		 * @param threadCount the desired number of threads to utilize
		 */
		static void Init(int threadCount) {
			delete self;
			self = new TaskSystemExecutor(threadCount);
		}

		struct TaskID {
			int id;
			struct IdCmp
			{
				bool operator() (const TaskID& lhs, const TaskID& rhs) const
				{
					return lhs.id < rhs.id;
				}
			};
		};

		/**
		 * @brief Schedule a task with specific priority to be executed
		 *
		 * @param task the parameters describing the task, Executor will be instantiated based on the expected name
		 * @param priority the task priority, bigger means executer sooner
		 * @return TaskID unique identifier used in later calls to wait or schedule callbacks for tasks
		 */
		virtual TaskID ScheduleTask(std::unique_ptr<Task> task, int priority) {
			std::unique_ptr<Executor> exec(executorConstructors[task->GetExecutorName()](std::move(task)));

			while (exec->ExecuteStep(0, 1) != Executor::ExecStatus::ES_Stop)
				;

			return TaskID{};
		}

		/**
		 * @brief Blocking wait for a given task. Does not block if the task has already finished
		 *
		 * @param task the task to wait for
		 */
		virtual void WaitForTask(TaskID task) {
			return;
		}

		/**
		 * @brief Register a callback to be executed when a task has finished executing. Executes the callbacl
		 *        immediately if the task has already finished
		 *
		 * @param task the task that was previously scheduled
		 * @param callback the callback to be executed
		 */
		virtual void OnTaskCompleted(TaskID task, std::function<void(TaskID)>&& callback) {
			callback(task);
		}

		/**
		 * @brief Load a dynamic library from a path and attempt to call OnLibraryInit
		 *
		 * @param path the path to the dynamic library
		 * @return true when OnLibraryInit is found, false otherwise
		 */
		virtual bool LoadLibrary(const std::string& path);

		/**
		 * @brief Register an executor with a name and constructor function. Should be called from
		 *        inside the dynamic libraries defining executors.
		 *
		 * @param executorName the name associated with the executor
		 * @param constructor constructor returning new instance of the executor
		 */
		virtual void Register(const std::string& executorName, ExecutorConstructor constructor) {
			executorConstructors[executorName] = constructor;
		}
		
		/// <summary>
		/// Stop worker threads. Delete instance of TaskSystemExecutor.
		/// </summary>
		virtual void Terminate() {};

	protected:
		static TaskSystemExecutor* self;
		std::map<std::string, ExecutorConstructor> executorConstructors;
	};
};