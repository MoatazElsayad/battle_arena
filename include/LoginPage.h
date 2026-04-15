#ifndef LOGINPAGE_H
#define LOGINPAGE_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class LoginPage;
}
QT_END_NAMESPACE

class LoginPage : public QMainWindow
{
    Q_OBJECT

public:
    LoginPage(QWidget *parent = nullptr);
    ~LoginPage();

private slots:
    void on_SignUpButton_clicked();

private:
    Ui::LoginPage *ui;
};
#endif // LOGINEPAGE_H
