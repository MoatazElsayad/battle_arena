
cmake_minimum_required(VERSION 3.16)
project(Gladiators VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/ui)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Widgets REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/AnimatedCharacter.cpp
    src/AnimationManager.cpp
    src/BattleWidget.cpp
    src/Character.cpp
    src/DatabaseManager.cpp
    src/Enemy.cpp
    src/GameManager.cpp
    src/GamePage.cpp
    src/GameOverPage.cpp
    src/InputHandler.cpp
    src/LeaderboardPage.cpp
    src/LoginPage.cpp
    src/MainWindow.cpp
    src/PausePage.cpp
    src/Player.cpp
    src/ProfilePage.cpp
    src/ProfileLobbyWidget.cpp
    src/Registration.cpp
    src/SettingsPage.cpp

    include/AnimatedCharacter.h
    include/AnimationManager.h
    include/BattleWidget.h
    include/Character.h
    include/DatabaseManager.h
    include/Enemy.h
    include/Enums.h
    include/GameManager.h
    include/GamePage.h
    include/GameOverPage.h
    include/InputHandler.h
    include/LeaderboardPage.h
    include/LoginPage.h
    include/MainWindow.h
    include/PausePage.h
    include/Player.h
    include/ProfilePage.h
    include/ProfileLobbyWidget.h
    include/Registration.h
    include/SettingsPage.h

    ui/GameOverPage.ui
    ui/LoginPage.ui
    ui/MainMenuPage.ui
    ui/MainWindow.ui
    ui/PausePage.ui
    ui/ProfilePage.ui
    ui/Registration.ui
    ui/WelcomePage.ui
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt6::Widgets
)
