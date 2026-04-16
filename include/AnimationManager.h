#ifndef ANIMATIONMANAGER_H
#define ANIMATIONMANAGER_H

#include <QMap>
#include <QString>
#include <QVector>
#include <QPixmap>
#include <QStringList>
#include "Enums.h"

class AnimationManager {
public:
    struct AnimationData {
        QVector<QPixmap> frames;
        int frameSpeed;            // milliseconds per frame
        bool loops;
    };

    AnimationManager();

    void loadAnimation(AnimationState state, int frameCount, const QString& spriteSheetPath, 
                      bool loops = true, int frameSpeed = 100);
    void loadAnimationFromDirectory(AnimationState state, const QString& directoryPath, 
                                    const QString& filePattern = "*.png", 
                                    bool loops = true, int frameSpeed = 100);
    
    QPixmap getFrame(AnimationState state, int index) const;
    int getFrameCount(AnimationState state) const;
    bool isLooping(AnimationState state) const;
    int getFrameSpeed(AnimationState state) const;
    bool hasAnimation(AnimationState state) const;
    
    QString stateToString(AnimationState state) const;
    AnimationState stringToState(const QString& str) const;

private:
    QMap<AnimationState, AnimationData> m_animations;
    QStringList m_animationNames;
    static constexpr int SPRITE_WIDTH = 48;
    static constexpr int SPRITE_HEIGHT = 64;
};

#endif // ANIMATIONMANAGER_H
