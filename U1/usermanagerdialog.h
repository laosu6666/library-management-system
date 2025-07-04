#ifndef USERMANAGERDIALOG_H
#define USERMANAGERDIALOG_H

#include <QDialog>
#include "library.h"
#include "ui_usermanagerdialog.h"

namespace Ui {
class UserManagerDialog;
}

class UserManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UserManagerDialog(Library *library, QWidget *parent = nullptr);
    ~UserManagerDialog();
private slots:

    void on_btnDelete_clicked();

private:
    Ui::UserManagerDialog *ui;
    Library *m_library;
    void loadUsers();
};

#endif // USERMANAGERDIALOG_H
