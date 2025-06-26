// include/creditdialog.h
#ifndef CREDITDIALOG_H
#define CREDITDIALOG_H

#include <QDialog>
#include "user.h"
#include "library.h"

namespace Ui {
class CreditDialog;
}

class CreditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreditDialog(User *user, Library *library, QWidget *parent = nullptr);
    ~CreditDialog();

private slots:
    void onAddCreditClicked();
    void onPayWithCreditClicked();

private:
    Ui::CreditDialog *ui;
    User *m_user;
    Library *m_library;
};

#endif // CREDITDIALOG_H
