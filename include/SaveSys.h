#pragma once

#include "Common.h"

#include "Map.h"
#include "GameTime.h"

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

    // 保存玩家存档（包含坐标和时间）
    // 调用前需保证 initDatabase() 已成功执行
    bool savePlayer(const Character& player, const GamePoint& pos, const GameTime& time);

    // 读取最新存档
    // 自动根据数据库中的 classType 构造对应职业的角色实例，并恢复坐标和时间
    bool loadLatestSave(std::shared_ptr<Character>& outPlayer, GamePoint& outPos, GameTime& outTime);
};
