#ifndef LOGINPAGE_H
#define LOGINPAGE_H

#include <QMainWindow>
#include <QString>

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

signals:
    void loginSucceeded(const QString &username);

private slots:
    void on_LoginButton_clicked();
    void on_SignUpButton_clicked();

private:
    Ui::LoginPage *ui;
};
#endif // LOGINPAGE_H
