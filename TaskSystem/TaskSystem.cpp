#include "TaskSystem.h"

#include <dlfcn.h>

void TaskSystem::LoadLibrary(const std::string &path) {
    void *handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (handle) {
        OnLibraryInitPtr initLib = (OnLibraryInitPtr)dlsym(handle, "OnLibraryInit");
        if (initLib) {
            initLib(*this);
        }
    }
}