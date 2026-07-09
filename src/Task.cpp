#include "Common.h"
#include "../include/Task.h"
#include "../include/Character.h"
#include "../include/Item.h"

Task::Task(int id, std::string desc, std::string npc, int gold, int exp, std::vector<std::shared_ptr<Item>> items)
    : id(id), description(desc), submitNPC(npc), status(TaskStatus::NotStarted),
      rewardGold(gold), rewardExp(exp), rewardItems(items) {}

void Task::AddObjective(TaskType type, std::string targetName, int amount) {
    objectives.push_back({type, targetName, amount, 0});
}

bool Task::IsComplete() const {
    for (const auto& obj : objectives) {
        if (!obj.isComplete()) {
            return false;
        }
    }
    return !objectives.empty();
}

void Task::UpdateKill(const std::string& enemyName) {
    if (status != TaskStatus::InProgress) return;
    for (auto& obj : objectives) {
        if (obj.type == TaskType::KillEnemy && obj.targetName == enemyName && !obj.isComplete()) {
            obj.currentAmount++;
        }
    }
}

void Task::UpdateBuy(const std::string& itemName, bool isHealItem) {
    if (status != TaskStatus::InProgress) return;
    for (auto& obj : objectives) {
        if (obj.type == TaskType::AnyBuy && !obj.isComplete()) {
            obj.currentAmount++;
        } else if (obj.type == TaskType::BuyHealItem && isHealItem && !obj.isComplete()) {
            obj.currentAmount++;
        } else if (obj.type == TaskType::BuyItem && obj.targetName == itemName && !obj.isComplete()) {
            obj.currentAmount++;
        }
    }
}

// ---------------------------------------------------------
// TaskManager
// ---------------------------------------------------------

TaskManager::TaskManager() {}

