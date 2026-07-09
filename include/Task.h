#pragma once
#include "Common.h"
#include "Item.h"

class Character;

enum class TaskStatus {
    NotStarted,
    InProgress,
    Completed,
    Submitted
};

enum class TaskType {
    KillEnemy,
    BuyItem,
    BuyHealItem,
    AnyBuy
};

struct TaskObjective {
    TaskType type;
    std::string targetName; // Enemy name, or item name. Empty if AnyBuy/BuyHealItem
    int targetAmount;
    int currentAmount;

    bool isComplete() const {
        return currentAmount >= targetAmount;
    }
};

class Task {
public:
    int id;
    std::string description;
    std::string submitNPC;
    TaskStatus status;
    int minLevel = 1;

    std::vector<TaskObjective> objectives;

    int rewardGold;
    int rewardExp;
    std::vector<std::shared_ptr<Item>> rewardItems;

    Task(int id, std::string desc, std::string npc, int gold, int exp, std::vector<std::shared_ptr<Item>> items = {});
    
    void AddObjective(TaskType type, std::string targetName, int amount);

    bool IsComplete() const;
    void UpdateKill(const std::string& enemyName);
    void UpdateBuy(const std::string& itemName, bool isHealItem);
};

class TaskManager {
private:
    std::vector<std::shared_ptr<Task>> tasks;
public:
    TaskManager();
    void InitTasks();

    std::string SerializeTasks() const;
    void DeserializeTasks(const std::string& jsonStr);

    void OnEnemyKilled(const std::string& enemyName);
    void OnItemBought(const std::string& itemName, bool isHealItem);

    // Returns a message if any interaction happens
    std::string GetNPCInteractionText(const std::string& npcName, Character* player);

    const std::vector<std::shared_ptr<Task>>& GetTasks() const { return tasks; }
};
