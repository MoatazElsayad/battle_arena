#ifndef PAUSEPAGE_H
#define PAUSEPAGE_H

#include <QPointer>
#include <QWidget>

class QFrame;
class QLabel;

class PausePage : public QWidget {
    Q_OBJECT

public:
    explicit PausePage(QWidget *parent = nullptr);
    ~PausePage();
    void showPauseOverlay();

signals:
    void resumeClicked();
    void menuClicked();
    void settingsClicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setupUI();
    void syncToParent();

    QPointer<QWidget> hostWidget_;
    QFrame *panel_;
    QLabel *titleLabel_;
    QLabel *subtitleLabel_;
};

#endif // PAUSEPAGE_H
