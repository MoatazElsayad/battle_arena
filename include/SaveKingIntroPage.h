#ifndef SAVEKINGINTROPAGE_H
#define SAVEKINGINTROPAGE_H

#include <QElapsedTimer>
#include <QPixmap>
#include <QTimer>
#include <QWidget>

#include "Enums.h"

class SaveKingIntroPage : public QWidget {
    Q_OBJECT

public:
    enum class SceneMode {
        Intro,
        RescueEnding
    };

    explicit SaveKingIntroPage(QWidget* parent = nullptr);

    void setSelectedGladiator(PlayerType type, const QString& displayName);
    void startScene();
    void startIntroScene();
    void startRescueEndingScene();
    void stopScene();
    SceneMode sceneMode() const;

signals:
    void sceneFinished();

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void advanceScene();

private:
    void ensureAssetsLoaded();
    void startScene(SceneMode mode);
    qreal sceneDuration() const;
    QString currentTitle(qreal seconds) const;
    QString currentCaption(qreal seconds) const;

    QTimer frameTimer_;
    QElapsedTimer sceneClock_;
    bool sceneActive_;
    bool finishEmitted_;
    SceneMode sceneMode_;

    PlayerType selectedGladiatorType_;
    QString selectedGladiatorName_;

    QPixmap background_;
    QPixmap rescueBackground_;
};

#endif // SAVEKINGINTROPAGE_H
