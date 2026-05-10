#ifndef LANARENAPAGE_H
#define LANARENAPAGE_H

#include <QVector>
#include <QWidget>

#include "Enums.h"
#include "NetTypes.h"

class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class LanSessionManager;

class LanArenaPage : public QWidget {
    Q_OBJECT

public:
    explicit LanArenaPage(QWidget* parent = nullptr);

    void setSessionManager(LanSessionManager* manager);
    void setIdentity(const QString& username, const QString& fighterName, PlayerType fighterType);

signals:
    void backRequested();
    void localFighterChanged(PlayerType fighterType, const QString& fighterName);

private slots:
    void hostSession();
    void joinSession();
    void disconnectSession();
    void readyToggled(bool checked);
    void primeMatch();
    void applySnapshot(const LanSessionSnapshot& snapshot);
    void appendLog(const QString& message);

private:
    void setupUi();
    void rebuildFighterEntries();
    void rebuildArenaEntries();
    void syncFighterSelection();
    void syncArenaSelection();
    void refreshAddressHints();
    void refreshArenaControlState();
    quint16 selectedPort() const;

    LanSessionManager* sessionManager_;
    LanSessionSnapshot currentSnapshot_;
    QVector<PlayerType> availablePlayerTypes_;
    bool syncingUi_;
    QString username_;
    QString fighterName_;
    PlayerType fighterType_;
    QString selectedArena_;

    QLabel* addressHintLabel_;
    QLabel* localNameLabel_;
    QLabel* localFighterLabel_;
    QLabel* localPortraitLabel_;
    QLabel* localConnectionLabel_;
    QLabel* localReadyLabel_;
    QLabel* remoteNameLabel_;
    QLabel* remoteFighterLabel_;
    QLabel* remotePortraitLabel_;
    QLabel* remoteConnectionLabel_;
    QLabel* remoteReadyLabel_;
    QLabel* linkStatusBadgeLabel_;
    QLabel* linkStatusTextLabel_;
    QLabel* linkStatusMetaLabel_;
    QLabel* duelHintLabel_;
    QLabel* arenaNameLabel_;
    QLabel* arenaDescriptionLabel_;
    QLabel* arenaPreviewLabel_;
    QLabel* arenaOwnerHintLabel_;
    QLineEdit* hostAddressEdit_;
    QLineEdit* portEdit_;
    QListWidget* fighterList_;
    QListWidget* arenaList_;
    QPushButton* hostButton_;
    QPushButton* joinButton_;
    QPushButton* disconnectButton_;
    QPushButton* readyButton_;
    QPushButton* primeButton_;
};

#endif // LANARENAPAGE_H
