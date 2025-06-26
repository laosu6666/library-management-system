// include/login.h
#ifndef LOGIN_H
#define LOGIN_H

#include <QDialog>
#include "user.h"
#include "library.h"

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

    User* getAuthenticatedUser() const { return m_user; }

private slots:
    void onLoginClicked();
    void onRegisterClicked();

private:
    Ui::LoginDialog *ui;
    User *m_user;
    Library *m_library;
};

#endif // LOGIN_H
