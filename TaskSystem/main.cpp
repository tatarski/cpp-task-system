#include "TaskSystem.h"

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

int main(int argc, char *argv[]) {
    TaskSystem &ts = TaskSystem::GetInstance();
    ts.LoadLibrary("libPrinterExecutor.dylib");

    std::unique_ptr<Task> task = std::make_unique<PrinterParams>(100, 5);

    TaskSystem::TaskID id = ts.ScheduleTask(std::move(task));
    ts.WaitForTask(id);

    return 0;
}