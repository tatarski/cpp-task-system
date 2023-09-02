#include <cassert>
#include<iostream>
#include <shared_mutex>
#include "TaskSystemImpl.h"

typedef TaskSystem::TaskSystemExecutor::TaskID TaskID;


namespace TaskSystem {
	void TS_LOAD_LIBARY(const std::string& libName, TaskSystem::TaskSystemExecutor& ts) {
#if defined(_WIN32) || defined(_WIN64)
		const bool libLoaded = ts.LoadLibrary(libName + ".dll");
#elif defined(__APPLE__)
		const bool libLoaded = ts.LoadLibrary("lib" + libName + ".dylib");
#elif defined(__linux__)
		const bool libLoaded = ts.LoadLibrary("lib" + libName + ".so");
#endif
		assert(libLoaded);
	}


	TaskID TaskSystemExecutorImpl::ScheduleTask(std::unique_ptr<Task> task, int priority) {
		logThread("Starting task schedule. Init task context.", 999999);

		// Initialize executor with given task
		std::string execName = task->GetExecutorName();

		std::shared_ptr<Executor> exec(executorConstructors[execName](std::move(task)));

		TaskID tid = { idGen.getId() };
		std::cout << "Still alive" << std::endl;

		std::shared_ptr<TaskContext> tc = std::make_shared<TaskContext>();
		tc->exec = exec;
		tc->taskComplete = std::make_shared<std::atomic<bool>>();
		tc->callbacksComplete = std::make_shared<std::atomic<bool>>();
		tc->taskComplete->store(false);
		tc->callbacksComplete->store(false);
		tc->priority = priority;

		// TODO: What happens if two writers end up locking Task Map Write Lock
		{
			logThread("Trying to lock Task Map Write Lock.", 999999);

			// Insert executor and callback function to map
			std::unique_lock<std::shared_mutex> taskMapWriteLock(TaskMapMutex);

			logThread("Acquired Task Map Write Lock. Insert Task Context to Task Map.", 999999);
			idTaskMap[tid] = tc;

			logThread("Unlocking Task Map Write Lock.", 999999);
		}
		// TODO: What happens if two writers end up locking Task PQ Write Lock
		{
			logThread("Trying to lock Task PQ Write Lock.", 999999);
			std::unique_lock<std::shared_mutex> taskPQWriteLock(taskPQMutex);
			logThread("Acquired Task PQ Write Lock. Insert Task Context to Task PQ.", 999999);

			taskPQ.push(tc);
			logThread("Unlocking Task PQ Write Lock.", 999999);
		}
		logThread("End of task schedule", 999999);
		return tid;
	}

	void TaskSystemExecutorImpl::WaitForTask(TaskID task) {
		// Read Lock For TaskMap
		std::shared_lock<std::shared_mutex> mapLock(TaskMapMutex);

		// TODO: Remove many accesses to TaskMap
		std::shared_ptr<std::atomic<bool>> callbacksComplete = idTaskMap[task]->callbacksComplete;
		mapLock.unlock();

		std::unique_lock<std::mutex> waitLock(idTaskMap[task]->waitMutex);
		idTaskMap[task]->cv.wait(waitLock, [this, &callbacksComplete] {
			logThread("@@@Checking for condition" + callbacksComplete->load(), 999999);
			return callbacksComplete->load();
		});
	}

	void TaskSystemExecutorImpl::OnTaskCompleted(TaskID task, std::function<void(TaskID)>&& callback) {
		std::unique_lock<std::shared_mutex> TaskMapWriteLock(TaskMapMutex);

		// Save callback for later calls
		idTaskMap[task]->onCompleteCallbacks.push_back(callback);
	}

	void TaskSystemExecutorImpl::Register(const std::string& executorName, ExecutorConstructor constructor) {
		executorConstructors[executorName] = constructor;
	}

	void TaskSystemExecutorImpl::Terminate() {
		static std::mutex terminate_mutex;
		std::unique_lock<std::mutex> terminate_lock(terminate_mutex);

		logThread("Terminate has been called. Acquired terminate_lock. Setting terminateThread to true.", 999999);

		terminateThreads = true;

		for (std::thread& t : threads) {
			t.join();
		}
		logThread("All threads joined. Task System has been terminated.", 999999);
		self = nullptr;

	}
	void TaskSystemExecutorImpl::workerFun(int tid) {
		logThread((std::string)"Started", tid);
		while (1) {
			if (terminateThreads) {
				logThread("TerminateThreads has been set - exiting", tid);
				return;
			}
			logThread("Try acquire PQ Read Lock", tid);
			std::shared_lock<std::shared_mutex> pqReadLock(taskPQMutex);
			logThread("Successfully acquired", tid);
			if (taskPQ.empty()) {
				logThread("Task Priority Queue is Empty: Unlock PQ Read Lock and sleep.", tid);
				pqReadLock.unlock();
				//pqReadLock.release();

				std::this_thread::sleep_for(std::chrono::milliseconds(100));

				continue;
			}
			std::shared_ptr<TaskContext> context = taskPQ.top();
			Executor::ExecStatus exec_status = context->exec->ExecuteStep(tid, threadCount);

			if (exec_status == Executor::ExecStatus::ES_Stop) {
				logThread("Task has completed. Unlocking PQ Read Lock. Trying to lock PQ Write Lock.", tid);

				pqReadLock.unlock();
				//pqReadLock.release();

				std::unique_lock<std::shared_mutex> pqWriteLock(taskPQMutex);

				logThread("PQ Write Lock has been locked.", tid);

				if (taskPQ.empty() || taskPQ.top() != context) {
					logThread("Task Queue is empty or top has changed. Unlocking PQ Write Lock.", tid);

					// Task queue is empty or top has changed

					pqWriteLock.unlock();
					//pqWriteLock.release();
					continue;
				}

				assert(taskPQ.top() == context);
				logThread("Removing top from Task PQ", tid);

				taskPQ.pop();
				
				logThread("Unlocking task for Task PQ", tid);

				pqWriteLock.unlock();
				
				// Set task complete flags
				context->taskComplete->store(true);
				context->cv.notify_all();

				/*logThread("##############Remove from TASKMAP: Trying to lock Task Map Write Lock", tid);
				{
					std::unique_lock<std::shared_mutex> TaskMapWriteLock(TaskMapMutex);
					logThread("Acquired Task Map Write Lock", tid);

					idTaskMap.erase(context->id);

					logThread("Unlocking Task Map Write Lock", tid);
				};*/
			
				// context should be only shared pointer to cur context
				logThread("CONTEXT USE COUNT: " + std::to_string(context.use_count()), tid);
				//assert(context.use_count() == 1);
				//return;
				
				if (context->onCompleteCallbacks.size() != 0) {
					logThread("Scheduling callbacks", tid);

					std::unique_ptr<Task> cb_task = std::make_unique<CallbackTaskParams>(context);

					TaskSystemExecutor::TaskID callback_task_id = ScheduleTask(std::move(cb_task), context->priority + 1);
				}
				else {
					logThread("Nothing to schedule", tid);

					/// No callbacks to schedule
					context->callbacksComplete->store(true);
					context->cv.notify_all();
				}
				continue;
			}
			else if (exec_status == Executor::ExecStatus::ES_Continue) {
				logThread("Task has not completed. Unlocking PQ Read Lock and continuing", tid);

				pqReadLock.unlock();
				//pqReadLock.release();
				continue;
			}
		}
	}
};
