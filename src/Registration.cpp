#include "Registration.h"
#include "ui_Registration.h"
#include <QMessageBox>

Registration::Registration(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Registration)
{
    ui->setupUi(this);
}

Registration::~Registration()
{
    delete ui;
}

void Registration::on_RegisterButton_clicked()
{
    const QString email = ui->lineEdit_email->text().trimmed();
    const QString username = ui->lineEdit_username->text().trimmed();
    const QString password = ui->lineEdit_password->text();
    const QString passwordRepeat = ui->lineEdit_password_2->text();

    if (email.isEmpty() || username.isEmpty() || password.isEmpty() || passwordRepeat.isEmpty()) {
        QMessageBox::warning(this, "Registration", "Please fill in all fields.");
        return;
    }

    if (password != passwordRepeat) {
        QMessageBox::warning(this, "Registration", "Passwords do not match.");
        return;
    }

    accept();
}
