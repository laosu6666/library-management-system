// src/mainwindow.cpp
#include "mainwindows.h"
#include "ui_mainwindow.h"
#include "login.h"
#include "bookdetaildialog.h"
#include "database.h"
#include "library.h"
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
    ui->setupUi(this);
    qDebug() << "UI setup complete";





    // 初始化数据库
    qDebug() << "Initializing database...";
    if (!Database::instance()->initialize()) {
        QMessageBox::critical(this, "错误", "数据库初始化失败");
        exit(1);
    }
    qDebug() << "Database initialized";

    // 连接信号槽
    qDebug() << "Connecting signals...";


    connect(ui->actionLogin, &QAction::triggered, this, &MainWindow::showLoginDialog);
    connect(ui->actionLogout, &QAction::triggered, this, &MainWindow::logout);
    connect(ui->btnSearch, &QPushButton::clicked, this, &MainWindow::onSearchBooks);
    connect(ui->btnReturn, &QPushButton::clicked, this, &MainWindow::onReturnBook);
    connect(ui->btnRenew, &QPushButton::clicked, this, &MainWindow::onRenewBook);
    connect(ui->btnPayFines, &QPushButton::clicked, this, &MainWindow::onPayFines);
    connect(ui->btnManageCredit, &QPushButton::clicked, this, &MainWindow::onManageCredit);
    connect(ui->btnAddBook, &QPushButton::clicked, this, &MainWindow::onAddBook);
    connect(ui->btnRemoveBook, &QPushButton::clicked, this, &MainWindow::onRemoveBook);
    connect(ui->btnManageUsers, &QPushButton::clicked, this, &MainWindow::onManageUsers);
    connect(ui->tblBooks, &QTableWidget::cellDoubleClicked, this, &MainWindow::onBookDetails);

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


}

void MainWindow::updateUI()
{
    qDebug() << "updateUI called";


    bool loggedIn = (m_currentUser != nullptr);
    bool isAdmin = loggedIn && (m_currentUser->type() == User::Super);

    ui->actionLogin->setEnabled(!loggedIn);
    ui->actionLogout->setEnabled(loggedIn);

    ui->btnReturn->setEnabled(loggedIn);
    ui->btnRenew->setEnabled(loggedIn);

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
    if (!ui || !ui->tblBooks) return;

    // 保存当前搜索结果
    m_currentSearchResults = books;

    ui->tblBooks->clearContents();
    ui->tblBooks->setRowCount(books.size());

    for(int i = 0; i < books.size(); ++i) {
        Book *book = books[i];
        ui->tblBooks->setItem(i, 0, new QTableWidgetItem(book->title()));
        ui->tblBooks->setItem(i, 1, new QTableWidgetItem(book->publisher()));
        ui->tblBooks->setItem(i, 2, new QTableWidgetItem(QString::number(book->availableCopies())));
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

        // 检查是否需要升级
        if(m_currentUser->canUpgrade() && hoursNeeded <= 10 && hoursNeeded > 0) {
            QMessageBox::information(this, "升级提示",
                QString("您还差 %1 小时阅读时长即可升级为超级读者！\n"
                        "升级后将获得：\n"
                        "  - 最大借阅数量增加到8本\n"
                        "  - 借阅期限延长到4周\n"
                        "  - 信用分提升至120")
                .arg(hoursNeeded, 0, 'f', 1));
        }
        // 检查信用分是否低于90
        else if(!m_currentUser->canUpgrade()) {
            QMessageBox::warning(this, "信用分警告",
                "您曾信用分低于90分，已失去升级资格！\n"
                "请保持良好信用记录");
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
    // 权限检查
    if (!m_currentUser || m_currentUser->type() != User::Super) {
        QMessageBox::warning(this, "权限不足", "只有管理员可以删除图书");
        return;
    }

    // 检查当前是否在管理员标签页
    if (ui->tabWidget->currentIndex() != 2) { // 假设系统管理是第3个标签页
        ui->tabWidget->setCurrentIndex(2);
    }

    // 获取选中的图书
    int row = ui->tblAdminBooks->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "提示", "请选择要删除的图书");
        return;
    }

    QString isbn = ui->tblAdminBooks->item(row, 0)->text();
    QString title = ui->tblAdminBooks->item(row, 1)->text();

    // 确认对话框
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(
        this,
        "确认删除",
        QString("确定要删除图书《%1》(ISBN:%2)吗?\n此操作不可撤销!").arg(title).arg(isbn),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply != QMessageBox::Yes) return;

    // 执行删除
    if (m_library->removeBook(isbn)) {
        QMessageBox::information(this, "成功", "图书删除成功");

        // 刷新管理员图书列表
        QList<Book*> books = m_library->searchBooks(""); // 获取所有图书
        updateAdminBookList(books);
    } else {
        QMessageBox::warning(this, "失败",
            "删除失败，可能原因：\n"
            "1. 存在未归还的副本\n"
            "2. 存在未处理的预约\n"
            "3. 数据库错误");
    }
}

