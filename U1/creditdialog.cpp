// src/creditdialog.cpp
#include "creditdialog.h"
#include "ui_creditdialog.h"
#include <QMessageBox>

CreditDialog::CreditDialog(User *user, Library *library, QWidget *parent)
    : QDialog(parent), ui(new Ui::CreditDialog), m_user(user), m_library(library)
{
    ui->setupUi(this);
    setWindowTitle("信用分管理");

    // 显示用户信息
    ui->lblUserName->setText(m_user->name());
    ui->lblCreditScore->setText(QString::number(m_user->creditScore()));

    // 显示当前罚款
    double fines = m_library->getUserFines(m_user->id());
    ui->lblFines->setText(QString::number(fines, 'f', 2));

    // 连接信号槽
    connect(ui->btnAddCredit, &QPushButton::clicked, this, &CreditDialog::onAddCreditClicked);
    connect(ui->btnPayWithCredit, &QPushButton::clicked, this, &CreditDialog::onPayWithCreditClicked);
}
void CreditDialog::onPayWithCreditClicked()
{
    double amount = ui->spinAmount->value();
    if(amount <= 0) {
        QMessageBox::warning(this, "错误", "请输入有效金额");
        return;
    }

    // 检查信用分是否足够支付罚款
    if (m_user->creditScore() < amount) {
        QMessageBox::warning(this, "错误", "信用分不足");
        return;
    }

    // 不能支付超过当前罚款的金额
    if (amount > m_user->fines()) {
        amount = m_user->fines();
    }

    // 1. 支付罚款
    m_user->payFine(amount);

    // 2. 扣除信用分 (1元 = 1信用分)
    m_user->deductCreditScore(static_cast<int>(amount));

    // 更新显示
    ui->lblFines->setText(QString::number(m_user->fines(), 'f', 2));
    ui->lblCreditScore->setText(QString::number(m_user->creditScore()));

    QMessageBox::information(this, "成功",
        QString("罚款支付成功！扣除信用分 %1 分").arg(static_cast<int>(amount)));
}
void CreditDialog::onAddCreditClicked()
{
    double amount = ui->spinAmount->value();
    if(amount <= 0) {
        QMessageBox::warning(this, "错误", "请输入有效金额");
        return;
    }

    // 缴费增加信用分 (1元 = 1信用分)
    int creditToAdd = static_cast<int>(amount);
    m_user->addCreditScore(creditToAdd);

    // 更新显示
    ui->lblCreditScore->setText(QString::number(m_user->creditScore()));

    QMessageBox::information(this, "成功",
        QString("缴费成功！信用分增加 %1 分").arg(creditToAdd));
}


CreditDialog::~CreditDialog()
{
    delete ui;
}

