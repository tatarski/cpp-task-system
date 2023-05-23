#pragma once

#include "Task.h"
#include "Executor.h"

#include <map>
#include <functional>

namespace TaskSystem {

struct TaskSystemExecutor {
private:
    TaskSystemExecutor() = default;
    void Register(const std::string &executorName, ExecutorConstructor constructor) {
        executors[executorName] = constructor;
    }
public:
    TaskSystemExecutor(const TaskSystemExecutor &) = delete;
    TaskSystemExecutor &operator=(const TaskSystemExecutor &) = delete;

    static TaskSystemExecutor &GetInstance() {
        static TaskSystemExecutor ts;
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

    bool LoadLibrary(const std::string &path);

    std::map<std::string, ExecutorConstructor> executors;
};

};