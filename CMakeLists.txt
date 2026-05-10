	
cmake_minimum_required(VERSION 3.16)
project(Gladiators VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/ui)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Widgets Multimedia Network REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/AnimatedCharacter.cpp
    src/AnimationManager.cpp
    src/BattleWidget.cpp
    src/Character.cpp
    src/ChronicleAiAdvisor.cpp
    src/ChronicleOpenRouterClient.cpp
    src/CombatHighlightTracker.cpp
    src/DatabaseManager.cpp
    src/Enemy.cpp
    src/ExhibitionSetupPage.cpp
    src/FighterAiBrain.cpp
    src/FighterAiProfile.cpp
    src/GameManager.cpp
    src/GamePage.cpp
    src/GameOverPage.cpp
    src/InputHandler.cpp
    src/LeaderboardPage.cpp
    src/LanArenaPage.cpp
    src/LanProtocol.cpp
    src/LanSessionManager.cpp
    src/LevelTransitionChroniclePage.cpp
    src/MainWindow.cpp
    src/PausePage.cpp
    src/Player.cpp
    src/ProfilePage.cpp
    src/ProfileLobbyWidget.cpp
    src/SaveKingIntroPage.cpp
    src/SettingsPage.cpp
    src/SoundManager.cpp
    src/WebsiteSyncClient.cpp

    include/AnimatedCharacter.h
    include/AnimationManager.h
    include/BattleWidget.h
    include/Character.h
    include/ChronicleAiAdvisor.h
    include/ChronicleAiTypes.h
    include/ChronicleOpenRouterClient.h
    include/CombatHighlightTracker.h
    include/CombatHighlightTypes.h
    include/DatabaseManager.h
    include/Enemy.h
    include/Enums.h
    include/ExhibitionSetupPage.h
    include/FighterAiBrain.h
    include/FighterAiProfile.h
    include/GameManager.h
    include/GamePage.h
    include/GameOverPage.h
    include/InputHandler.h
    include/LeaderboardPage.h
    include/LanArenaPage.h
    include/LanProtocol.h
    include/LanSessionManager.h
    include/LevelTransitionChroniclePage.h
    include/MainWindow.h
    include/NetTypes.h
    include/PausePage.h
    include/Player.h
    include/ProfilePage.h
    include/ProfileLobbyWidget.h
    include/SaveKingIntroPage.h
    include/SettingsPage.h
    include/SoundManager.h
    include/WebsiteSyncClient.h

    ui/GameOverPage.ui
    ui/MainMenuPage.ui
    ui/MainWindow.ui
    ui/PausePage.ui
    ui/ProfilePage.ui
    ui/WelcomePage.ui
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt6::Widgets
        Qt6::Multimedia
        Qt6::Network
)
