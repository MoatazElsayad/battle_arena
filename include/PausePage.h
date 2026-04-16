#ifndef PAUSEPAGE_H
#define PAUSEPAGE_H

#include <QWidget>

class PausePage : public QWidget {
    Q_OBJECT

public:
    explicit PausePage(QWidget *parent = nullptr);
    ~PausePage();

signals:
    void resumeClicked();
    void menuClicked();
    void settingsClicked();

private:
    void setupUI();
};

#endif // PAUSEPAGE_H
