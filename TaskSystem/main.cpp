#include "TaskSystem.h"
#include "TaskSystemImpl.h"
#include <cassert>
#include <chrono>
#include <thread>


using namespace TaskSystem;

struct PrinterParams : Task {
    int max;
    int sleep;
    int taskId;

    PrinterParams(int max, int sleep, int taskId): max(max), sleep(sleep), taskId(taskId) {}
    virtual std::optional<int> GetIntParam(const std::string &name) const {
        if (name == "max") {
            return max;
        } else if (name == "sleep") {
            return sleep;
        }
        else if (name == "taskId") {
            return taskId;
        }
        return std::nullopt;
    }
    virtual std::string GetExecutorName() const { return "printer"; }
};


struct RaytracerParams : Task {
    std::string sceneName;

    RaytracerParams(const std::string &sceneName): sceneName(sceneName) {}
    virtual std::optional<std::string> GetStringParam(const std::string &name) const {
        if (name == "sceneName") {
            return sceneName;
        }
        return std::nullopt;
    }
    virtual std::string GetExecutorName() const { return "raytracer"; }
};

void testRenderer() {
    TaskSystemExecutor &ts = TaskSystemExecutor::GetInstance();

    TaskSystem::TS_LOAD_LIBARY("RaytracerExecutor", ts);

    std::unique_ptr<Task> task = std::make_unique<RaytracerParams>("Example");

    TaskSystemExecutor::TaskID id = ts.ScheduleTask(std::move(task), 1);
    ts.OnTaskCompleted(id, [](TaskSystemExecutor::TaskID id) {
        printf("Task finished:%d\n", id.id);
        });
    ts.WaitForTask(id);
    //ts.Terminate();
}

void testPrinter() {
    TaskSystemExecutor &ts = TaskSystemExecutor::GetInstance();
    TaskSystem::TS_LOAD_LIBARY("PrinterExecutor", ts);

    std::unique_ptr<Task> p1 = std::make_unique<PrinterParams>(300, 1, 1);
    std::unique_ptr<Task> p2 = std::make_unique<PrinterParams>(20, 500, 2);

    // Schedule low priority task
    TaskSystemExecutor::TaskID id1 = ts.ScheduleTask(std::move(p1), 10);

    // Wait some time and schedule higher priority task - task system starts executing it.
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    TaskSystemExecutor::TaskID id2 = ts.ScheduleTask(std::move(p2), 20);

    // Register task 2 callbacks
    ts.OnTaskCompleted(id2, [](TaskSystemExecutor::TaskID id) {
        printf("Task 2 finished 1 id:%d\n", id.id);
    });
    ts.OnTaskCompleted(id2, [](TaskSystemExecutor::TaskID id) {
        printf("Task 2 finished 2 id: %d\n", id.id);
    });

    // Schedule task 3 after task 2 finishes.
    // TODO: no real way to wait for task3 once it has been scheduled in task2 callback
    // Possible to wait in worker?
    /*ts.OnTaskCompleted(id2, [&ts](TaskSystemExecutor::TaskID id) {
        printf("Task 2 Finish: Scheduling task 3 id: %d\n", id.id);
        std::unique_ptr<Task> p3 = std::make_unique<PrinterParams>(5, 3000, 3);
        ts.ScheduleTask(std::move(p3), 200);
    });*/
    
    ts.OnTaskCompleted(id1, [](TaskSystemExecutor::TaskID id) {
        printf("Task 1 finished id: %d\n", id.id);
    });


    ts.WaitForTask(id1);
    ts.WaitForTask(id2);

    //ts.Terminate();
}

int main(int argc, char *argv[]) {
    // Init task system implementation
    TaskSystemExecutorImpl::Init(1);

    testPrinter();

    //testRenderer();

    TaskSystemExecutor::GetInstance().Terminate();
    return 0;
}