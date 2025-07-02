#ifndef ADDBOOKDIALOG_H
#define ADDBOOKDIALOG_H

#include <QDialog>
#include "library.h"
#include "ui_addbookdialog.h"

namespace Ui {
class AddBookDialog;
}

class AddBookDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddBookDialog(Library *library, QWidget *parent = nullptr);
    ~AddBookDialog();

private slots:
    void on_btnAdd_clicked();

private:
    Ui::AddBookDialog *ui;
    Library *m_library;
};

#endif // ADDBOOKDIALOG_H
