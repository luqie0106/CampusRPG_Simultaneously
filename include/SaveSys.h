#pragma once

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// 前向声明：避免循环依赖（SaveSys.h 不需要 Character 完整定义）
class Character;

class SaveSys {
private:
    QSqlDatabase db;

    // ── 内部辅助：背包序列化 / 反序列化 ─────────────────────────────────
    // 将 Character 背包 + 已装备槽序列化为 JSON 字符串，存入 SQLite
    static QString serializeInventory(const Character& player);
    // 从 JSON 字符串中还原背包 + 装备到 Character
    static void    deserializeInventory(const QString& json, Character& player);

public:
    // 构造函数：初始化数据库连接
    SaveSys();

    // 初始化建表（CREATE TABLE IF NOT EXISTS）
    bool initDatabase();

    // 保存玩家存档（INSERT OR REPLACE）
    // 调用前需保证 initDatabase() 已成功执行
    bool savePlayer(const Character& player);

    // 读取玩家存档，成功时返回 true 并填充 player
    // player 须已构造（如 Steve/Athlete/Nerd），函数只覆盖可序列化字段
    bool loadPlayer(Character& player);
};
