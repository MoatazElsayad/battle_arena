#ifndef GAMEPAGE_H
#define GAMEPAGE_H

#include <optional>
#include <QWidget>
#include <QString>
#include "ChronicleAiTypes.h"
#include "CombatHighlightTypes.h"

class BattleWidget;
class ChronicleAiAdvisor;
class GameManager;
class LanSessionManager;
class SoundManager;
class QLabel;

class GamePage : public QWidget {
    Q_OBJECT

public:
    // 1v1 teammate:
    // Reuse this page for duel mode.
    // The only change in duel should be flow control: one match, then finish.
    explicit GamePage(QWidget *parent = nullptr);
    ~GamePage();

    void setGameManager(GameManager *gm);
    void setLanSessionManager(LanSessionManager *manager);
    void setSoundManager(SoundManager *sm);
    void startBattle();
    void pauseBattle();
    void resumeBattle();
    bool hasActiveBattle() const;
    ChronicleBattleReport lastChronicleReport() const;
    std::optional<CombatHighlightSnapshot> lastBattleHighlight() const;

signals:
    void battleFinished();
    void chronicleRequested(int completedLevel, bool campaignComplete);
    void pauseRequested();

private slots:
    void onBattleFinished();
    void onLevelTransitionFinished();
    void updateStats();

private:
    void setupUI();

    GameManager *gameManager_;
    SoundManager *soundManager_;
    ChronicleAiAdvisor *chronicleAiAdvisor_;
    BattleWidget *battleWidget_;
    QLabel *playerInfoLabel_;
    QLabel *instructionsLabel_;
    int pendingChronicleLevel_;
    bool pendingChronicleCampaignComplete_;
    ChronicleBattleReport pendingChronicleReport_;
    std::optional<CombatHighlightSnapshot> pendingBattleHighlight_;
};

#endif // GAMEPAGE_H
