#pragma once

#include "Task.h"

#include <memory>
namespace TaskSystem {

struct Executor {
    enum ExecStatus {
        ES_Continue, ES_Stop
    };

    Executor(std::unique_ptr<Task> taskToExecute) : task(std::move(taskToExecute)) {}
    virtual ~Executor() {}

    virtual ExecStatus ExecuteStep(int threadIndex, int threadCount) = 0;
    std::unique_ptr<Task> task;
};

typedef Executor*(*ExecutorConstructor)(std::unique_ptr<Task> taskToExecute);
struct TaskSystemExecutor;
typedef void (*OnLibraryInitPtr)(TaskSystemExecutor &);

};

#define IMPLEMENT_ON_INIT() extern "C" void OnLibraryInit(TaskSystem::TaskSystemExecutor &ts)
