#include "../include/SaveSys.h"

// 构造函数：初始化数据库连接
SaveSys::SaveSys() {
    // 一键连接 SQLite，全平台通用
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("game_archive.db"); // 存档直接生成在运行目录下
}

// 初始化建表的方法
bool SaveSys::initDatabase() {
    if (!db.open()) {
        qDebug() << "数据库打开失败:" << db.lastError().text();
        return false;
    }

    QSqlQuery query;
    // 执行建表 SQL 语句
    bool success = query.exec("CREATE TABLE IF NOT EXISTS player ("
                              "name TEXT, "
                              "level INTEGER, "
                              "hp INTEGER);");
    if (!success) {
        qDebug() << "建表失败:" << query.lastError().text();
    } else {
        qDebug() << "数据库及玩家表初始化成功！";
    }
    return success;
}
