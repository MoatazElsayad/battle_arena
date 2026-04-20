#include "InputHandler.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <unordered_map>

namespace {
namespace fs = std::filesystem;

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string normalizeName(const std::string& value) {
    std::string normalized = toLower(value);
    std::replace(normalized.begin(), normalized.end(), '_', ' ');
    return normalized;
}

std::string characterAssetFolder(PlayerType type) {
    switch (type) {
        case PlayerType::ARCEN:
            return "Arcen";
        case PlayerType::DEMON_SLAYER:
            return "Demon_Slayer";
        case PlayerType::FANTASY_WARRIOR:
            return "Fantasy_Warrior";
        case PlayerType::HUNTRESS:
            return "Huntress";
        case PlayerType::KNIGHT:
            return "Knight";
        case PlayerType::MARTIAL:
            return "Martial";
        case PlayerType::MARTIAL_HERO:
            return "Martial_Hero";
        case PlayerType::MEDIEVAL_WARRIOR:
            return "Medieval_Warrior";
        case PlayerType::WIZARD:
            return "Wizard";
        default:
            return "";
    }
}

fs::path resolvePlayersRoot() {
    const std::array<fs::path, 6> candidates = {
        fs::path("assets") / "players",
        fs::path(".") / "assets" / "players",
        fs::path("..") / "assets" / "players",
        fs::path("..") / ".." / "assets" / "players",
        fs::path("..") / ".." / ".." / "assets" / "players",
        fs::path("..") / ".." / ".." / ".." / "assets" / "players"
    };

    for (const fs::path& candidate : candidates) {
        std::error_code ec;
        if (fs::exists(candidate, ec) && fs::is_directory(candidate, ec)) {
            return candidate;
        }
    }
    return fs::path();
}

bool isSpriteImageFile(const fs::path& filePath, const fs::path& characterRoot) {
    std::error_code ec;
    if (!fs::is_regular_file(filePath, ec)) {
        return false;
    }

    std::string ext = toLower(filePath.extension().string());
    if (ext != ".png" && ext != ".jpg" && ext != ".jpeg" && ext != ".webp") {
        return false;
    }

    fs::path rel = fs::relative(filePath, characterRoot, ec);
    if (ec) {
        return false;
    }

    // Only count image files in Sprite/Sprites folders as gameplay features.
    for (const fs::path& part : rel) {
        std::string folder = toLower(part.string());
        if (folder == "sprite" || folder == "sprites") {
            return true;
        }
    }
    return false;
}

CharacterFeatureSet featureSetFor(PlayerType type) {
    static std::unordered_map<int, CharacterFeatureSet> cache;
    int key = static_cast<int>(type);
    auto found = cache.find(key);
    if (found != cache.end()) {
        return found->second;
    }

    // Hardcode Arcen features since it uses a single Attack.png for all attacks
    if (type == PlayerType::ARCEN) {
        CharacterFeatureSet arcenFeatures{1, {"attack"}, 3, false, false};
        cache[key] = arcenFeatures;
        return arcenFeatures;
    }

    CharacterFeatureSet features{0, {}, 0, false, false};

    fs::path playersRoot = resolvePlayersRoot();
    std::string folderName = characterAssetFolder(type);
    if (playersRoot.empty() || folderName.empty()) {
        cache[key] = features;
        return features;
    }

    fs::path characterRoot = playersRoot / folderName;
    std::error_code ec;
    if (!fs::exists(characterRoot, ec) || !fs::is_directory(characterRoot, ec)) {
        cache[key] = features;
        return features;
    }

    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(characterRoot, ec)) {
        if (ec) {
            break;
        }
        fs::path filePath = entry.path();
        if (!isSpriteImageFile(filePath, characterRoot)) {
            continue;
        }

        features.featureImageCount += 1;
        std::string featureName = normalizeName(filePath.stem().string());
        features.featureNames.push_back(featureName);

        if (featureName.find("attack") != std::string::npos) {
            features.attackOptions += 1;
        }
        if (featureName.find("jump") != std::string::npos ||
            featureName.find("fall") != std::string::npos ||
            featureName.find("going up") != std::string::npos ||
            featureName.find("going down") != std::string::npos) {
            features.canJump = true;
        }
        if (featureName.find("heal") != std::string::npos) {
            features.canHeal = true;
        }
    }

