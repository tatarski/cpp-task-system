#pragma once

#include "Task.h"

#include <memory>

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

struct TaskSystem;
typedef void (*OnLibraryInitPtr)(TaskSystem &);

#define IMPLEMENT_ON_INIT() extern "C" void OnLibraryInit(TaskSystem &ts)
