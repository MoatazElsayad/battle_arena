#ifndef LANARENAPAGE_H
#define LANARENAPAGE_H

#include <QWidget>

#include "Enums.h"
#include "NetTypes.h"

class QLabel;
class QLineEdit;
class QPlainTextEdit;
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
    void refreshIdentityLabels();
    void refreshAddressHints();
    quint16 selectedPort() const;
    QString buildPlayerSummary(const QString& title, const LanPlayerInfo& player, bool connected) const;
    QString badgeStyleForState(LanSessionState state) const;

    LanSessionManager* sessionManager_;
    QString username_;
    QString fighterName_;
    PlayerType fighterType_;

    QLabel* identityLabel_;
    QLabel* addressHintLabel_;
    QLabel* statusBadgeLabel_;
    QLabel* statusTextLabel_;
    QLabel* sessionMetaLabel_;
    QLabel* localPlayerLabel_;
    QLabel* remotePlayerLabel_;
    QLabel* duelHintLabel_;
    QLineEdit* hostAddressEdit_;
    QLineEdit* portEdit_;
    QPushButton* hostButton_;
    QPushButton* joinButton_;
    QPushButton* disconnectButton_;
    QPushButton* readyButton_;
    QPushButton* primeButton_;
    QPlainTextEdit* eventLog_;
};

#endif // LANARENAPAGE_H
