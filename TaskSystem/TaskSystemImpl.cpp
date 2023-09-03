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

		// Create executor instance
		std::string execName = task->GetExecutorName();
		std::shared_ptr<Executor> exec(executorConstructors[execName](std::move(task)));
		TaskID tid = { idGen.getId() };

		// Create task context instance
		std::shared_ptr<TaskContext> tc = std::make_shared<TaskContext>();
		tc->exec = exec;
		tc->taskComplete = std::make_shared<std::atomic<bool>>();
		tc->callbacksComplete = std::make_shared<std::atomic<bool>>();
		tc->taskComplete->store(false);
		tc->callbacksComplete->store(false);
		tc->priority = priority;
		tc->id = tid;

		// Insert task context into task map
		{
			logThread("Trying to lock Task Map Write Lock.", 999999);
			std::unique_lock<std::shared_mutex> taskMapWriteLock(TaskMapMutex);
			logThread("Acquired Task Map Write Lock. Insert task context into task map.", 999999);
			
			idTaskMap[tid] = tc;

			logThread("Unlocking Task Map Write Lock.", 999999);
		}
		
		// Insert task context into task priority queue
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
		std::shared_ptr<TaskContext> cur_task;
		
		// Get desired task context
		{
			std::shared_lock<std::shared_mutex> taskMapReadLock(TaskMapMutex);
			if (!idTaskMap.count(task)) {
				throw std::invalid_argument("Trying to wait for task with invalid taskId");
			}
			cur_task = idTaskMap[task];
		}

		std::shared_ptr<std::atomic<bool>> callbacksComplete = cur_task->callbacksComplete;
		
		// Wait for callbacksComplete to be set
		{
			std::unique_lock<std::mutex> waitLock(idTaskMap[task]->waitMutex);

			cur_task->cv.wait(waitLock, [callbacksComplete] {
				logThread("@@@Checking for condition" + callbacksComplete->load(), 999999);
				return callbacksComplete->load();
				});
		}
	}

	void TaskSystemExecutorImpl::OnTaskCompleted(TaskID task, std::function<void(TaskID)>&& callback) {
		std::unique_lock<std::shared_mutex> TaskMapWriteLock(TaskMapMutex);

		// TODO: Remove Locking taskMap mutex
 
		// Save callback for later calls
		idTaskMap[task]->onCompleteCallbacks.push_back(callback);
	}

	void TaskSystemExecutorImpl::Register(const std::string& executorName, ExecutorConstructor constructor) {
		executorConstructors[executorName] = constructor;
	}

	void TaskSystemExecutorImpl::Terminate() {

		static std::mutex terminate_mutex;

		if (terminateThreads) return;
		
		std::unique_lock<std::mutex> terminate_lock(terminate_mutex);
		
		if (terminateThreads) return;

		logThread("Terminate has been called. Acquired terminate_lock. Setting terminateThread to true.", 999999);

		terminateThreads = true;

		// Threads should join eventually.
		for (std::thread& t : threads) {
			t.join();
		}
		logThread("All threads joined. Task System has been terminated.", 999999);

		// Delete current instance
		delete self;
		self = nullptr;

	}
	void TaskSystemExecutorImpl::workerFun(int tid) {
		logThread((std::string)"Started", tid);
		bool taskChanged = true;
		std::shared_ptr<TaskContext> context;

		while (1) {
			if (terminateThreads) {
				logThread("TerminateThreads has been set - exiting", tid);
				return;
			}

			Executor::ExecStatus exec_status;

			// Get task context for task with highest priority only if previous task has completed

			// TODO: signal in some way when a higher priority task is pushed
			// TODO: If higher priority task is not present - do not get new task from priority queue 
			{

				logThread("Try acquire PQ Read Lock", tid);
				std::shared_lock<std::shared_mutex> pqReadLock(taskPQMutex);
				logThread("Successfully acquired", tid);
				
				if (taskPQ.empty()) {
					// Should try loading task again next iteration
					logThread("Task Priority Queue is Empty: Unlock PQ Read Lock and sleep.", tid);
					
					// Unlock read lock and sleep for some time
					pqReadLock.unlock();
					//pqReadLock.release();

					// TODO: sleep time
					std::this_thread::sleep_for(std::chrono::milliseconds(100));

					continue;
				}

				// Acquiring new task with highest priority
				context = taskPQ.top();
			}

			// Execute step from current task context
			exec_status = context->exec->ExecuteStep(tid, threadCount);
			
			if (exec_status == Executor::ExecStatus::ES_Stop) {
				// Task has completed and should be removed from priority queue and task map
				
				// Remove task from task priority queue
				{
					logThread("Task has completed. Trying to lock PQ Write Lock.", tid);
					std::unique_lock<std::shared_mutex> pqWriteLock(taskPQMutex);
					logThread("PQ Write Lock has been locked.", tid);

					// Task queue has been emptied while waiting for Task PQ Write Lock or 
					// Taks queue top has changed while waiting for Task PQ Write Lock
					if (taskPQ.empty() || taskPQ.top().get() != context.get()) {
						logThread("Task Queue is empty or top has changed. Unlocking PQ Write Lock.", tid);

						continue;
					}

					assert(taskPQ.top() == context);
					logThread("Removing top from Task PQ", tid);

					taskPQ.pop();

					logThread("Unlocking Task PQ lock", tid);

					context->taskComplete->store(true);
				}
				
				// Task has completed -> only one worker thread can enter here only once per task.

				// TODO: Erasing tasks from taskmap causes 
				/*logThread("##############Remove from TASKMAP: Trying to lock Task Map Write Lock", tid);
				{
					std::unique_lock<std::shared_mutex> TaskMapWriteLock(TaskMapMutex);
					logThread("Acquired Task Map Write Lock", tid);

					idTaskMap.erase(context->id);

					logThread("Unlocking Task Map Write Lock", tid);
				};*/

				
				if (context->onCompleteCallbacks.size() != 0) {
					// Schedule callbacks task. Callbacks task should set callbacksComplete on finished task once finished.

					logThread("Scheduling callbacks", tid);

					std::unique_ptr<Task> cb_task = std::make_unique<CallbackTaskParams>(context);

					TaskSystemExecutor::TaskID callback_task_id = ScheduleTask(std::move(cb_task), context->priority + 1);
				}
				else {
					logThread("Nothing to schedule", tid);

					/// No callbacks to schedule. Set callbacksComplate to true.
					{
						std::lock_guard<std::mutex> callbackWaitLock(context->waitMutex);

						context->callbacksComplete->store(true);
					}
				}
				continue;
			}
			else if (exec_status == Executor::ExecStatus::ES_Continue) {
				// Top task has not completed - continue
				logThread("Task has not completed. Continuing", tid);

				continue;
			}
		}
	}
};
