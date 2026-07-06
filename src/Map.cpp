#include "Common.h"

#include "../include/Map.h"

GamePoint::GamePoint() : x(0), y(0) {}
GamePoint::GamePoint(int px, int py) : x(px), y(py) {}

bool GamePoint::operator==(const GamePoint& other) const {
    return x == other.x && y == other.y;
}
bool GamePoint::operator!=(const GamePoint& other) const {
    return !(*this == other);
}

AStarNode::AStarNode() : g(0), h(0), parent(nullptr) {}

int AStarNode::f() const { return g + h; }

bool AStarNode::operator>(const AStarNode& other) const {
    if (f() == other.f()) return h > other.h;
    return f() > other.f();
}

bool CompareNodePtr::operator()(const AStarNode* lhs, const AStarNode* rhs) const {
    return *lhs > *rhs;
}

int MapSystem::calculateH(const GamePoint& p1, const GamePoint& p2) {
    return std::abs(p1.x - p2.x) + std::abs(p1.y - p2.y);
}

bool MapSystem::isValid(int x, int y) {
    return (x >= 0 && x < mapWidth && y >= 0 && y < mapHeight && mapData[y][x] == 0);
}

MapSystem::MapSystem(int width, int height) : mapWidth(width), mapHeight(height) {
    mapData.resize(mapHeight, std::vector<int>(mapWidth, 0));
}


void MapSystem::InitMapSize(int width, int height) {
    mapWidth  = width;
    mapHeight = height;
    // 全部初始化为障碍(1)。
    // QtMapLoader 随后对子地图内的每个格子：
    //   - obstruction 层为空 (tileId==0) → setTile(x,y,0) 开通
    //   - obstruction 层非空            → 保持 1（屏蔽）
    //   - 子地图未覆盖的空隙区域   → 保持 1（天然刑刀）
    mapData.assign(mapHeight, std::vector<int>(mapWidth, 1));
}

void MapSystem::setTile(int x, int y, int type) {
    if (x >= 0 && x < mapWidth && y >= 0 && y < mapHeight) {
        mapData[y][x] = type;
    }
}

bool MapSystem::isWalkable(int tileX, int tileY) const {
    if (tileX < 0 || tileX >= mapWidth || tileY < 0 || tileY >= mapHeight) return false;
    return mapData[tileY][tileX] == 0;
}

std::vector<GamePoint> MapSystem::findPath(GamePoint start, GamePoint end) {
    std::vector<GamePoint> path;

    // 边界情况判断
    if (!isValid(start.x, start.y) || !isValid(end.x, end.y)) {
        return path; 
    }
    if (start == end) {
        path.push_back(start);
        return path;
    }

    // 用 unique_ptr 智能指针管理所有临时生成的节点内存，防止内存泄漏
    std::vector<std::unique_ptr<AStarNode>> nodePool;

    // 🌟 完美的 STL 优先队列（Open List）
    std::priority_queue<AStarNode*, std::vector<AStarNode*>, CompareNodePtr> openList;
    
    // 🌟 纯 STL 标记数组（Close List 和 快速 G 值查询）
    std::vector<std::vector<bool>> closedList(mapHeight, std::vector<bool>(mapWidth, false));
    std::vector<std::vector<int>> gScore(mapHeight, std::vector<int>(mapWidth, 1e9)); // 初始化为无穷大

    // 1. 创建起点节点并推入 Open List
    auto startNode = std::make_unique<AStarNode>();
    startNode->pos = start;
    startNode->g = 0;
    startNode->h = calculateH(start, end);
    startNode->parent = nullptr;
    
    gScore[start.y][start.x] = 0;
    openList.push(startNode.get());
    nodePool.push_back(std::move(startNode));

    // 上下左右 四方向移动偏量
    const std::vector<GamePoint> directions = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} };
    bool foundEnd = false;
    AStarNode* endNodePtr = nullptr;

    // 2. A* 主循环开始
    while (!openList.empty()) {
        // 取出 F 值最小的节点
        AStarNode* current = openList.top();
        openList.pop();

        GamePoint curPos = current->pos;

        // 如果该节点已经被封闭了，直接跳过
        if (closedList[curPos.y][curPos.x]) continue;
        
        // 移入 Close List
        closedList[curPos.y][curPos.x] = true;

        // 检查是否到达终点
        if (curPos == end) {
            foundEnd = true;
            endNodePtr = current;
            break;
        }

        // 3. 遍历 4 个方向的邻居
        for (const auto& dir : directions) {
            int nextX = curPos.x + dir.x;
            int nextY = curPos.y + dir.y;

            // 邻居必须合法且未在 Close List 中
            if (isValid(nextX, nextY) && !closedList[nextY][nextX]) {
                int tentativeG = current->g + 1; // 假设移动代价为 1

                // 如果找到了更短的到达该格子的路径
                if (tentativeG < gScore[nextY][nextX]) {
                    gScore[nextY][nextX] = tentativeG;

                    // 创建新邻居节点
                    auto neighbor = std::make_unique<AStarNode>();
                    neighbor->pos = GamePoint(nextX, nextY);
                    neighbor->g = tentativeG;
                    neighbor->h = calculateH(neighbor->pos, end);
                    neighbor->parent = current;

                    openList.push(neighbor.get());
                    nodePool.push_back(std::move(neighbor));
                }
            }
        }
    }

    // 4. 倒推还原路径
    if (foundEnd && endNodePtr != nullptr) {
        AStarNode* curr = endNodePtr;
        while (curr != nullptr) {
            path.push_back(curr->pos);
            curr = curr->parent;
        }
        // 使用 STL 的 reverse 翻转路径（因为倒推出来是 终点->起点）
        std::reverse(path.begin(), path.end());
    }

    // nodePool 离开生命周期，智能指针会自动析构并释放全部 AStarNode 内存！
    return path; 
}
