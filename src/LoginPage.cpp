#include "LoginPage.h"
#include "ui_LoginPage.h"
#include "Registration.h"
#include <QMessageBox>

LoginPage::LoginPage(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::LoginPage)
{
    ui->setupUi(this);
    this->setStyleSheet("background-color: #4E8D9C;");
}

LoginPage::~LoginPage()
{
    delete ui;
}

void LoginPage::on_LoginButton_clicked()
{
    const QString username = ui->lineEdit_username->text().trimmed();
    const QString password = ui->lineEdit_password->text();

    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Login", "Please enter username and password.");
        return;
    }

    emit loginSucceeded(username);
}

void LoginPage::on_SignUpButton_clicked()
{
    Registration registration(this);
    registration.exec();
}
