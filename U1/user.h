// include/user.h
#ifndef USER_H
#define USER_H

#include <QObject>
#include <QString>
#include <QMessageBox>

class User : public QObject
{
    Q_OBJECT
public:
    enum UserType { Normal, Super };

    explicit User(const QString &id, const QString &email,
                 const QString &password, const QString &name,
                 UserType type = Normal, float readingHours = 0.0,
                 double fines = 0.0, int creditScore = 100,
                 bool hadLowCredit = false, QObject *parent = nullptr);

    QString id() const;
    QString email() const;
    QString password() const;
    QString name() const;
    UserType type() const;
    float readingHours() const;
    double fines() const;
    int creditScore() const;
    bool hadLowCredit() const;

    // 借阅规则
    int maxBorrowCount() const;
    int borrowDays() const;

    // 升级功能
    bool upgradeToSuper();
    void addReadingHours(float hours);

    // 罚款管理
    void addFine(double amount);
    void payFine(double amount);

    // 信用分管理
    void setCreditScore(int score);
    void setHadLowCredit(bool had);
    void addCreditScore(int points);
    void deductCreditScore(int points);
    bool canBorrow() const;
    bool canUpgrade() const;
    void payFineWithCredit(double amount);

    // 用户ID生成
    static QString generateUserId();

private:
    QString m_id;
    QString m_email;
    QString m_password;
    QString m_name;
    UserType m_type;
    float m_readingHours;
    double m_fines;
    int m_creditScore;
    bool m_hadLowCredit;

    // 借阅规则
    int m_maxBorrow;
    int m_borrowDays;
};

#endif // USER_H
