#include "TaskSystem.h"

#include <cassert>

using namespace TaskSystem;

struct PrinterParams : Task {
    int max;
    int sleep;

    PrinterParams(int max, int sleep): max(max), sleep(sleep) {}
    virtual std::optional<int> GetIntParam(const std::string &name) const {
        if (name == "max") {
            return max;
        } else if (name == "sleep") {
            return sleep;
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

int main(int argc, char *argv[]) {
    TaskSystemExecutor &ts = TaskSystemExecutor::GetInstance();

    // ts.LoadLibrary("libPrinterExecutor.dylib");
    // std::unique_ptr<Task> task = std::make_unique<PrinterParams>(100, 5);

    const bool libLoaded = ts.LoadLibrary("/Users/poseidon4o/workspace/task-system-build/ts_executors/libRaytracerExecutor.dylib");
    assert(libLoaded);
    std::unique_ptr<Task> task = std::make_unique<RaytracerParams>("Example");

    TaskSystemExecutor::TaskID id = ts.ScheduleTask(std::move(task));
    ts.WaitForTask(id);

    return 0;
}