#pragma once

#include "Common.h"
#include "Map.h"
#include "Enemy.h"
// ─────────────────────────────────────────────────────────────────────────────
// 地图上可交互实体的类型
// ─────────────────────────────────────────────────────────────────────────────
enum class InteractableType {
    Enemy,       // 普通怪物
    Boss,        // Boss（固定点位，破韧机制启用）
    Shop,        // 商店建筑
    BlackMarket, // 黑市商人（夜晚随机生成）
    NPC,         // 任务 NPC（对话/接取任务，任务系统预留）
    Door,        // 门/传送点（走上自动触发切场退，当前预留）
};

// ─────────────────────────────────────────────────────────────────────────────
// 玩家主动触发的交互动作类型
// UI 层确认交互后，将此枚举连同 targetId 传入 ExecuteInteraction()
// ─────────────────────────────────────────────────────────────────────────────
enum class InteractionType {
    StartBattle,      // 进入战斗（Enemy / Boss 点位）
    EnterShop,        // 进入普通商店
    EnterBlackMarket, // 进入黑市（夜晚专属）
    TalkToNPC,        // 与 NPC 对话（任务系统 TODO，暂留桩）
    TeleportMap,      // 传送到目标地图（当前地图全系拼接大图，预留供未来扩展）
};

// ─────────────────────────────────────────────────────────────────────────────
// 地图上的可交互实体描述
//
// 使用方式（由做地图的同学调用）：
//   engine.GetWorldMap().AddInteractable(
//       InteractableInfo::MakeEnemy(1, {10, 15}, "校园混混", Enemy::Bully()));
//
// id 由调用者保证唯一。
// ─────────────────────────────────────────────────────────────────────────────
struct InteractableInfo {
    int              id;                  // 全局唯一 ID（由调用者指定）
    InteractableType type;                // 实体类型
    InteractionType  defaultInteraction;  // 玩家点击按钮时触发的默认动作
    GamePoint        pos;                 // 世界格子坐标
    std::string      displayName;         // UI 弹出按钮显示的名称

    // 仅 type == Enemy / Boss 时有效：战斗触发时复制给引擎的敌人模板。
    // 地图同学通过 MakeEnemy() 工厂传入，WorldMap 不修改此字段。
    std::optional<Enemy> enemyTemplate;

    // 仅 type == Door 时有效：目标地图名称和传送坐标。
    // 当前全局地图已是大拼图，暂时预留供未来扩展多层地图时使用。
    std::string targetMap;               // 目标地图名（相对于 maps/ 目录的文件名）
    int         targetX = 0;             // 传送目标坐标 X（世界格子）
    int         targetY = 0;             // 传送目标坐标 Y（世界格子）

    // ── 便捷工厂方法 ─────────────────────────────────────────────────
    // 怪物/Boss 点位：传入 Enemy 工厂生成的实例作为模板
    static InteractableInfo MakeEnemy(int id, GamePoint pos,
                                      const std::string& displayName,
                                      Enemy enemyTempl);

    // 商店点位
    static InteractableInfo MakeShop(int id, GamePoint pos,
                                     const std::string& displayName);

    // NPC 点位（任务系统预留）
    static InteractableInfo MakeNPC(int id, GamePoint pos,
                                    const std::string& displayName);

    // 黑市商人点位
    static InteractableInfo MakeBlackMarket(int id, GamePoint pos,
                                            const std::string& displayName);

    // 门/传送点（当前地图已是大拼图，预留供未来扩展使用）
    // targetMap: 目标地图文件名（相对于 maps/），targetX/Y: 目标世界格子坐标
    static InteractableInfo MakeDoor(int id, GamePoint pos,
                                     const std::string& displayName,
                                     const std::string& targetMap,
                                     int targetX, int targetY);
};

