#ifndef MAINMENUPAGE_H
#define MAINMENUPAGE_H

#include <QWidget>

class MainMenuPage : public QWidget
{
    Q_OBJECT

public:
    explicit MainMenuPage(QWidget *parent = nullptr);
    ~MainMenuPage();

signals:
    void playClicked();
    void profileClicked();
    void quitClicked();

public slots:

private:
    class MainMenuPagePrivate *d;
};

#endif // MAINMENUPAGE_H