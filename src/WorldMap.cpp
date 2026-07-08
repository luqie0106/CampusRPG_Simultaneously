#include "Common.h"

#include "../include/WorldMap.h"

// ─────────────────────────────────────────────────────────────────────────────
// InteractableInfo 便捷工厂实现
// ─────────────────────────────────────────────────────────────────────────────

InteractableInfo InteractableInfo::MakeEnemy(int id, GamePoint pos,
                                              const std::string& displayName,
                                              Enemy enemyTempl) {
    InteractableInfo info;
    info.id                 = id;
    info.type               = InteractableType::Enemy;
    info.defaultInteraction = InteractionType::StartBattle;
    info.pos                = pos;
    info.displayName        = displayName;
    info.enemyTemplate      = std::move(enemyTempl);
    return info;
}

InteractableInfo InteractableInfo::MakeShop(int id, GamePoint pos,
                                             const std::string& displayName) {
    InteractableInfo info;
    info.id                 = id;
    info.type               = InteractableType::Shop;
    info.defaultInteraction = InteractionType::EnterShop;
    info.pos                = pos;
    info.displayName        = displayName;
    // enemyTemplate 默认为 std::nullopt
    return info;
}

InteractableInfo InteractableInfo::MakeNPC(int id, GamePoint pos,
                                            const std::string& displayName) {
    InteractableInfo info;
    info.id                 = id;
    info.type               = InteractableType::NPC;
    info.defaultInteraction = InteractionType::TalkToNPC;
    info.pos                = pos;
    info.displayName        = displayName;
    return info;
}

InteractableInfo InteractableInfo::MakeBlackMarket(int id, GamePoint pos,
                                                    const std::string& displayName) {
    InteractableInfo info;
    info.id                 = id;
    info.type               = InteractableType::BlackMarket;
    info.defaultInteraction = InteractionType::EnterBlackMarket;
    info.pos                = pos;
    info.displayName        = displayName;
    // enemyTemplate 默认为 std::nullopt
    return info;
}

InteractableInfo InteractableInfo::MakeDoor(int id, GamePoint pos,
                                            const std::string& displayName,
                                            const std::string& tgtMap,
                                            int tgtX, int tgtY) {
    InteractableInfo info;
    info.id                 = id;
    info.type               = InteractableType::Door;
    info.defaultInteraction = InteractionType::TeleportMap;
    info.pos                = pos;
    info.displayName        = displayName;
    info.targetMap          = tgtMap;
    info.targetX            = tgtX;
    info.targetY            = tgtY;
    return info;
}

InteractableInfo InteractableInfo::MakeGroundItem(int id, GamePoint pos,
                                                  const std::string& displayName,
                                                  std::shared_ptr<Item> item) {
    InteractableInfo info;
    info.id                 = id;
    info.type               = InteractableType::GroundItem;
    info.defaultInteraction = InteractionType::PickUpItem;
    info.pos                = pos;
    info.displayName        = displayName;
    info.itemData           = std::move(item);
    return info;
}

// ─────────────────────────────────────────────────────────────────────────────
// WorldMap 构造与出生点管理
// ─────────────────────────────────────────────────────────────────────────────

WorldMap::WorldMap(int mapWidth, int mapHeight, GamePoint spawnPoint)
    : m_mapSystem(mapWidth, mapHeight),
      m_playerPos(spawnPoint),
      m_spawnPoint(spawnPoint) {}

void WorldMap::SetSpawnPoint(GamePoint point) {
    m_spawnPoint = point;
}

GamePoint WorldMap::GetSpawnPoint() const {
    return m_spawnPoint;
}

void WorldMap::ResetPlayerToSpawn() {
    m_playerPos = m_spawnPoint;
}

void WorldMap::SetPlayerPos(GamePoint point) {
    m_playerPos = point;
}

// ─────────────────────────────────────────────────────────────────────────────
// 地图数据访问
// ─────────────────────────────────────────────────────────────────────────────

MapSystem& WorldMap::GetMapSystem() {
    return m_mapSystem;
}

const MapSystem& WorldMap::GetMapSystem() const {
    return m_mapSystem;
}

void WorldMap::InitMapSize(int width, int height) {
    m_mapSystem.InitMapSize(width, height);
}

