#include "Task.h"
#include "Executor.h"
#include "TaskSystem.h"

#include <chrono>
#include <thread>


struct Printer : Executor {
    Printer(std::unique_ptr<Task> taskToExecute) : Executor(std::move(taskToExecute)) {
        max = task->GetIntParam("max").value();
        sleepMs = task->GetIntParam("sleep").value();
    }

    virtual ~Printer() {}

    virtual ExecStatus ExecuteStep(int threadIndex, int threadCount) {
        const int myValue = current.fetch_add(1);
        if (myValue >= max) {
            current.fetch_sub(1);
            return ExecStatus::ES_Stop;
        }

        printf("Printer [%d/%d]: %d\n", threadIndex, threadCount, myValue);
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
        return ExecStatus::ES_Continue;
    };

    std::atomic<int> current = 0;
    int max = 0;
    int sleepMs = 0;
};

Executor* ExecutorConstructorImpl(std::unique_ptr<Task> taskToExecute) {
    return new Printer(std::move(taskToExecute));
}

struct TaskSystem;

IMPLEMENT_ON_INIT() {
    ts.RegisterExecutor("printer", &ExecutorConstructorImpl);
}
