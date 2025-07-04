#ifndef BOOKDETAILDIALOG_H
#define BOOKDETAILDIALOG_H

#include <QDialog>
#include "book.h"
#include "ui_bookdetaildialog.h"
#include "library.h"
#include "user.h"

namespace Ui {
class BookDetailDialog;
}

class BookDetailDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BookDetailDialog(Book *book, Library *library, User *user, QWidget *parent = nullptr);
    ~BookDetailDialog();

private slots:
    void on_btnBorrow_clicked();
    void on_btnAddComment_clicked();

private:
    Ui::BookDetailDialog *ui;
    Book *m_book;
    Library *m_library;
    User *m_currentUser;
};

#endif // BOOKDETAILDIALOG_H
