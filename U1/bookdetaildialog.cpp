#include "bookdetaildialog.h"
#include "ui_bookdetaildialog.h"
#include <QMessageBox>

BookDetailDialog::BookDetailDialog(Book *book, Library *library, User *user, QWidget *parent)
    : QDialog(parent), ui(new Ui::BookDetailDialog), m_book(book), m_library(library), m_currentUser(user)
{
    ui->setupUi(this);
    setWindowTitle("图书详情");

    // 显示图书详细信息
    ui->lblTitle->setText(m_book->title());
    ui->lblAuthor->setText("作者: " + m_book->author());
    ui->lblPublisher->setText("出版社: " + m_book->publisher());
    ui->lblPublishDate->setText("出版日期: " + m_book->publishDate().toString("yyyy-MM-dd"));
    ui->lblPrice->setText("价格: ¥" + QString::number(m_book->price(), 'f', 2));
    ui->lblISBN->setText("ISBN: " + m_book->isbn());
    ui->txtIntro->setText(m_book->introduction());
    ui->lblAvailableCopies->setText("可借数量: " + QString::number(m_book->availableCopies()));

    // 显示评分
    double rating = Book::calculateAverageRating(m_book->isbn());
    ui->lblRating->setText("评分: " + QString::number(rating, 'f', 1));

    // 根据用户状态设置按钮可用性
    bool canBorrow = m_currentUser && m_currentUser->canBorrow();
    ui->btnBorrow->setEnabled(canBorrow);
    ui->btnAddComment->setEnabled(m_currentUser != nullptr);
}

BookDetailDialog::~BookDetailDialog()
{
    delete ui;
}

void BookDetailDialog::on_btnBorrow_clicked()
{
    if (m_library->borrowBook(m_currentUser->id(), m_book->isbn())) {
        QMessageBox::information(this, "成功", "借阅成功！");
        accept();
    } else {
        QMessageBox::warning(this, "失败", "借阅失败");
    }
}

void BookDetailDialog::on_btnAddComment_clicked()
{
    QString comment = ui->txtComment->toPlainText();
    int rating = ui->spinRating->value();

    if (comment.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入评论内容");
        return;
    }

    if (m_library->addComment(m_currentUser->id(), m_book->isbn(), comment, rating)) {
        QMessageBox::information(this, "成功", "评论添加成功");
        ui->txtComment->clear();

        // 更新评分显示
        double newRating = Book::calculateAverageRating(m_book->isbn());
        ui->lblRating->setText("评分: " + QString::number(newRating, 'f', 1));
    } else {
        QMessageBox::warning(this, "失败", "评论添加失败");
    }
}
