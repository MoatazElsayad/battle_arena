#include "AnimatedCharacter.h"
#include "AnimationManager.h"
#include <QDebug>

AnimatedCharacter::AnimatedCharacter(QObject *parent)
    : QObject(parent)
    , m_animManager(nullptr)
    , m_currentState(AnimationState::IDLE)
    , m_queuedState(AnimationState::IDLE)
    , m_frameIndex(0)
    , m_isDead(false)
    , m_isTransitioning(false)
    , m_animationTimer(new QTimer(this))
    , m_facingLeft(false)
{
    connect(m_animationTimer, &QTimer::timeout, this, &AnimatedCharacter::advanceFrame);
}

AnimatedCharacter::~AnimatedCharacter() {
}

void AnimatedCharacter::setAnimationManager(AnimationManager* manager) {
    m_animManager = manager;
    if (m_animManager) {
        setAnimationState(AnimationState::IDLE);
        startAnimationTimer();
    }
}

void AnimatedCharacter::setAnimationState(AnimationState state) {
    // Dead characters can only play death animation
    if (m_isDead && state != AnimationState::DEATH) {
        return;
    }
    
    // Don't change state if already in transition, queue instead
    if (m_isTransitioning && state != AnimationState::HURT && state != AnimationState::DEATH) {
        m_queuedState = state;
        return;
    }
    
    if (m_currentState != state) {
        m_currentState = state;
        m_frameIndex = 0;
        m_isTransitioning = m_animManager ? !m_animManager->isLooping(state) : false;
        
        if (m_animManager) {
            int frameSpeed = m_animManager->getFrameSpeed(state);
            m_animationTimer->stop();
            m_animationTimer->start(frameSpeed);
        }
    }
}

QPixmap AnimatedCharacter::getCurrentFrame() const {
    if (!m_animManager) {
        return QPixmap();
    }
    return m_animManager->getFrame(m_currentState, m_frameIndex);
}

void AnimatedCharacter::updateAnimation() {
    // Called externally if needed for synchronization
    if (!m_animManager) return;
}

void AnimatedCharacter::advanceFrame() {
    if (!m_animManager) return;
    
    int frameCount = m_animManager->getFrameCount(m_currentState);
    if (frameCount == 0) {
        // Animation doesn't exist, fall back to idle
        if (m_currentState != AnimationState::IDLE) {
            setAnimationState(AnimationState::IDLE);
        }
        return;
    }
    
    m_frameIndex++;
    
    if (m_frameIndex >= frameCount) {
        bool loops = m_animManager->isLooping(m_currentState);
        
        if (loops) {
            m_frameIndex = 0;
        } else {
            m_frameIndex = frameCount - 1;
            m_isTransitioning = false;
            
            // Handle end-of-animation logic
            if (m_currentState == AnimationState::DEATH) {
                m_isDead = true;
            } else if (m_currentState == AnimationState::HURT) {
                // Return to idle after hurt animation
                if (m_queuedState != AnimationState::IDLE) {
                    setAnimationState(m_queuedState);
                    m_queuedState = AnimationState::IDLE;
                } else {
                    setAnimationState(AnimationState::IDLE);
                }
                return;
            } else if (m_currentState != AnimationState::IDLE) {
                // Other non-looping animations return to idle
                setAnimationState(AnimationState::IDLE);
                return;
            }
        }
    }
}

void AnimatedCharacter::startAnimationTimer() {
    if (!m_animManager) return;
    
    int frameSpeed = m_animManager->getFrameSpeed(m_currentState);
    m_animationTimer->start(frameSpeed);
}

void AnimatedCharacter::tryChangeState(AnimationState newState) {
    if (m_isDead && newState != AnimationState::DEATH) {
        return;
    }
    
    bool canInterrupt = true;
    
    // Can't interrupt certain animations
    if (m_currentState == AnimationState::DEATH) {
        canInterrupt = false;
    } else if (m_currentState == AnimationState::HURT || 
               m_currentState == AnimationState::ATTACK1 || 
               m_currentState == AnimationState::ATTACK2 || 
               m_currentState == AnimationState::ATTACK3 || 
               m_currentState == AnimationState::STRONG_ATTACK) {
        canInterrupt = false;
    }
    
    // Damage and death can always interrupt
    if (canInterrupt || newState == AnimationState::HURT || newState == AnimationState::DEATH) {
        setAnimationState(newState);
    } else {
        // Queue the state change for later
        m_queuedState = newState;
    }
}

void AnimatedCharacter::kill() {
    m_isDead = true;
    setAnimationState(AnimationState::DEATH);
}

void AnimatedCharacter::takeDamage() {
    if (!m_isDead) {
        tryChangeState(AnimationState::HURT);
    }
}

void AnimatedCharacter::reset() {
    m_frameIndex = 0;
    m_isDead = false;
    m_isTransitioning = false;
    m_queuedState = AnimationState::IDLE;
    m_currentState = AnimationState::IDLE;
    if (m_animManager) {
        startAnimationTimer();
    }
}