void TaskManager::InitTasks() {
    tasks.clear();

    // 1. 周老师 - 打3个校园混混
    auto t1 = std::make_shared<Task>(1, "对付3个校园混混", "周老师", 0, 30);
    t1->AddObjective(TaskType::KillEnemy, "校园混混", 3);
    tasks.push_back(t1);

    // 2. 王老师 - 对付5个逃课大神
    auto t2 = std::make_shared<Task>(2, "对付5个逃课大神", "王老师", 50, 50);
    t2->AddObjective(TaskType::KillEnemy, "逃课大神", 5);
    tasks.push_back(t2);

    // 3. 王老师 - 对付4个考试黄牛
    auto t3 = std::make_shared<Task>(3, "对付4个考试黄牛", "王老师", 40, 40);
    t3->AddObjective(TaskType::KillEnemy, "考试黄牛", 4);
    tasks.push_back(t3);

    // 4. 周老师 - 对付3个小弟弟
    auto t4 = std::make_shared<Task>(4, "对付3个小弟弟", "周老师", 0, 30);
    t4->AddObjective(TaskType::KillEnemy, "小弟弟", 3);
    tasks.push_back(t4);

    // 5. 小明 - 对付1个教导主任
    auto t5 = std::make_shared<Task>(5, "对付1个教导主任", "小明", 200, 100, std::vector<std::shared_ptr<Item>>{StatBoostItem::StrengthPotion()});
    t5->AddObjective(TaskType::KillEnemy, "教导主任", 1);
    t5->minLevel = 5;
    tasks.push_back(t5);

    // 6. 小明 - 对付1个体育委员长
    auto t6 = std::make_shared<Task>(6, "对付1个体育委员长", "小明", 250, 150, std::vector<std::shared_ptr<Item>>{StatBoostItem::StrengthPotion()});
    t6->AddObjective(TaskType::KillEnemy, "体育委员长", 1);
    t6->minLevel = 5;
    tasks.push_back(t6);

    // 7. 小明 - 对付1个校长
    auto t7 = std::make_shared<Task>(7, "对付1个校长", "小明", 300, 200, std::vector<std::shared_ptr<Item>>{CouponItem::ShopDiscountCoupon()});
    t7->AddObjective(TaskType::KillEnemy, "校长", 1);
    t7->minLevel = 5;
    tasks.push_back(t7);

    // 8. 刘老师 - 商店购买任意1件物品
    auto t8 = std::make_shared<Task>(8, "在商店中购买任意一件物品", "刘老师", 0, 20, std::vector<std::shared_ptr<Item>>{Medicine::HealingPotion()});
    t8->AddObjective(TaskType::AnyBuy, "", 1);
    tasks.push_back(t8);

    // 9. 刘老师 - 购买5个回复道具
    auto t9 = std::make_shared<Task>(9, "购买5个回复道具", "刘老师", 0, 50, std::vector<std::shared_ptr<Item>>{StatBoostItem::DefensePotion()});
    t9->AddObjective(TaskType::BuyHealItem, "", 5);
    tasks.push_back(t9);

    // 10. 小明 - 对付1个宿管阿姨
    auto t10 = std::make_shared<Task>(10, "对付1个宿管阿姨", "小明", 400, 300, std::vector<std::shared_ptr<Item>>{StatBoostItem::AllStatPotion()});
    t10->AddObjective(TaskType::KillEnemy, "宿管阿姨", 1);
    t10->minLevel = 5;
    tasks.push_back(t10);

    // 11. 小明 - 对付5个午夜卷王幽灵
    auto t11 = std::make_shared<Task>(11, "对付5个午夜卷王幽灵", "小明", 0, 100, std::vector<std::shared_ptr<Item>>{Medicine::HealingPotion(), StatBoostItem::DefensePotion()});
    t11->AddObjective(TaskType::KillEnemy, "午夜卷王幽灵", 5);
    tasks.push_back(t11);

    // 12. 小华 - 对付3个树林野兽
    auto t12 = std::make_shared<Task>(12, "对付3个树林野兽", "小华", 0, 40);
    t12->AddObjective(TaskType::KillEnemy, "树林野兽", 3);
    tasks.push_back(t12);

    // 13. 小华 - 对付3个幽暗黑影
    auto t13 = std::make_shared<Task>(13, "对付3个幽暗黑影", "小华", 60, 50);
    t13->AddObjective(TaskType::KillEnemy, "幽暗黑影", 3);
    tasks.push_back(t13);

    // 14. 小王 - 对付1个树林霸主
    auto t14 = std::make_shared<Task>(14, "对付1个树林霸主", "小王", 250, 150, std::vector<std::shared_ptr<Item>>{StatBoostItem::StrengthPotion()});
    t14->AddObjective(TaskType::KillEnemy, "树林霸主", 1);
    tasks.push_back(t14);

    // 15. 商人 - 购买1个铁甲
    auto t15 = std::make_shared<Task>(15, "购买1件铁甲", "商人", 0, 50, std::vector<std::shared_ptr<Item>>{StatBoostItem::DefensePotion()});
    t15->AddObjective(TaskType::BuyItem, "铁甲", 1);
    tasks.push_back(t15);

    // 16. 刘老师 - 对付3个校园混混 和 3个逃课大神
    auto t16 = std::make_shared<Task>(16, "对付3个校园混混和3个逃课大神", "刘老师", 100, 80, std::vector<std::shared_ptr<Item>>{StatBoostItem::StrengthPotion()});
    t16->AddObjective(TaskType::KillEnemy, "校园混混", 3);
    t16->AddObjective(TaskType::KillEnemy, "逃课大神", 3);
    tasks.push_back(t16);

    // 17. 周老师 - 对付2个考试黄牛 和 2个小弟弟
    auto t17 = std::make_shared<Task>(17, "对付2个考试黄牛和2个小弟弟", "周老师", 100, 80, std::vector<std::shared_ptr<Item>>{StatBoostItem::DefensePotion()});
    t17->AddObjective(TaskType::KillEnemy, "考试黄牛", 2);
    t17->AddObjective(TaskType::KillEnemy, "小弟弟", 2);
    tasks.push_back(t17);

    // 18. 小王 - 购买3个回复道具 和 购买1个铁剑
    auto t18 = std::make_shared<Task>(18, "购买3个回复道具和1把铁剑", "小王", 0, 100, std::vector<std::shared_ptr<Item>>{StatBoostItem::AllStatPotion()});
    t18->AddObjective(TaskType::BuyHealItem, "", 3);
    t18->AddObjective(TaskType::BuyItem, "铁剑", 1);
    tasks.push_back(t18);

    // 19. 商人 - 对付5个午夜卷王幽灵 和 5个幽暗黑影
    auto t19 = std::make_shared<Task>(19, "对付5个午夜卷王幽灵和5个幽暗黑影", "商人", 500, 200, std::vector<std::shared_ptr<Item>>{CouponItem::ShopDiscountCoupon()});
    t19->AddObjective(TaskType::KillEnemy, "午夜卷王幽灵", 5);
    t19->AddObjective(TaskType::KillEnemy, "幽暗黑影", 5);
    tasks.push_back(t19);

    // 20. 小明 - 对付 教导主任，体育委员长，校长 各1个
    auto t20 = std::make_shared<Task>(20, "击败校园三大巨头（教导主任、体育委员长、校长）", "小明", 1000, 500, std::vector<std::shared_ptr<Item>>{StatBoostItem::AllStatPotion()});
    t20->AddObjective(TaskType::KillEnemy, "教导主任", 1);
    t20->AddObjective(TaskType::KillEnemy, "体育委员长", 1);
    t20->AddObjective(TaskType::KillEnemy, "校长", 1);
    t20->minLevel = 5;
    tasks.push_back(t20);

    // Note: Rewards will be populated later in InitTasks() with item instances.
    // However, since items are dynamically generated, it's better to assign rewards right now.
}

