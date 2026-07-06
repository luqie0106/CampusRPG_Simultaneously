#include "Common.h"

#include "../include/SaveSys.h"
#include "../include/Character.h"  // 需要完整定义以访问 GetBackpack() 等
#include "../include/Exceptions.h"

// ─────────────────────────────────────────────────────────────────────────────
// 构造函数：注册 SQLite 驱动，设置存档文件路径
// ─────────────────────────────────────────────────────────────────────────────
SaveSys::SaveSys() {
    // 使用固定连接名，防止重复调用时产生 "connection already exists" 警告
    if (!QSqlDatabase::contains("CampusRPG_conn")) {
        db = QSqlDatabase::addDatabase("QSQLITE", "CampusRPG_conn");
    } else {
        db = QSqlDatabase::database("CampusRPG_conn");
    }
    db.setDatabaseName("game_archive.db"); // 存档生成在运行目录下
}

// ─────────────────────────────────────────────────────────────────────────────
// initDatabase：建表（扩充版）
//
// player 表字段说明：
//   id             — 主键，方便后续多存档扩展
//   name           — 角色名（同时作唯一标识）
//   classType      — 职业枚举整数（0=Student,1=Athlete,2=Nerd）
//   level          — 当前等级
//   exp            — 当前等级内累积经验
//   hp             — 当前生命值
//   maxHealth      — 最大生命值（由升级次数决定，不可省略）
//   gold           — 持有金币
//   inventory_json — 背包 + 已装备槽的 JSON 序列化字符串
//                    采用 QJsonDocument 读写，格式见 serializeInventory()
// ─────────────────────────────────────────────────────────────────────────────
bool SaveSys::initDatabase() {
    if (!db.open()) {
        throw GameException("数据库打开失败：" + db.lastError().text().toStdString());
    }

    QSqlQuery query(db);
    bool success = query.exec(
        "CREATE TABLE IF NOT EXISTS player ("
        "  id             INTEGER PRIMARY KEY AUTOINCREMENT, "
        "  name           TEXT    NOT NULL UNIQUE, "  // 角色名唯一，方便按名查档
        "  classType      INTEGER NOT NULL DEFAULT 0, "
        "  level          INTEGER NOT NULL DEFAULT 1, "
        "  exp            INTEGER NOT NULL DEFAULT 0, "
        "  hp             INTEGER NOT NULL DEFAULT 20, "
        "  maxHealth      INTEGER NOT NULL DEFAULT 20, "
        "  gold           INTEGER NOT NULL DEFAULT 200, "
        "  inventory_json TEXT    NOT NULL DEFAULT '{}' "  // JSON 字符串，默认空对象
        ");"
    );

    if (!success) {
        throw GameException("建表失败：" + query.lastError().text().toStdString());
    }

    qDebug() << "数据库及玩家表初始化成功！";
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// serializeInventory（私有静态）
//
// JSON 格式设计：
// {
//   "backpack": [
//     { "itemType": "Medicine", "name": "治愈药水", "value": 50, "hpRecovery": 20 },
//     { "itemType": "Food",     "name": "熟牛排",   "value": 30, "hpRecovery": 8,
//       "atkBuff": 3, "defBuff": 0, "duration": 3,
//       "effectType": 0, "effectValue": 0 },
//     { "itemType": "Equipment","name": "铁剑",     "value": 80,
//       "atkBonus": 5, "defBonus": 0, "durability": 8, "slot": 4 }
//   ],
//   "equipped": {
//     "Head":   { ... 同 Equipment 格式 ... },
//     "Body":   null,
//     "Legs":   null,
//     "Feet":   null,
//     "Weapon": { ... }
//   }
// }
// ─────────────────────────────────────────────────────────────────────────────
static QJsonObject equipmentToJson(const Equipment* e) {
    if (!e) return QJsonObject();
    QJsonObject obj;
    obj["itemType"]   = "Equipment";
    obj["name"]       = QString::fromStdString(e->getName());
    obj["value"]      = e->getValue();
    obj["atkBonus"]   = e->GetAttackBonus();
    obj["defBonus"]   = e->GetDefenseBonus();
    obj["durability"] = e->GetDurability();
    obj["slot"]       = static_cast<int>(e->GetSlot());
    return obj;
}

QString SaveSys::serializeInventory(const Character& player) {
    QJsonObject root;

    // ── 1. 背包物品列表 ──────────────────────────────────────────────────────
    QJsonArray backpackArr;
    const auto& items = player.GetBackpack().GetItems();
    for (const auto& itemPtr : items) {
        Item* raw = itemPtr.get();

        if (auto* med = dynamic_cast<Medicine*>(raw)) {
            QJsonObject obj;
            obj["itemType"]   = "Medicine";
            obj["name"]       = QString::fromStdString(med->getName());
            obj["value"]      = med->getValue();
            obj["hpRecovery"] = med->GetHpRecovery();
            backpackArr.append(obj);

        } else if (auto* food = dynamic_cast<Food*>(raw)) {
            QJsonObject obj;
            obj["itemType"]   = "Food";
            obj["name"]       = QString::fromStdString(food->getName());
            obj["value"]      = food->getValue();
            obj["hpRecovery"] = food->GetHpRecovery();
            obj["atkBuff"]    = food->GetAtkBuff();
            obj["defBuff"]    = food->GetDefBuff();
            obj["duration"]   = food->GetDuration();
            obj["effectType"] = static_cast<int>(food->GetEffectType());
            obj["effectValue"]= food->GetEffectValue();
            backpackArr.append(obj);

        } else if (auto* equip = dynamic_cast<Equipment*>(raw)) {
            backpackArr.append(equipmentToJson(equip));
        }
        // 其他未知子类暂不序列化（保持可扩展）
    }
    root["backpack"] = backpackArr;

    // ── 2. 已装备槽 ──────────────────────────────────────────────────────────
    // 注意：GetEquipmentAt 是非 const，此处通过 const_cast 临时绕过；
    // 后续可将 GetEquipmentAt 添加 const 重载来彻底解决。
    Character& mutablePlayer = const_cast<Character&>(player);
    QJsonObject equippedObj;
    auto writeSlot = [&](const char* key, EquipSlot slot) {
        auto ptr = mutablePlayer.GetEquipmentAt(slot);
        equippedObj[key] = ptr ? equipmentToJson(ptr.get()) : QJsonValue(QJsonValue::Null);
    };
    writeSlot("Head",   EquipSlot::Head);
    writeSlot("Body",   EquipSlot::Body);
    writeSlot("Legs",   EquipSlot::Legs);
    writeSlot("Feet",   EquipSlot::Feet);
    writeSlot("Weapon", EquipSlot::Weapon);
    root["equipped"] = equippedObj;

    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

// ─────────────────────────────────────────────────────────────────────────────
// deserializeInventory（私有静态）
//
// 从 JSON 字符串中重建背包物品 + 装备到 player。
// 利用 Item.cpp 中已有的工厂方法（如 Equipment::IronSword()）按名称匹配；
// 若名称匹配失败，则使用 JSON 中记录的原始属性兜底构造。
// ─────────────────────────────────────────────────────────────────────────────

// 按 JSON 对象构造一个 Equipment（兜底路径，不依赖工厂名称）
static std::unique_ptr<Equipment> jsonToEquipment(const QJsonObject& obj) {
    return std::make_unique<Equipment>(
        obj["name"].toString().toStdString(),
        obj["value"].toInt(),
        obj["defBonus"].toInt(),
        obj["atkBonus"].toInt(),
        obj["durability"].toInt(),
        static_cast<EquipSlot>(obj["slot"].toInt())
    );
}

void SaveSys::deserializeInventory(const QString& json, Character& player) {
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (doc.isNull() || !doc.isObject()) return;

    QJsonObject root = doc.object();

    // ── 1. 还原背包 ──────────────────────────────────────────────────────────
    QJsonArray backpackArr = root["backpack"].toArray();
    for (const QJsonValue& val : backpackArr) {
        QJsonObject obj = val.toObject();
        QString type = obj["itemType"].toString();

        if (type == "Medicine") {
            player.GetBackpack().AddItem(std::make_unique<Medicine>(
                obj["name"].toString().toStdString(),
                obj["value"].toInt(),
                obj["hpRecovery"].toInt()
            ));
        } else if (type == "Food") {
            player.GetBackpack().AddItem(std::make_unique<Food>(
                obj["name"].toString().toStdString(),
                obj["value"].toInt(),
                obj["hpRecovery"].toInt(),
                obj["atkBuff"].toInt(),
                obj["defBuff"].toInt(),
                obj["duration"].toInt(),
                static_cast<StatusEffectType>(obj["effectType"].toInt()),
                obj["effectValue"].toInt()
            ));
        } else if (type == "Equipment") {
            player.GetBackpack().AddItem(jsonToEquipment(obj));
        }
    }

    // ── 2. 还原已装备槽 ──────────────────────────────────────────────────────
    QJsonObject equippedObj = root["equipped"].toObject();
    auto restoreSlot = [&](const char* key) {
        QJsonValue slotVal = equippedObj[key];
        if (slotVal.isNull() || !slotVal.isObject()) return;
        QJsonObject obj = slotVal.toObject();
        // 直接用属性还原装备，调用 EquipItem（会自动将旧装备退回背包）
        player.EquipItem(std::make_shared<Equipment>(
            obj["name"].toString().toStdString(),
            obj["value"].toInt(),
            obj["defBonus"].toInt(),
            obj["atkBonus"].toInt(),
            obj["durability"].toInt(),
            static_cast<EquipSlot>(obj["slot"].toInt())
        ));
    };
    restoreSlot("Head");
    restoreSlot("Body");
    restoreSlot("Legs");
    restoreSlot("Feet");
    restoreSlot("Weapon");
}

// ─────────────────────────────────────────────────────────────────────────────
// savePlayer：将玩家全量状态写入数据库（INSERT OR REPLACE）
// ─────────────────────────────────────────────────────────────────────────────
bool SaveSys::savePlayer(const Character& player) {
    if (!db.isOpen() && !db.open()) {
        throw GameException("存档失败：数据库未打开。");
    }

    QString inventoryJson = serializeInventory(player);

    QSqlQuery query(db);
    // INSERT OR REPLACE 利用 name UNIQUE 约束，实现"有则覆盖，无则新增"
    query.prepare(
        "INSERT OR REPLACE INTO player "
        "(name, classType, level, exp, hp, maxHealth, gold, inventory_json) "
        "VALUES (:name, :classType, :level, :exp, :hp, :maxHealth, :gold, :inv);"
    );
    query.bindValue(":name",      QString::fromStdString(player.GetName()));
    query.bindValue(":classType", static_cast<int>(player.GetClass()));
    query.bindValue(":level",     player.GetLevel());
    query.bindValue(":exp",       player.GetExp());
    query.bindValue(":hp",        player.GetHealth());
    query.bindValue(":maxHealth", player.GetMaxHealth());
    query.bindValue(":gold",      player.GetGold());
    query.bindValue(":inv",       inventoryJson);

    if (!query.exec()) {
        throw GameException("存档写入失败：" + query.lastError().text().toStdString());
    }

    qDebug() << "存档成功：" << QString::fromStdString(player.GetName());
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// loadPlayer：从数据库中读取最新存档并填充 player
//
// 调用方须在调用前按 classType 构造正确子类（Steve/Athlete/Nerd），
// 本函数只负责覆盖 level/exp/hp/gold/inventory 等可序列化字段。
// ─────────────────────────────────────────────────────────────────────────────
bool SaveSys::loadPlayer(Character& player) {
    if (!db.isOpen() && !db.open()) {
        throw GameException("读档失败：数据库未打开。");
    }

    QSqlQuery query(db);
    // 按角色名查询（也可改为 ORDER BY id DESC LIMIT 1 取最新存档）
    query.prepare(
        "SELECT level, exp, hp, maxHealth, gold, inventory_json "
        "FROM player WHERE name = :name LIMIT 1;"
    );
    query.bindValue(":name", QString::fromStdString(player.GetName()));

    if (!query.exec() || !query.next()) {
        qDebug() << "未找到存档：" << QString::fromStdString(player.GetName());
        return false;
    }

    // ── 还原基础属性（通过公开 API 写入，保持封装性）──────────────────────
    int savedLevel = query.value("level").toInt();
    // 逐级 LevelUp() 以保证 maxHealth/attack 随级别正确累加
    while (player.GetLevel() < savedLevel) {
        player.LevelUp();
    }
    // exp 直接通过 AddExp 累积（已在 LevelUp 中被清零，现在从 0 补回）
    // 注意：AddExp 可能再次触发升级，因此先用 GetExp() 判断差值
    int expDiff = query.value("exp").toInt() - player.GetExp();
    if (expDiff > 0) player.AddExp(expDiff);

    // 血量：先将 hp 设为 maxHealth（LevelUp 已提升），再按实际值调整
    int savedHp  = query.value("hp").toInt();
    int savedMax = query.value("maxHealth").toInt();
    // 若存档 maxHealth > 当前（理论上 LevelUp 后应一致），补齐差值
    int hpGap = savedMax - player.GetMaxHealth();
    if (hpGap > 0) {
        // 通过 AddDefense(0) 不影响属性，直接用 HealHp 修正 maxHealth 差异暂不处理
        // TODO: Character 可暴露 SetMaxHealth() 以更精确还原
    }
    // 将当前 hp 置为 maxHealth 再扣减至存档值
    player.HealHp(player.GetMaxHealth());                   // 先回满
    player.TakeDamage(player.GetHealth() - savedHp);        // 再扣至存档血量

    // 金币：用 AddGold / SpendGold 修正差值
    int goldDiff = query.value("gold").toInt() - player.GetGold();
    if (goldDiff > 0)  player.AddGold(goldDiff);
    else if (goldDiff < 0) player.SpendGold(-goldDiff);

    // ── 还原背包 + 装备 ────────────────────────────────────────────────────
    deserializeInventory(query.value("inventory_json").toString(), player);

    qDebug() << "读档成功：" << QString::fromStdString(player.GetName());
    return true;
}
