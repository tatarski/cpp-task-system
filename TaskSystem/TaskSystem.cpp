#include <dlfcn.h>

#include "TaskSystem.h"

#include <cassert>

namespace TaskSystem {

TaskSystemExecutor* TaskSystemExecutor::self = nullptr;

TaskSystemExecutor &TaskSystemExecutor::GetInstance() {
    return *self;
}


bool TaskSystemExecutor::LoadLibrary(const std::string &path) {
    void *handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    assert(handle);
    if (handle) {
        OnLibraryInitPtr initLib = (OnLibraryInitPtr)dlsym(handle, "OnLibraryInit");
        assert(initLib);
        if (initLib) {
            initLib(*this);
            printf("Initialized [%s] executor\n", path.c_str());
            return true;
        }
    }
    return false;
}

};
