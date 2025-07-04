// include/mainwindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "library.h"
#include "user.h"
#include "creditdialog.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(Library *library, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onLogin();
    void onSearchBooks();
    void onBorrowBook();
    void onReturnBook();
    void onRenewBook();

    void onPayFines();
    void onAddBook();
    void onRemoveBook();
    void onManageUsers();
    void onViewTopBooks();
    void onManageCredit();
    void onReserveBook();
    void onBookDetails();
    void logout();
    void on_btnRemoveBook_clicked();





private:
    Ui::MainWindow *ui;
    Library *m_library;
    User *m_currentUser;


    QList<Book*> m_currentSearchResults; // 存储当前搜索结果



    void updateUI();
    void showLoginDialog();
    void showBookDetails(Book *book);
    void updateBookList(const QList<Book*> &books);
    void updateUserInfo();
    void checkForUpgrade();
    void updateAdminBookList(const QList<Book*>& books);

protected:
    void showEvent(QShowEvent *event) override;

};

#endif // MAINWINDOW_H
