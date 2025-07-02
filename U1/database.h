// include/database.h
#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

class Database : public QObject
{
    Q_OBJECT
public:
    explicit Database(QObject *parent = nullptr);
    ~Database();

    bool initialize();
    bool execute(const QString &query);
    QSqlQuery executeQuery(const QString &query);
    QString escapeString( QString input);

    static Database* instance();

private:
    QSqlDatabase db;
    static Database* m_instance;
};

#endif // DATABASE_H
