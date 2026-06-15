#pragma once

#include <vector>
#include <queue>
#include <cmath>
#include <algorithm>

// 1. 用纯 C++ 结构体替代 QPoint
struct GamePoint {
    int x = 0;
    int y = 0;

    GamePoint() = default;
    GamePoint(int px, int py) : x(px), y(py) {}

    // 重载等于号，方便后续判断是否到达终点
    bool operator==(const GamePoint& other) const {
        return x == other.x && y == other.y;
    }
};

// 2. A* 算法专用的节点结构体
struct AStarNode {
    GamePoint pos;
    int g = 0;          // 从起点到当前格子的实际代价
    int h = 0;          // 从当前格子到终点的启发式预估代价
    AStarNode* parent = nullptr; 

    int f() const { return g + h; }

    // 优先队列排序规则：F 值越小越优先
    bool operator>(const AStarNode& other) const {
        if (f() == other.f()) return h > other.h;
        return f() > other.f();
    }
};

class MapSystem {
private:
    int mapWidth;
    int mapHeight;
    
    // 🌟 用标准库的嵌套 vector 替代 QVector<QVector<int>>
    std::vector<std::vector<int>> mapData; 

    // A* 算法的曼哈顿距离计算（H 值）
    int calculateH(const GamePoint& p1, const GamePoint& p2) {
        return std::abs(p1.x - p2.x) + std::abs(p1.y - p2.y);
    }

    bool isValid(int x, int y) {
        return (x >= 0 && x < mapWidth && y >= 0 && y < mapHeight && mapData[y][x] == 0);
    }

public:
    MapSystem(int width = 50, int height = 50) : mapWidth(width), mapHeight(height) {
        // 🌟 纯 STL 方式初始化二维动态数组
        mapData.resize(mapHeight, std::vector<int>(mapWidth, 0));
        
        // 组长可以手动刷几堵墙进行初步测试
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
    // 🌟 纯 STL 版 A* 寻路算法接口
    // ========================================================
    std::vector<GamePoint> findPath(GamePoint start, GamePoint end) {
        std::vector<GamePoint> path;

        if (!isValid(start.x, start.y) || !isValid(end.x, end.y)) {
            return path; 
        }

        // 四方向移动偏量
        const std::vector<GamePoint> directions = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} };

        // 🌟 完美的 STL 优先队列（Open List）
        std::priority_queue<AStarNode*, std::vector<AStarNode*>, std::greater<AStarNode*>> openList;
        
        // 🌟 纯 STL 的 Close List 标记二维数组
        std::vector<std::vector<bool>> closedList(mapHeight, std::vector<bool>(mapWidth, false));

        // ----------------------------------------------------
        // 留给算法组员补全的 A* 核心循环核心点：
        // openList.push(new AStarNode{start, 0, calculateH(start, end), nullptr});
        // while(!openList.empty()) { ... }
        // ----------------------------------------------------

        // 临时兜底：默认直线路径（确保游戏不崩溃）
        path.push_back(start);
        path.push_back(end);
        
        return path; 
    }
};