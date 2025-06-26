// src/mainwindow.cpp
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "login.h"
#include "creditdialog.h"
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QDateTime>
#include <QTimer>

MainWindow::MainWindow(Library *library, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_library(library), m_currentUser(nullptr)
{
    ui->setupUi(this);

    // 连接信号槽
    connect(ui->actionLogin, &QAction::triggered, this, &MainWindow::showLoginDialog);
    connect(ui->btnSearch, &QPushButton::clicked, this, &MainWindow::onSearchBooks);
    connect(ui->btnBorrow, &QPushButton::clicked, this, &MainWindow::onBorrowBook);
    connect(ui->btnReturn, &QPushButton::clicked, this, &MainWindow::onReturnBook);
    connect(ui->btnRenew, &QPushButton::clicked, this, &MainWindow::onRenewBook);
    connect(ui->btnAddComment, &QPushButton::clicked, this, &MainWindow::onAddComment);
    connect(ui->btnPayFines, &QPushButton::clicked, this, &MainWindow::onPayFines);
    connect(ui->btnManageCredit, &QPushButton::clicked, this, &MainWindow::onManageCredit);

    // 初始状态
    updateUI();

    // 创建定时器，每小时检查一次升级条件
    QTimer *upgradeTimer = new QTimer(this);
    connect(upgradeTimer, &QTimer::timeout, this, &MainWindow::checkForUpgrade);
    upgradeTimer->start(3600000); // 1小时
}

void MainWindow::showLoginDialog()
{
    LoginDialog dlg(this);
    if(dlg.exec() == QDialog::Accepted) {
        m_currentUser = dlg.getAuthenticatedUser();
        updateUI();
        updateUserInfo();
    }
}

void MainWindow::updateUI()
{
    bool loggedIn = (m_currentUser != nullptr);
    bool isAdmin = loggedIn && (m_currentUser->type() == User::Super);

    // 设置UI元素启用状态
    ui->actionLogin->setEnabled(!loggedIn);
    ui->actionLogout->setEnabled(loggedIn);
    ui->btnBorrow->setEnabled(loggedIn);
    ui->btnReturn->setEnabled(loggedIn);
    ui->btnRenew->setEnabled(loggedIn);
    ui->btnAddComment->setEnabled(loggedIn);
    ui->btnPayFines->setEnabled(loggedIn);
    ui->btnManageCredit->setEnabled(loggedIn);
    ui->btnAddBook->setEnabled(isAdmin);
    ui->btnRemoveBook->setEnabled(isAdmin);
    ui->btnManageUsers->setEnabled(isAdmin);

    // 设置窗口标题
    if(loggedIn) {
        setWindowTitle(tr("图书管理系统 - 用户: %1").arg(m_currentUser->name()));
    } else {
        setWindowTitle(tr("图书管理系统 - 未登录"));
    }
}

void MainWindow::onSearchBooks()
{
    QString keyword = ui->txtSearch->text();
    if(keyword.isEmpty()) {
        QMessageBox::information(this, "提示", "请输入搜索关键词");
        return;
    }

    QList<Book*> books = m_library->searchBooks(keyword);
    updateBookList(books);
}

void MainWindow::updateBookList(const QList<Book*> &books)
{
    ui->tblBooks->clearContents();
    ui->tblBooks->setRowCount(books.size());

    for(int i = 0; i < books.size(); ++i) {
        Book *book = books[i];
        ui->tblBooks->setItem(i, 0, new QTableWidgetItem(book->isbn()));
        ui->tblBooks->setItem(i, 1, new QTableWidgetItem(book->title()));
        ui->tblBooks->setItem(i, 2, new QTableWidgetItem(book->author()));
        ui->tblBooks->setItem(i, 3, new QTableWidgetItem(QString::number(book->availableCopies())));

        // 显示评分
        double rating = Book::calculateAverageRating(book->isbn());
        ui->tblBooks->setItem(i, 4, new QTableWidgetItem(QString::number(rating, 'f', 1)));
    }
}

void MainWindow::onBorrowBook()
{
    if(!m_currentUser) return;

    int row = ui->tblBooks->currentRow();
    if(row < 0) {
        QMessageBox::information(this, "提示", "请选择要借阅的图书");
        return;
    }

    QString isbn = ui->tblBooks->item(row, 0)->text();
    if(m_library->borrowBook(m_currentUser->id(), isbn)) {
        QMessageBox::information(this, "成功", "图书借阅成功");
        onSearchBooks(); // 刷新列表
        updateUserInfo(); // 更新用户信息
    } else {
        QMessageBox::warning(this, "失败", "图书借阅失败");
    }
}

void MainWindow::updateUserInfo()
{
    if(!m_currentUser) return;

    ui->lblUserName->setText(m_currentUser->name());
    ui->lblUserID->setText(m_currentUser->id());

    // 显示用户类型和信用分
    QString userType = (m_currentUser->type() == User::Super) ? "超级读者" : "普通读者";
    QString creditInfo = QString("信用分: %1").arg(m_currentUser->creditScore());

    ui->lblUserType->setText(userType);
    ui->lblCreditScore->setText(creditInfo);

    ui->lblReadingHours->setText(QString::number(m_currentUser->readingHours(), 'f', 1));
    ui->lblFines->setText(QString::number(m_currentUser->fines(), 'f', 2));

    // 显示当前借阅数量
    int currentBorrow = m_library->getCurrentBorrowCount(m_currentUser->id());
    ui->lblCurrentBorrow->setText(QString("%1 / %2")
                                 .arg(currentBorrow)
                                 .arg(m_currentUser->maxBorrowCount()));

    // 显示借阅记录
    QList<Book*> borrowedBooks = m_library->getBooksBorrowedByUser(m_currentUser->id());
    ui->tblBorrowedBooks->clearContents();
    ui->tblBorrowedBooks->setRowCount(borrowedBooks.size());

    for(int i = 0; i < borrowedBooks.size(); ++i) {
        Book *book = borrowedBooks[i];
        // 获取借阅记录详情
        Library::BorrowRecord record = m_library->getBorrowRecord(m_currentUser->id(), book->isbn());

        ui->tblBorrowedBooks->setItem(i, 0, new QTableWidgetItem(book->isbn()));
        ui->tblBorrowedBooks->setItem(i, 1, new QTableWidgetItem(book->title()));
        ui->tblBorrowedBooks->setItem(i, 2, new QTableWidgetItem(record.borrowDate.toString("yyyy-MM-dd")));
        ui->tblBorrowedBooks->setItem(i, 3, new QTableWidgetItem(record.dueDate.toString("yyyy-MM-dd")));

        // 显示剩余天数
        int daysLeft = QDate::currentDate().daysTo(record.dueDate);
        QTableWidgetItem *daysItem = new QTableWidgetItem(QString::number(daysLeft));
        if(daysLeft <= 3) {
            daysItem->setForeground(Qt::red); // 即将到期显示为红色
        }
        ui->tblBorrowedBooks->setItem(i, 4, daysItem);
    }
}

void MainWindow::onManageCredit()
{
    if(!m_currentUser) return;

    CreditDialog dlg(m_currentUser, m_library, this);
    dlg.exec();
    updateUserInfo(); // 更新用户信息
}

void MainWindow::checkForUpgrade()
{
    if(!m_currentUser) return;

    if(m_currentUser->type() == User::Normal) {
        float hoursNeeded = 200.0 - m_currentUser->readingHours();
        if(hoursNeeded > 0 && hoursNeeded <= 10) {
            QMessageBox::information(this, "升级提示",
                QString("您还差 %1 小时阅读时长即可升级为超级读者！\n"
                        "升级后将获得：\n"
                        "  - 最大借阅数量增加到8本\n"
                        "  - 借阅期限延长到4周\n"
                        "  - 信用分提升至120")
                .arg(hoursNeeded, 0, 'f', 1));
        }

        // 检查信用分是否低于90
        if(m_currentUser->creditScore() < 90) {
            QMessageBox::warning(this, "信用分警告",
                "您的信用分低于90分，已失去升级资格！\n"
                "请通过缴费提升信用分");
        }
    }
}
