#pragma once

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QString>

class SaveSys {
private:
    QSqlDatabase db;

public:
    // 构造函数：初始化数据库连接
    SaveSys();

    // 初始化建表的方法
    bool initDatabase();

    // 以后组员可以在这里扩展：保存角色、读取角色等函数
    // void savePlayer(const Character& player);
    // Character loadPlayer();
};
