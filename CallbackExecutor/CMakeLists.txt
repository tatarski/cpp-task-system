project(CallbackExecutor)

set (CMAKE_CXX_STANDARD 17)

set(SOURCES
    CallBackExecutor.cpp
)
set(HEADERS
    CallBackExecutor.h
)

add_library(${PROJECT_NAME} SHARED ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ../TaskSystem)

install(TARGETS ${PROJECT_NAME} DESTINATION ${PLUGIN_INSTALL_PATH})
