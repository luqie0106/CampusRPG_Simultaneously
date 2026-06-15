#pragma once

#include <vector>
#include <queue>
#include <cmath>
#include <algorithm>
#include <memory>    // 引入智能指针

// 1. 纯 C++ 结构体替代 QPoint
struct GamePoint {
    int x = 0;
    int y = 0;

    GamePoint() = default;
    GamePoint(int px, int py) : x(px), y(py) {}

    bool operator==(const GamePoint& other) const {
        return x == other.x && y == other.y;
    }
    bool operator!=(const GamePoint& other) const {
        return !(*this == other);
    }
};

// 2. A* 算法专用的节点结构体
struct AStarNode {
    GamePoint pos;
    int g = 0; // 从起点到当前格子的实际代价
    int h = 0; // 从当前格子到终点的启发式预估代价
    AStarNode* parent = nullptr; 

    int f() const { return g + h; }

    // 优先队列排序：F 值越小越优先。F 相等时，H 越小越优先
    bool operator>(const AStarNode& other) const {
        if (f() == other.f()) return h > other.h;
        return f() > other.f();
    }
};

// 辅助结构体：用来在优先队列里比较节点的指针
struct CompareNodePtr {
    bool operator()(const AStarNode* lhs, const AStarNode* rhs) const {
        return *lhs > *rhs;
    }
};

class MapSystem {
private:
    int mapWidth;
    int mapHeight;
    std::vector<std::vector<int>> mapData; // 0代表空地，1代表障碍物

    // A* 算法的曼哈顿距离计算（H 值）
    int calculateH(const GamePoint& p1, const GamePoint& p2) {
        return std::abs(p1.x - p2.x) + std::abs(p1.y - p2.y);
    }

    // 检查某个格子是否在地图合法范围内且不是障碍物
    bool isValid(int x, int y) {
        return (x >= 0 && x < mapWidth && y >= 0 && y < mapHeight && mapData[y][x] == 0);
    }

public:
    MapSystem(int width = 50, int height = 50) : mapWidth(width), mapHeight(height) {
        mapData.resize(mapHeight, std::vector<int>(mapWidth, 0));
        
        // 组长保留的测试墙（第10行，第5到20列是墙）
        for(int x = 5; x < 20; ++x) {
            mapData[10][x] = 1; 
        }
    }

    void setTile(int x, int y, int type) {
        if (x >= 0 && x < mapWidth && y >= 0 && y < mapHeight) {
            mapData[y][x] = type;
        }
    }

    bool isWalkable(int tileX, int tileY) {
        if (tileX < 0 || tileX >= mapWidth || tileY < 0 || tileY >= mapHeight) return false;
        return mapData[tileY][tileX] == 0;
    }

    // ========================================================
    // 🌟 纯 STL 版 A* 寻路算法（完整实现）
    // ========================================================
    std::vector<GamePoint> findPath(GamePoint start, GamePoint end) {
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
};
