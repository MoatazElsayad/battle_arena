
cmake_minimum_required(VERSION 3.16)
project(Gladiators VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Widgets REQUIRED)


include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/battleArena/include
)

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/DatabaseManager.cpp
    src/InputHandler.cpp
    src/LoginPage.cpp
    src/registration.cpp
)

set(HEADERS
    include/MainWindow.h
    include/DatabaseManager.h
    include/InputHandler.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS}
    ui/Registration.ui
    src/LoginPage.cpp)
target_link_libraries(${PROJECT_NAME} Qt6::Widgets)
