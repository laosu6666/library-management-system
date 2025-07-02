// src/mainwindow.cpp
#include "mainwindows.h"
#include "ui_mainwindow.h"
#include "login.h"
#include "database.h"
#include "creditdialog.h"
#include "addbookdialog.h"
#include "usermanagerdialog.h"
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QDateTime>
#include <QTimer>
#include <QDebug>

MainWindow::MainWindow(Library *library, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_library(library), m_currentUser(nullptr)
{
    qDebug() << "MainWindow constructor start";

    // 先初始化UI
    ui->setupUi(this);  // 确保UI对象树先构建完成
    qDebug() << "UI setup complete";

    // 再初始化数据库
    qDebug() << "Initializing database...";
    if (!Database::instance()->initialize()) {
        QMessageBox::critical(this, "错误", "数据库初始化失败");
        exit(1);
    }
    qDebug() << "Database initialized";

    // 设置UI初始状态（延迟到showEvent中执行）
    qDebug() << "Deferring initial UI setup";

    // 连接信号槽
    qDebug() << "Connecting signals...";
    connect(ui->actionLogin, &QAction::triggered, this, &MainWindow::showLoginDialog);
        connect(ui->actionLogout, &QAction::triggered, this, &MainWindow::logout); // 连接到logout方法
        connect(ui->btnSearch, &QPushButton::clicked, this, &MainWindow::onSearchBooks);
        connect(ui->btnBorrow, &QPushButton::clicked, this, &MainWindow::onBorrowBook);
        connect(ui->btnReturn, &QPushButton::clicked, this, &MainWindow::onReturnBook);
        connect(ui->btnRenew, &QPushButton::clicked, this, &MainWindow::onRenewBook);
        connect(ui->btnAddComment, &QPushButton::clicked, this, &MainWindow::onAddComment);
        connect(ui->btnPayFines, &QPushButton::clicked, this, &MainWindow::onPayFines);
        connect(ui->btnManageCredit, &QPushButton::clicked, this, &MainWindow::onManageCredit);
        connect(ui->btnAddBook, &QPushButton::clicked, this, &MainWindow::onAddBook);
        connect(ui->btnRemoveBook, &QPushButton::clicked, this, &MainWindow::onRemoveBook);
        connect(ui->btnManageUsers, &QPushButton::clicked, this, &MainWindow::onManageUsers);
        connect(ui->btnViewTopBooks, &QPushButton::clicked, this, &MainWindow::onViewTopBooks);

    qDebug() << "Signals connected";

    // 创建升级检查定时器
    QTimer *upgradeTimer = new QTimer(this);
    connect(upgradeTimer, &QTimer::timeout, this, &MainWindow::checkForUpgrade);
    upgradeTimer->start(3600000); // 1小时

    qDebug() << "MainWindow constructor end";
}
void MainWindow::showLoginDialog()
{
    LoginDialog dlg(this);
    if(dlg.exec() == QDialog::Accepted) {
        // 删除旧用户（如果存在）
        if (m_currentUser) {
            delete m_currentUser;
            m_currentUser = nullptr;
        }

        // 转移所有权
        m_currentUser = dlg.getAuthenticatedUser();
        updateUI();
        updateUserInfo();
        checkForUpgrade();
    }
}
void MainWindow::logout()
{
    m_currentUser = nullptr;
    updateUI();

    // 清空用户信息
    ui->lblUserName->setText("未登录");
    ui->lblUserID->setText("N/A");
    ui->lblUserType->setText("未登录");
    ui->lblReadingHours->setText("0小时");
    ui->lblCreditScore->setText("100");
    ui->lblCurrentBorrow->setText("0/5");
    ui->lblFines->setText("0.00元");

    // 清空借阅记录表
    ui->tblBorrowedBooks->clearContents();
    ui->tblBorrowedBooks->setRowCount(0);

    // 重置评论区域
    ui->txtComment->clear();
    ui->spinRating->setValue(3);
}

void MainWindow::updateUI()
{
    qDebug() << "updateUI called";

    if (!ui || !ui->actionLogin || !ui->actionLogout || !ui->btnBorrow) {
        qWarning() << "UI elements not initialized, skipping updateUI";
        return;
    }

    bool loggedIn = (m_currentUser != nullptr);
    bool isAdmin = loggedIn && (m_currentUser->type() == User::Super);

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

    if(loggedIn && m_currentUser) {
        setWindowTitle(tr("图书管理系统 - 用户: %1").arg(m_currentUser->name()));
    } else {
        setWindowTitle(tr("图书管理系统 - 未登录"));
    }
}

void MainWindow::onSearchBooks()
{
    // 添加UI元素空指针检查
    if (!ui || !ui->txtSearch || !ui->tblBooks) {
        qWarning() << "UI elements not ready in onSearchBooks";
        return;
    }

    QString keyword = ui->txtSearch->text();
    if(keyword.isEmpty()) {
        // 确保表格有效
        if (ui->tblBooks->rowCount() == 0) {
            ui->tblBooks->setRowCount(1);
        }

        // 安全设置表格项
        QTableWidgetItem* item = new QTableWidgetItem("请输入搜索关键词");
        if (ui->tblBooks->item(0, 0) == nullptr) {
            ui->tblBooks->setItem(0, 0, item);
        } else {
            ui->tblBooks->item(0, 0)->setText("请输入搜索关键词");
        }
        return;
    }

    if (!m_library) {
        qWarning() << "Library system not available for search";
        return;
    }

    QList<Book*> books = m_library->searchBooks(keyword);
    updateBookList(books);
}

void MainWindow::updateBookList(const QList<Book*>& books)
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
    if(!m_currentUser || !ui) {
            qDebug() << "onBorrowBook: Invalid user or UI";
            return;
        }
    if(!m_currentUser) return;
    if (!m_library) return;

    int row = ui->tblBooks->currentRow();
    if(row < 0 || row >= ui->tblBooks->rowCount()) {
        QMessageBox::information(this, "提示", "请选择有效的图书");
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
    if (!m_currentUser || !ui) {
            qDebug() << "updateUserInfo: Invalid user or UI";
            return;
        }
    qDebug() << "updateUserInfo called";
    if (!m_currentUser) {
        qDebug() << "m_currentUser is nullptr!";
        return;
    }

    ui->lblUserName->setText(m_currentUser->name());
    ui->lblUserID->setText(m_currentUser->id());

    // 显示用户类型和信用分
    QString userType = (m_currentUser->type() == User::Super) ? "超级读者" : "普通读者";
    QString creditInfo = QString::number(m_currentUser->creditScore());

    ui->lblUserType->setText(userType);
    ui->lblCreditScore->setText(creditInfo);

    ui->lblReadingHours->setText(QString::number(m_currentUser->readingHours(), 'f', 1) + "小时");
    ui->lblFines->setText(QString::number(m_currentUser->fines(), 'f', 2) + "元");

    // 显示当前借阅数量
    int currentBorrow = m_library->getCurrentBorrowCount(m_currentUser->id());
    ui->lblCurrentBorrow->setText(QString("%1 / %2")
                                 .arg(currentBorrow)
                                 .arg(m_currentUser->maxBorrowCount()));

    // 显示借阅记录
    QList<Book*> borrowedBooks = m_library->getBooksBorrowedByUser(m_currentUser->id());

    // 安全处理表格更新
    if (ui->tblBorrowedBooks) {
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
}

void MainWindow::onManageCredit()
{
    if(!m_currentUser) return;
    if (!m_library) return;

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

void MainWindow::onReturnBook()
{
    if(!m_currentUser) return;
    if (!m_library) return;

    int row = ui->tblBorrowedBooks->currentRow();
    if(row < 0 || row >= ui->tblBorrowedBooks->rowCount()) {
        QMessageBox::information(this, "提示", "请选择有效的图书");
        return;
    }

    QString isbn = ui->tblBorrowedBooks->item(row, 0)->text();
    if(m_library->returnBook(m_currentUser->id(), isbn)) {
        QMessageBox::information(this, "成功", "图书归还成功");
        updateUserInfo();
        onSearchBooks();
    } else {
        QMessageBox::warning(this, "失败", "图书归还失败");
    }
}

void MainWindow::onRenewBook()
{
    if(!m_currentUser) return;
    if (!m_library) return;

    int row = ui->tblBorrowedBooks->currentRow();
    if(row < 0 || row >= ui->tblBorrowedBooks->rowCount()) {
        QMessageBox::information(this, "提示", "请选择有效的图书");
        return;
    }

    QString isbn = ui->tblBorrowedBooks->item(row, 0)->text();
    if(m_library->renewBook(m_currentUser->id(), isbn)) {
        QMessageBox::information(this, "成功", "续借成功");
        updateUserInfo();
    } else {
        QMessageBox::warning(this, "失败", "续借失败");
    }
}

void MainWindow::onAddComment()
{
    if(!m_currentUser) return;
    if (!m_library) return;

    int row = ui->tblBooks->currentRow();
    if(row < 0 || row >= ui->tblBooks->rowCount()) {
        QMessageBox::information(this, "提示", "请选择有效的图书");
        return;
    }

    QString isbn = ui->tblBooks->item(row, 0)->text();
    QString comment = ui->txtComment->toPlainText();
    int rating = ui->spinRating->value();

    if(comment.isEmpty()) {
        QMessageBox::information(this, "提示", "请输入评论内容");
        return;
    }

    if(m_library->addComment(m_currentUser->id(), isbn, comment, rating)) {
        QMessageBox::information(this, "成功", "评论成功");
        ui->txtComment->clear(); // 清空评论框
        ui->spinRating->setValue(3); // 重置评分
        onSearchBooks();
    } else {
        QMessageBox::warning(this, "失败", "评论失败");
    }
}

void MainWindow::onPayFines()
{
    if(!m_currentUser) return;
    if (!m_library) return;

    double amount = ui->spinPayFines->value();
    if(amount <= 0) {
        QMessageBox::warning(this, "错误", "请输入有效金额");
        return;
    }

    if(m_library->payFines(m_currentUser->id(), amount)) {
        QMessageBox::information(this, "成功", "罚款支付成功");
        ui->spinPayFines->setValue(0.0); // 重置支付金额
        updateUserInfo();
    } else {
        QMessageBox::warning(this, "失败", "支付失败");
    }
}

void MainWindow::onRemoveBook()
{
    if (!m_library) return;

    int row = ui->tblBooks->currentRow();
    if(row < 0 || row >= ui->tblBooks->rowCount()) {
        QMessageBox::information(this, "提示", "请选择有效的图书");
        return;
    }

    QString isbn = ui->tblBooks->item(row, 0)->text();
    if(m_library->removeBook(isbn)) {
        QMessageBox::information(this, "成功", "图书删除成功");
        onSearchBooks();
    } else {
        QMessageBox::warning(this, "失败", "删除失败");
    }
}

void MainWindow::onViewTopBooks()
{
    if (!m_library) return;

    QList<Book*> books = m_library->getTopRatedBooks();
    updateBookList(books);
}

void MainWindow::showBookDetails(Book *book)
{
    if(!book) return;

    QString info = QString("书名：%1\n作者：%2\n出版社：%3\n出版日期：%4\n简介：%5")
        .arg(book->title())
        .arg(book->author())
        .arg(book->publisher())
        .arg(book->publishDate().toString("yyyy-MM-dd"))
        .arg(book->introduction());

    QMessageBox::information(this, "图书详情", info);
}

void MainWindow::onLogin()
{
    showLoginDialog();
}

void MainWindow::onAddBook()
{
    if (!m_library) return;

    AddBookDialog dlg(m_library, this);
    if (dlg.exec() == QDialog::Accepted) {
        onSearchBooks(); // 刷新图书列表
    }
}

void MainWindow::onManageUsers()
{
    if (!m_library) return;

    UserManagerDialog dlg(m_library, this);
    dlg.exec();
}
void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    static bool firstShow = true;
    if (firstShow) {
        firstShow = false;

        // 使用单次定时器延迟初始化
        QTimer::singleShot(100, this, [this]() {
            qDebug() << "Performing delayed initial UI setup";

            // 添加空指针检查
            if (!ui || !ui->tblBooks) {
                qWarning() << "UI not ready for initial setup";
                return;
            }

            updateUI();
            if (m_library) {
                onSearchBooks();
            }
        });
    }
}
MainWindow::~MainWindow()
{
    qDebug() << "MainWindow destructor start";

    // 清理资源
    delete ui;
    if (m_currentUser) {
        delete m_currentUser;
        m_currentUser = nullptr;
    }

    // 注意：m_library 由外部管理，不需要在这里删除

    qDebug() << "MainWindow destructor end";
}
void MainWindow::on_btnRemoveBook_clicked()
{
    int row = ui->tblBooks->currentRow();
    if (row < 0) return;
    QString isbn = ui->tblBooks->item(row, 0)->text();

    if (m_library->removeBook(isbn)) {
        QMessageBox::information(this, "提示", "删除成功");
        onSearchBooks(); // 删除后刷新
    } else {
        QMessageBox::warning(this, "提示", "删除失败，可能有未归还的借阅记录");
    }
}