// ─────────────────────────────────────────────────────────────────────────────
// GetVisibleMap() 返回的单格信息
// Qt 渲染地图瓦片时使用
// ─────────────────────────────────────────────────────────────────────────────
struct MapTileInfo {
    GamePoint pos;             // 该格世界坐标
    int       tileType;        // 0=空地  1=障碍物（直接来自 MapSystem）
    bool      hasInteractable; // 该格是否存在可交互实体
    int       interactableId;  // hasInteractable==true 时的实体 id，否则为 -1
};

// ─────────────────────────────────────────────────────────────────────────────
// WorldMap：世界实体管理器
//
// 职责：
//   1. 持有 MapSystem（地图可行走性数据，地图同学通过 GetMapSystem() 设置障碍）
//   2. 维护玩家当前世界坐标
//   3. 存储所有可交互实体（怪物 / 商店 / NPC 点位）
//   4. 提供查询接口，由 GameEngine 封装后暴露给 Qt UI
//
// 注意：WorldMap 不主动触发任何游戏逻辑（战斗 / 商店切换），
//       所有状态机操作均由 GameEngine 完成。
// ─────────────────────────────────────────────────────────────────────────────
class WorldMap {
public:
    // 构造：地图尺寸 + 玩家出生坐标
    // 出生坐标默认 (25, 25)，后期可通过 SetSpawnPoint() 调整
    WorldMap(int mapWidth  = 50,
             int mapHeight = 50,
             GamePoint spawnPoint = GamePoint(0, 0));  // 现设为(0,0)，地图设计完成后再调整

    // ── 出生点管理 ────────────────────────────────────────────────────
    // 允许后期（如读档后、关卡切换后）重新设定出生坐标
    void      SetSpawnPoint(GamePoint point);
    GamePoint GetSpawnPoint() const;

    // 将玩家坐标重置到当前出生点（新游戏开始 / 死亡复活后可调用）
    void ResetPlayerToSpawn();

    // ── 地图数据访问 ──────────────────────────────────────────────────
    // 地图同学通过此方法拿到 MapSystem，调用 setTile() 布置障碍 / 地形
    MapSystem& GetMapSystem();
    const MapSystem& GetMapSystem() const;

    // 重置地图尺寸（全障碍初始化），供 QtMapLoader::LoadWorldToScene 调用
    // QtMapLoader 随后按 obstruction 层逐格调用 GetMapSystem().setTile() 开通可行走区域
    void InitMapSize(int width, int height);

    // ── 实体注册 / 注销 ───────────────────────────────────────────────
    // 地图同学在地图初始化时调用 AddInteractable() 注册点位
    void AddInteractable(InteractableInfo info);

    // 战斗胜利 / 实体失效后移除（由 GameEngine 在 ExecuteInteraction 内调用）
    void RemoveInteractable(int id);

    // ── 玩家移动（含碰撞检测）────────────────────────────────────────
    // dx / dy 取值 -1、0、+1；仅四方向移动
    // 目标格不可行走时返回 false，不修改坐标
    bool MovePlayer(int dx, int dy);

    // ── 纯查询接口（供 GameEngine 封装后暴露给 Qt）───────────────────
    GamePoint GetPlayerPos() const;

    // 以玩家为中心、上下左右各扩展 halfRadius 格
    // 返回 (2*halfRadius+1)^2 个格子的信息，按行优先顺序排列（dy 外层，dx 内层）
    std::vector<MapTileInfo> GetVisibleMap(int halfRadius = 2) const;

    // 返回与玩家曼哈顿距离 ≤ radius 的全部可交互实体列表
    std::vector<InteractableInfo> CheckNearbyInteractables(int radius = 1) const;

    // 按 id 查找实体；找不到返回 nullptr
    const InteractableInfo* GetInteractableById(int id) const;

private:
    MapSystem                     m_mapSystem;
    GamePoint                     m_playerPos;
    GamePoint                     m_spawnPoint;
    std::vector<InteractableInfo> m_interactables;
    mutable std::mutex            m_mapMutex;  // 保护 m_interactables 的多线程并发读写
};
