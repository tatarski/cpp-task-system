project(TaskSystem)

set (CMAKE_CXX_STANDARD 17)

set(SOURCES
    TaskSystemImpl.cpp
    TaskSystem.cpp
    main.cpp
)

set(HEADERS
    Task.h
    Executor.h
    TaskSystem.h
    IdGenerator.h
    TaskSystemImpl.h
)

add_executable(${PROJECT_NAME} "${SOURCES};${HEADERS}")

target_compile_definitions(${PROJECT_NAME} PRIVATE TS_EXECUTOR_PATH="${PLUGIN_INSTALL_PATH}")

install(TARGETS ${PROJECT_NAME} DESTINATION ${PLUGIN_INSTALL_PATH})
