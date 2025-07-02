#include "addbookdialog.h"
#include "ui_addbookdialog.h"
#include <QMessageBox>

AddBookDialog::AddBookDialog(Library *library, QWidget *parent)
    : QDialog(parent), ui(new Ui::AddBookDialog), m_library(library)
{
    ui->setupUi(this);
    connect(ui->btnAdd, &QPushButton::clicked, this, &AddBookDialog::on_btnAdd_clicked);
}

AddBookDialog::~AddBookDialog()
{
    delete ui;
}

void AddBookDialog::on_btnAdd_clicked()
{
    QString isbn = ui->txtISBN->text().trimmed();
    QString title = ui->txtTitle->text().trimmed();
    QString author = ui->txtAuthor->text().trimmed();
    int totalCopies = ui->spinTotalCopies->value();
    QString publisher = ui->txtPublisher->text().trimmed();
    QDate publishDate = ui->datePublish->date();
    double price = ui->spinPrice->value();
    QString intro = ui->txtIntro->toPlainText().trimmed();

    if(isbn.isEmpty() || title.isEmpty() || author.isEmpty() || totalCopies <= 0) {
        QMessageBox::warning(this, "提示", "请填写完整信息");
        return;
    }

    if(m_library->addBook(isbn, title, author, totalCopies, publisher, publishDate, price, intro)) {
        QMessageBox::information(this, "成功", "添加图书成功");
        accept();
    } else {
        QMessageBox::warning(this, "失败", "添加图书失败，ISBN可能已存在");
    }
}
