cmake_minimum_required(VERSION 2.8.11)

# avoid some cmake warnings
IF(POLICY CMP0026)
    CMAKE_POLICY(SET CMP0026 OLD)
ENDIF()

project(MyChat)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# Qt5
find_package(Qt5Core REQUIRED)
include_directories(${Qt5Core_INCLUDE_DIRS})
find_package(Qt5Widgets REQUIRED)
include_directories(${Qt5Widgets_INCLUDE_DIRS})

add_compile_options(--std=c++14 -fPIC -g)

#pthread
find_package(Threads)

#json11
add_subdirectory(json11)
include_directories(json11)

set(SERVER_SRC_LIST server/server.cpp server/serverTest.cpp server/serverDatabase.cpp socket/TcpChatSocket.cpp)
set(CLIENT_SRC_LIST client/client.cpp client/clientTest.cpp socket/TcpChatSocket.cpp)
add_executable(Server ${SERVER_SRC_LIST})
target_link_libraries(Server json11 ${CMAKE_THREAD_LIBS_INIT})
add_executable(Client ${CLIENT_SRC_LIST})
target_link_libraries(Client json11 Qt5::Core Qt5::Widgets ${CMAKE_THREAD_LIBS_INIT})