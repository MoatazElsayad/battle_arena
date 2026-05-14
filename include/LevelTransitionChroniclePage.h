#ifndef LEVELTRANSITIONCHRONICLEPAGE_H
#define LEVELTRANSITIONCHRONICLEPAGE_H

#include <QElapsedTimer>
#include <QHash>
#include <QPixmap>
#include <QPushButton>
#include <QRect>
#include <QRectF>
#include <QString>
#include <QTimer>
#include <QVector>
#include <QWidget>

#include "ChronicleAiTypes.h"
#include "Enums.h"

class ChronicleAiAdvisor;
class QWheelEvent;

class LevelTransitionChroniclePage : public QWidget {
    Q_OBJECT

public:
    explicit LevelTransitionChroniclePage(QWidget *parent = nullptr);

    void configure(int completedLevel, int totalLevels, PlayerType playerType, const QString &playerName,
                   const ChronicleBattleReport& report = ChronicleBattleReport());
    void startChronicle();

signals:
    void continueRequested();
    void replayRequested(int level);

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private slots:
    void advanceAnimation();
    void handleContinueClicked();
    void handleAiSummaryReady(const ChronicleSummary& summary);

private:
    struct RealmInfo {
        int level;
        QString name;
        QString blurb;
        QString backgroundPath;
        int difficulty;
    };

    void setupButtons();
    void loadAssets();
    QVector<RealmInfo> realms() const;
    QPixmap loadPixmap(const QString &relativePath) const;
    QPixmap playerPreviewPixmap() const;
    QRectF aiSummaryPanelRect() const;
    void drawAiSummaryPanel(QPainter &painter, qreal seconds);
    bool isZombieChronicle() const;

    QTimer frameTimer_;
    QElapsedTimer sceneClock_;
    QPushButton *continueButton_;
    QPushButton *replayButton_;
    ChronicleAiAdvisor *aiAdvisor_;
    mutable QHash<QString, QPixmap> backgroundCache_;
    mutable QHash<QString, QRect> opaqueBoundsCache_;
    mutable QPixmap spriteSheetCache_;
    mutable PlayerType cachedPlayerType_;

    int completedLevel_;
    int totalLevels_;
    PlayerType playerType_;
    QString playerName_;
    ChronicleBattleReport battleReport_;
    ChronicleSummary aiSummary_;
    qreal aiSummaryScrollOffset_;
    bool aiSummaryLoading_;
    bool continueEmitted_;
};

#endif // LEVELTRANSITIONCHRONICLEPAGE_H