// ─────────────────────────────────────────────────────────────────────────────
// 实体注册 / 注销
// ─────────────────────────────────────────────────────────────────────────────

void WorldMap::AddInteractable(InteractableInfo info) {
    std::lock_guard<std::mutex> lock(m_mapMutex);
    m_interactables.push_back(std::move(info));
}

void WorldMap::RemoveInteractable(int id) {
    std::lock_guard<std::mutex> lock(m_mapMutex);
    m_interactables.erase(
        std::remove_if(m_interactables.begin(), m_interactables.end(),
                       [id](const InteractableInfo& e) { return e.id == id; }),
        m_interactables.end());
}

void WorldMap::MarkInteractableDead(int id, int currentMinutes) {
    std::lock_guard<std::mutex> lock(m_mapMutex);
    for (auto& e : m_interactables) {
        if (e.id == id) {
            e.isDead = true;
            e.deadTimeMinutes = currentMinutes;
            break;
        }
    }
}

void WorldMap::RespawnDeadMonsters(int currentMinutes) {
    std::lock_guard<std::mutex> lock(m_mapMutex);
    for (auto& e : m_interactables) {
        // 只有怪物和 Boss 才会重生
        if (e.isDead && (e.type == InteractableType::Enemy || e.type == InteractableType::Boss)) {
            // 3个游戏小时 = 180 分钟
            if (currentMinutes - e.deadTimeMinutes >= 180) {
                e.isDead = false;
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// 玩家移动（含碰撞检测）
// ─────────────────────────────────────────────────────────────────────────────

bool WorldMap::MovePlayer(int dx, int dy) {
    int newX = m_playerPos.x + dx;
    int newY = m_playerPos.y + dy;

    if (!m_mapSystem.isWalkable(newX, newY)) {
        return false;   // 目标格不可行走（越界 / 障碍物）
    }

    m_playerPos = GamePoint(newX, newY);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// 纯查询接口
// ─────────────────────────────────────────────────────────────────────────────

GamePoint WorldMap::GetPlayerPos() const {
    return m_playerPos;
}

std::vector<MapTileInfo> WorldMap::GetVisibleMap(int halfRadius) const {
    std::vector<MapTileInfo> tiles;
    tiles.reserve(static_cast<std::size_t>((2 * halfRadius + 1) * (2 * halfRadius + 1)));

    int cx = m_playerPos.x;
    int cy = m_playerPos.y;

    // 按行优先顺序：dy 外层（上→下），dx 内层（左→右）
    std::lock_guard<std::mutex> lock(m_mapMutex);
    for (int dy = -halfRadius; dy <= halfRadius; ++dy) {
        for (int dx = -halfRadius; dx <= halfRadius; ++dx) {
            int tx = cx + dx;
            int ty = cy + dy;

            MapTileInfo tile;
            tile.pos             = GamePoint(tx, ty);
            tile.tileType        = m_mapSystem.isWalkable(tx, ty) ? 0 : 1;
            tile.hasInteractable = false;
            tile.interactableId  = -1;

            for (const auto& entity : m_interactables) {
                if (entity.pos.x == tx && entity.pos.y == ty) {
                    tile.hasInteractable = true;
                    tile.interactableId  = entity.id;
                    break;
                }
            }

            tiles.push_back(tile);
        }
    }

    return tiles;
}

std::vector<InteractableInfo> WorldMap::CheckNearbyInteractables(int radius) const {
    std::lock_guard<std::mutex> lock(m_mapMutex);
    std::vector<InteractableInfo> result;
    int px = m_playerPos.x;
    int py = m_playerPos.y;

    for (const auto& entity : m_interactables) {
        if (entity.isDead) continue; // 过滤假死状态的实体
        int dist = std::abs(entity.pos.x - px) + std::abs(entity.pos.y - py);
        if (dist <= radius) {
            result.push_back(entity);
        }
    }

    return result;
}

const InteractableInfo* WorldMap::GetInteractableById(int id) const {
    std::lock_guard<std::mutex> lock(m_mapMutex);
    for (const auto& entity : m_interactables) {
        if (entity.id == id) {
            return &entity;
        }
    }
    return nullptr;
}
