#ifndef EXHIBITIONSETUPPAGE_H
#define EXHIBITIONSETUPPAGE_H

#include <QVector>
#include <QWidget>

#include "Enums.h"
#include "ProfileLobbyWidget.h"

class QLabel;
class QListWidget;
class QPushButton;

class ExhibitionSetupPage : public QWidget {
    Q_OBJECT

public:
    explicit ExhibitionSetupPage(QWidget* parent = nullptr);

    void applySessionState(const QString& playerName,
                           const std::vector<PlayerType>& playerTypes,
                           PlayerType selectedType,
                           const ProfileLobbyWidget::DuelSetup& setup);
    void setPlayerName(const QString& playerName);
    void setAvailablePlayerTypes(const std::vector<PlayerType>& playerTypes);
    void setSelectedPlayerType(PlayerType type);
    PlayerType selectedPlayerType() const;

    void setDuelSetup(const ProfileLobbyWidget::DuelSetup& setup);
    ProfileLobbyWidget::DuelSetup duelSetup() const;

signals:
    void backRequested();
    void launchRequested();
    void selectionChanged();

private:
    void setupUi();
    void rebuildFighterEntries();
    void rebuildOpponentEntries();
    void rebuildArenaEntries();
    void refreshToggleStates();
    void refreshPreview();
    void ensureValidOpponentSelection();
    void ensureValidArenaSelection();

    QString playerName_;
    QVector<PlayerType> availablePlayerTypes_;
    PlayerType selectedPlayerType_;
    ProfileLobbyWidget::DuelSetup duelSetup_;
    bool syncingUi_;

    QLabel* playerPortraitLabel_;
    QLabel* playerNameLabel_;
    QLabel* opponentPortraitLabel_;
    QLabel* opponentNameLabel_;
    QLabel* summaryLabel_;
    QLabel* arenaPreviewLabel_;
    QLabel* arenaNameLabel_;
    QLabel* arenaDescriptionLabel_;

    QListWidget* fighterList_;
    QPushButton* randomModeButton_;
    QPushButton* manualModeButton_;
    QPushButton* playerCategoryButton_;
    QPushButton* enemyCategoryButton_;
    QListWidget* opponentList_;
    QListWidget* arenaList_;
    QPushButton* backButton_;
    QPushButton* launchButton_;
};

#endif // EXHIBITIONSETUPPAGE_H
