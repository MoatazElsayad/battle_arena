#ifndef LANSESSIONMANAGER_H
#define LANSESSIONMANAGER_H

#include <QAbstractSocket>
#include <QObject>
#include <QStringList>

#include "NetTypes.h"

class QTcpServer;
class QTcpSocket;
class QTimer;

class LanSessionManager : public QObject {
    Q_OBJECT

public:
    explicit LanSessionManager(QObject* parent = nullptr);
    ~LanSessionManager() override;

    const LanSessionSnapshot& snapshot() const { return snapshot_; }
    const LanCombatInputFrame& latestRemoteCombatInput() const { return latestRemoteCombatInput_; }
    const LanCombatState& latestCombatState() const { return latestCombatState_; }
    bool combatBridgeActive() const { return combatBridgeActive_; }

    void setLocalProfile(const QString& username, const QString& fighterName, int fighterType);
    void setArenaName(const QString& arenaName);

    bool startHosting(const QString& username, const QString& fighterName, int fighterType, quint16 port = kDefaultLanPort);
    bool joinSession(const QString& hostAddress, const QString& username, const QString& fighterName, int fighterType, quint16 port = kDefaultLanPort);

    void setLocalReady(bool ready);
    void primeMatch();
    void disconnectSession();
    void beginCombatBridge();
    void endCombatBridge();
    void sendCombatInput(quint8 inputBits);
    void publishCombatState(const LanCombatState& state);

    QStringList localAddressHints() const;

signals:
    void snapshotChanged(const LanSessionSnapshot& snapshot);
    void eventLogged(const QString& message);
    void errorRaised(const QString& message);
    void matchPrimed(const LanSessionSnapshot& snapshot);

private slots:
    void handleNewConnection();
    void handleSocketConnected();
    void handleSocketReadyRead();
    void handleSocketDisconnected();
    void handleSocketError(QAbstractSocket::SocketError socketError);
    void sendPing();

private:
    void attachSocket(QTcpSocket* socket);
    void releaseSocket(bool abortConnection);
    void resetSession(LanRole role = LanRole::NONE, LanSessionState state = LanSessionState::IDLE);
    void emitSnapshot();
    void logEvent(const QString& message);
    void raiseError(const QString& message);
    void sendHello();
    void sendCharacterSelection();
    void sendReadyState();
    void sendStartMatch();
    void sendMessage(LanPacketType type, const QJsonObject& payload);
    void processIncomingLine(const QByteArray& line);
    void updateDerivedState();
    void updateStatusLine();
    bool hasActiveSocket() const;
    bool bothPlayersReady() const;
    bool bothPlayersNamed() const;
    QString bestLocalAddress() const;

    QTcpServer* server_;
    QTcpSocket* socket_;
    QTimer* pingTimer_;
    QByteArray readBuffer_;
    qint64 pendingPingToken_;
    qint64 pendingPingStartedMs_;
    quint32 outgoingCombatSequence_;
    bool combatBridgeActive_;
    LanCombatInputFrame latestRemoteCombatInput_;
    LanCombatState latestCombatState_;
    LanSessionSnapshot snapshot_;
};

#endif // LANSESSIONMANAGER_H
