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

    TaskSystem::TS_LOAD_LIBARY("PrinterExecutor", ts);

    std::unique_ptr<Task> task = std::make_unique<RaytracerParams>("Example");

    TaskSystemExecutor::TaskID id = ts.ScheduleTask(std::move(task), 1);
    ts.WaitForTask(id);
}

void testPrinter() {
    TaskSystemExecutor &ts = TaskSystemExecutor::GetInstance();
    TaskSystem::TS_LOAD_LIBARY("PrinterExecutor", ts);

    // two instances of the same task
    std::unique_ptr<Task> p1 = std::make_unique<PrinterParams>(10, 1000, 1);
    std::unique_ptr<Task> p2 = std::make_unique<PrinterParams>(10, 300, 2);
    std::unique_ptr<Task> p3 = std::make_unique<PrinterParams>(5, 2000, 3);

    // give some time for the first task to execute
    TaskSystemExecutor::TaskID id1 = ts.ScheduleTask(std::move(p1), 9);
    //std::this_thread::sleep_for(std::chrono::milliseconds(400));

    //// insert bigger priority task, TaskSystem should switch to it
    TaskSystemExecutor::TaskID id2 = ts.ScheduleTask(std::move(p2), 99);
    //std::this_thread::sleep_for(std::chrono::milliseconds(400));

    ts.OnTaskCompleted(id1, [](TaskSystemExecutor::TaskID id) {
        printf("Task 1 finished\n");
    });
    
    ts.OnTaskCompleted(id2, [](TaskSystemExecutor::TaskID id) {
        printf("Task 2 finished\n");
    });

    //std::this_thread::sleep_for(std::chrono::milliseconds(60000));
    //return;
    /*ts.WaitForTask(id3);*/

    ts.WaitForTask(id1);
    ts.WaitForTask(id2);

    TaskSystemExecutor::TaskID id3 = ts.ScheduleTask(std::move(p3), 999);
    ts.WaitForTask(id3);

    ts.Terminate();
}

int main(int argc, char *argv[]) {
    TaskSystemExecutorImpl::Init(4);

    testPrinter();

    return 0;
}