    if (features.attackOptions > 3) {
        features.attackOptions = 3;
    }

    // Arcen uses the same Attack.png for all 3 attacks, so we need to ensure it reports 3 attacks
    if (type == PlayerType::ARCEN && features.attackOptions < 3) {
        features.attackOptions = 3;
    }

    cache[key] = features;
    return features;
}
}

InputHandler::InputHandler() {
}

void InputHandler::keyDown(int k) {
    keys.insert(k);
    justPressed.insert(k);
}

void InputHandler::keyUp(int k) {
    keys.erase(k);
    justPressed.erase(k);
}

bool InputHandler::isPressed(int k) const {
    return keys.count(k) > 0;
}

bool InputHandler::wasJustPressed(int k) const {
    return justPressed.count(k) > 0;
}

void InputHandler::update() {
    justPressed.clear();
}

void InputHandler::reset() {
    keys.clear();
    justPressed.clear();
}

CharacterFeatureSet InputHandler::getCharacterFeatures(PlayerType type) {
    return featureSetFor(type);
}

bool InputHandler::canPerformAction(PlayerType type, PlayerAction action) {
    CharacterFeatureSet features = featureSetFor(type);

    switch (action) {
        case PlayerAction::MOVE_LEFT:
        case PlayerAction::MOVE_RIGHT:
        case PlayerAction::PAUSE:
            return true;
        case PlayerAction::ATTACK1:
            return features.attackOptions >= 1;
        case PlayerAction::ATTACK2:
            return features.attackOptions >= 2;
        case PlayerAction::ATTACK3:
            return features.attackOptions >= 3;
        case PlayerAction::JUMP:
            return features.canJump;
        case PlayerAction::HEAL:
            return features.canHeal;
        default:
            return false;
    }
}

int InputHandler::countActionFeatures(PlayerType type) {
    return featureSetFor(type).featureScore();
}

std::vector<std::string> InputHandler::getCharacterFeatureNames(PlayerType type) {
    return featureSetFor(type).featureNames;
}

std::string InputHandler::playerTypeToDisplayName(PlayerType type) {
    switch (type) {
        case PlayerType::ARCEN:
            return "Arcen";
        case PlayerType::DEMON_SLAYER:
            return "Demon Slayer";
        case PlayerType::FANTASY_WARRIOR:
            return "Fantasy Warrior";
        case PlayerType::HUNTRESS:
            return "Huntress";
        case PlayerType::KNIGHT:
            return "Knight";
        case PlayerType::MARTIAL:
            return "Martial";
        case PlayerType::MARTIAL_HERO:
            return "Martial Hero";
        case PlayerType::MEDIEVAL_WARRIOR:
            return "Medieval Warrior";
        case PlayerType::WIZARD:
            return "Wizard";
        default:
            return "Unknown";
    }
}

std::vector<PlayerType> InputHandler::getCharactersSortedByFeatures() {
    std::vector<PlayerType> characters = {
        PlayerType::ARCEN,
        PlayerType::DEMON_SLAYER,
        PlayerType::FANTASY_WARRIOR,
        PlayerType::HUNTRESS,
        PlayerType::KNIGHT,
        PlayerType::MARTIAL,
        PlayerType::MARTIAL_HERO,
        PlayerType::MEDIEVAL_WARRIOR,
        PlayerType::WIZARD
    };

    std::stable_sort(characters.begin(), characters.end(), [](PlayerType a, PlayerType b) {
        int aScore = InputHandler::countActionFeatures(a);
        int bScore = InputHandler::countActionFeatures(b);
        if (aScore != bScore) {
            return aScore < bScore;
        }
        return InputHandler::playerTypeToDisplayName(a) < InputHandler::playerTypeToDisplayName(b);
    });

    return characters;
}
