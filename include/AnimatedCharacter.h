#ifndef ANIMATEDCHARACTER_H
#define ANIMATEDCHARACTER_H

#include <QPixmap>
#include <QTimer>
#include <QObject>
#include "Enums.h"

class AnimationManager;

class AnimatedCharacter : public QObject {
    Q_OBJECT

public:
    explicit AnimatedCharacter(QObject *parent = nullptr);
    ~AnimatedCharacter();

    void setAnimationManager(AnimationManager* manager);
    void setAnimationState(AnimationState state);
    AnimationState getCurrentState() const { return m_currentState; }
    
    QPixmap getCurrentFrame() const;
    int getCurrentFrameIndex() const { return m_frameIndex; }
    
    bool isDead() const { return m_isDead; }
    void kill();
    void takeDamage();
    void reset();

public slots:
    void updateAnimation();

protected slots:
    void advanceFrame();

private:
    void startAnimationTimer();
    void tryChangeState(AnimationState newState);

    AnimationManager* m_animManager;
    AnimationState m_currentState;
    AnimationState m_queuedState;
    int m_frameIndex;
    bool m_isDead;
    bool m_isTransitioning;
    QTimer* m_animationTimer;
};

#endif // ANIMATEDCHARACTER_H