// 新增更新管理员表格函数
void MainWindow::updateAdminBookList(const QList<Book*>& books)
{
    if (!ui || !ui->tblAdminBooks) return;

    ui->tblAdminBooks->clearContents();
    ui->tblAdminBooks->setRowCount(books.size());

    for(int i = 0; i < books.size(); ++i) {
        Book *book = books[i];
        ui->tblAdminBooks->setItem(i, 0, new QTableWidgetItem(book->isbn()));
        ui->tblAdminBooks->setItem(i, 1, new QTableWidgetItem(book->title()));
        ui->tblAdminBooks->setItem(i, 2, new QTableWidgetItem(book->author()));
        ui->tblAdminBooks->setItem(i, 3, new QTableWidgetItem(book->publisher()));
        ui->tblAdminBooks->setItem(i, 4, new QTableWidgetItem(QString::number(book->availableCopies())));
    }
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
    AddBookDialog dlg(m_library, this);
    if (dlg.exec() == QDialog::Accepted) {
        // 添加成功后清空搜索框并刷新
        ui->txtSearch->clear();
        onSearchBooks();

        // 刷新管理员视图
        QList<Book*> books = m_library->searchBooks("");
        updateAdminBookList(books);

        // 刷新排行榜
        onViewTopBooks(); // 添加这行
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

        QTimer::singleShot(100, this, [this]() {
            // 初始化数据库
            if (!Database::instance()->initialize()) {
                QMessageBox::critical(this, "错误", "数据库初始化失败");
                return;
            }

            updateUI();

            // 加载搜索图书
            if (m_library) {
                onSearchBooks();

                // 加载管理员图书列表
                QList<Book*> books = m_library->searchBooks("");
                updateAdminBookList(books);

                 onViewTopBooks();  // 添加这行
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
void MainWindow::onReserveBook()
{
    if(!m_currentUser || !m_library) return;
    int row = ui->tblBooks->currentRow();
    if(row < 0 || row >= ui->tblBooks->rowCount()) {
        QMessageBox::information(this, "提示", "请选择有效的图书");
        return;
    }
    QString isbn = ui->tblBooks->item(row, 0)->text();
    if(m_library->reserveBook(m_currentUser->id(), isbn)) {
        QMessageBox::information(this, "成功", "预约成功");
    } else {
        QMessageBox::warning(this, "失败", "预约失败");
    }
}
void MainWindow::onBookDetails()
{
    int row = ui->tblBooks->currentRow();
    if(row < 0 || row >= ui->tblBooks->rowCount()) {
        QMessageBox::information(this, "提示", "请选择有效的图书");
        return;
    }

    // 确保行号在搜索结果范围内
    if (row < m_currentSearchResults.size()) {
        Book* book = m_currentSearchResults[row];

        // 创建书籍详情对话框
        BookDetailDialog dlg(book, m_library, m_currentUser, this);
        if(dlg.exec() == QDialog::Accepted) {
            // 借阅成功后刷新界面
            onSearchBooks();
            updateUserInfo();
        }
    } else {
        QMessageBox::warning(this, "错误", "书籍信息不可用");
    }
}
void MainWindow::onViewTopBooks()
{
    if (!ui || !ui->tblTopBooks) {
        qWarning() << "tblTopBooks not available";
        return;
    }

    // 设置表格列数
    ui->tblTopBooks->setColumnCount(4);
    QStringList headers;
    headers << "ISBN" << "书名" << "平均评分" << "评论数";
    ui->tblTopBooks->setHorizontalHeaderLabels(headers);

    ui->tblTopBooks->clearContents();
    ui->tblTopBooks->setRowCount(0);

    if (!m_library) {
        qWarning() << "Library system not available";
        return;
    }

    // 获取排行数据
    QList<BookRankInfo> books = m_library->getTopRankedBooks(10);
    if (books.isEmpty()) {
        ui->tblTopBooks->setRowCount(1);
        QTableWidgetItem* item = new QTableWidgetItem("暂无数据");
        ui->tblTopBooks->setItem(0, 0, item);
        ui->tblTopBooks->setSpan(0, 0, 1, 4); // 合并单元格
        return;
    }

    ui->tblTopBooks->setRowCount(books.size());
    for (int i = 0; i < books.size(); ++i) {
        const BookRankInfo &info = books[i];
        ui->tblTopBooks->setItem(i, 0, new QTableWidgetItem(info.isbn));
        ui->tblTopBooks->setItem(i, 1, new QTableWidgetItem(info.title));
        ui->tblTopBooks->setItem(i, 2, new QTableWidgetItem(QString::number(info.avgRating, 'f', 1)));
        ui->tblTopBooks->setItem(i, 3, new QTableWidgetItem(QString::number(info.commentCount)));
    }
}
