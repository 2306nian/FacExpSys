// database.h
#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlDatabase>

class Database : public QObject
{
    Q_OBJECT

public:
    static Database *instance();

    // 初始化数据库连接和建表
    bool initialize(const QString &dbPath = "../Db/"); // 存在Db文件夹里

    QSqlDatabase db() const { return m_db; }

private:
    explicit Database(QObject *parent = nullptr);
    static Database *m_instance;
    QSqlDatabase m_db;
};

#endif // DATABASE_H