void TaskManager::OnEnemyKilled(const std::string& enemyName) {
    for (auto& t : tasks) {
        if (t->status == TaskStatus::InProgress) {
            t->UpdateKill(enemyName);
            if (t->IsComplete()) {
                t->status = TaskStatus::Completed;
            }
        }
    }
}

void TaskManager::OnItemBought(const std::string& itemName, bool isHealItem) {
    for (auto& t : tasks) {
        if (t->status == TaskStatus::InProgress) {
            t->UpdateBuy(itemName, isHealItem);
            if (t->IsComplete()) {
                t->status = TaskStatus::Completed;
            }
        }
    }
}

std::string TaskManager::GetNPCInteractionText(const std::string& npcName, Character* player) {
    std::stringstream ss;
    bool foundAny = false;

    for (auto& t : tasks) {
        if (t->submitNPC == npcName) {
            if (t->status == TaskStatus::NotStarted) {
                if (player->GetLevel() < t->minLevel) continue; // 等级不够，NPC 暂时不给这个任务
                t->status = TaskStatus::InProgress;
                ss << "【接取任务】" << t->description << "\n";
                foundAny = true;
            } else if (t->status == TaskStatus::InProgress) {
                ss << "【进行中】" << t->description << " (尚未完成)\n";
                foundAny = true;
            } else if (t->status == TaskStatus::Completed) {
                t->status = TaskStatus::Submitted;
                ss << "【完成任务】" << t->description << "\n";
                if (t->rewardGold > 0) {
                    player->AddGold(t->rewardGold);
                    ss << "获得金币: " << t->rewardGold << "\n";
                }
                if (t->rewardExp > 0) {
                    bool leveled = player->AddExp(t->rewardExp);
                    ss << "获得经验: " << t->rewardExp << "\n";
                    if (leveled) ss << "🌟 升级了！\n";
                }
                for (auto& item : t->rewardItems) {
                    player->GetBackpack().AddItem(item->Clone());
                    ss << "获得物品: " << item->getName() << "\n";
                }
                foundAny = true;
            }
        }
    }

    if (!foundAny) {
        ss << "【" << npcName << "】：「同学，好好学习，天天向上！」\n";
    }

    return ss.str();
}

std::string TaskManager::SerializeTasks() const {
    QJsonArray taskArray;
    for (const auto& task : tasks) {
        if (task->status == TaskStatus::NotStarted) continue; // 未接取的不用存
        QJsonObject taskObj;
        taskObj["id"] = task->id;
        taskObj["status"] = static_cast<int>(task->status);
        QJsonArray objArray;
        for (const auto& obj : task->objectives) {
            objArray.append(obj.currentAmount);
        }
        taskObj["objectives"] = objArray;
        taskArray.append(taskObj);
    }
    return QString(QJsonDocument(taskArray).toJson(QJsonDocument::Compact)).toStdString();
}

void TaskManager::DeserializeTasks(const std::string& jsonStr) {
    if (jsonStr.empty()) return;
    QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(jsonStr).toUtf8());
    if (!doc.isArray()) return;
    QJsonArray taskArray = doc.array();
    
    for (int i = 0; i < taskArray.size(); ++i) {
        QJsonObject taskObj = taskArray[i].toObject();
        int id = taskObj["id"].toInt();
        for (auto& task : tasks) {
            if (task->id == id) {
                task->status = static_cast<TaskStatus>(taskObj["status"].toInt());
                QJsonArray objArray = taskObj["objectives"].toArray();
                for (int j = 0; j < objArray.size() && j < task->objectives.size(); ++j) {
                    task->objectives[j].currentAmount = objArray[j].toInt();
                }
                break;
            }
        }
    }
}
