#include "AnimationManager.h"
#include <QDebug>
#include <QDir>
#include <QFileInfo>

AnimationManager::AnimationManager() {
    m_animationNames << "idle" << "run" << "attack1" << "attack2" << "attack3" 
                     << "strong_attack" << "hurt" << "death";
}

void AnimationManager::loadAnimation(AnimationState state, int frameCount, 
                                      const QString& spriteSheetPath, bool loops, int frameSpeed) {
    QPixmap spriteSheet(spriteSheetPath);
    if (spriteSheet.isNull()) {
        qWarning() << "Failed to load sprite sheet:" << spriteSheetPath;
        return;
    }

    AnimationData data;
    data.frameSpeed = frameSpeed;
    data.loops = loops;

    int frameWidth = spriteSheet.width() / frameCount;
    int frameHeight = spriteSheet.height();

    for (int i = 0; i < frameCount; ++i) {
        QPixmap frame = spriteSheet.copy(i * frameWidth, 0, frameWidth, frameHeight);
        data.frames.append(frame);
    }

    m_animations[state] = data;
    qDebug() << "Loaded animation:" << stateToString(state) << "with" << frameCount << "frames";
}

void AnimationManager::loadAnimationFromDirectory(AnimationState state, const QString& directoryPath, 
                                                   const QString& filePattern, bool loops, int frameSpeed) {
    QDir dir(directoryPath);
    if (!dir.exists()) {
        qWarning() << "Animation directory not found:" << directoryPath;
        return;
    }

    QStringList files = dir.entryList(QStringList() << filePattern, QDir::Files);
    files.sort();

    if (files.isEmpty()) {
        qWarning() << "No animation frames found in:" << directoryPath;
        return;
    }

    AnimationData data;
    data.frameSpeed = frameSpeed;
    data.loops = loops;

    for (const QString& filename : files) {
        QPixmap frame(dir.filePath(filename));
        if (!frame.isNull()) {
            data.frames.append(frame);
        } else {
            qWarning() << "Failed to load animation frame:" << filename;
        }
    }

    if (!data.frames.isEmpty()) {
        m_animations[state] = data;
        qDebug() << "Loaded animation from directory:" << stateToString(state) 
                 << "with" << data.frames.size() << "frames";
    }
}

QPixmap AnimationManager::getFrame(AnimationState state, int index) const {
    auto it = m_animations.find(state);
    if (it == m_animations.end()) {
        return QPixmap();
    }
    const AnimationData& data = it.value();
    if (index >= 0 && index < data.frames.size()) {
        return data.frames[index];
    }
    return QPixmap();
}

int AnimationManager::getFrameCount(AnimationState state) const {
    auto it = m_animations.find(state);
    if (it == m_animations.end()) {
        return 0;
    }
    return it.value().frames.size();
}

bool AnimationManager::isLooping(AnimationState state) const {
    auto it = m_animations.find(state);
    if (it == m_animations.end()) {
        return false;
    }
    return it.value().loops;
}

int AnimationManager::getFrameSpeed(AnimationState state) const {
    auto it = m_animations.find(state);
    if (it == m_animations.end()) {
        return 100;
    }
    return it.value().frameSpeed;
}

bool AnimationManager::hasAnimation(AnimationState state) const {
    return m_animations.contains(state) && m_animations[state].frames.size() > 0;
}

QString AnimationManager::stateToString(AnimationState state) const {
    int index = static_cast<int>(state);
    if (index >= 0 && index < m_animationNames.size()) {
        return m_animationNames[index];
    }
    return QString();
}

AnimationState AnimationManager::stringToState(const QString& str) const {
    int index = m_animationNames.indexOf(str);
    if (index >= 0) {
        return static_cast<AnimationState>(index);
    }
    return AnimationState::IDLE;
}
