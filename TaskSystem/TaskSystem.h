#pragma once

#include "Task.h"
#include "Executor.h"

#include <map>
#include <functional>

struct TaskSystem {
private:
    TaskSystem() = default;
    void Register(const std::string &executorName, ExecutorConstructor constructor) {
        executors[executorName] = constructor;
    }
public:
    TaskSystem(const TaskSystem &) = delete;
    TaskSystem &operator=(const TaskSystem &) = delete;

    static TaskSystem &GetInstance() {
        static TaskSystem ts;
        return ts;
    }

    struct TaskID {};
    TaskID ScheduleTask(std::unique_ptr<Task> task) {
        std::unique_ptr<Executor> exec(executors[task->GetExecutorName()](std::move(task)));

        while (exec->ExecuteStep(0, 1) != Executor::ExecStatus::ES_Stop)
            ;

        return TaskID{};
    }

    void WaitForTask(TaskID task) {
        return;
    }

    void OnTaskCompleted(TaskID task, std::function<void(TaskID)> &&callback) {
        callback(task);
    }

    static void RegisterExecutor(const std::string &executorName, ExecutorConstructor constructor) {
        GetInstance().Register(executorName, constructor);
    }

    void LoadLibrary(const std::string &path);

    std::map<std::string, ExecutorConstructor> executors;
};