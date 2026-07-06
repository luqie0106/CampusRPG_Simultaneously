#pragma once

#include "Common.h"

// 1. 纯 C++ 结构体替代 QPoint
struct GamePoint {
    int x;
    int y;

    GamePoint();
    GamePoint(int px, int py);

    bool operator==(const GamePoint& other) const;
    bool operator!=(const GamePoint& other) const;
};

// 2. A* 算法专用的节点结构体
struct AStarNode {
    GamePoint pos;
    int g;
    int h;
    AStarNode* parent;

    AStarNode();

    int f() const;

    // 优先队列排序：F 值越小越优先。F 相等时，H 越小越优先
    bool operator>(const AStarNode& other) const;
};

// 辅助结构体：用来在优先队列里比较节点的指针
struct CompareNodePtr {
    bool operator()(const AStarNode* lhs, const AStarNode* rhs) const;
};

class MapSystem {
private:
    int mapWidth;
    int mapHeight;
    std::vector<std::vector<int>> mapData; // 0代表空地，1代表障碍物

    // A* 算法的曼哈顿距离计算（H 值）
    int calculateH(const GamePoint& p1, const GamePoint& p2);

    // 检查某个格子是否在地图合法范围内且不是障碍物
    bool isValid(int x, int y);

public:
    MapSystem(int width = 50, int height = 50);

    // 重置地图尺寸并将所有格子初始化为障碍(1)。
    // 由 QtMapLoader::LoadWorldToScene 在首次解析时调用；
    // 随后 setTile(x,y,0) 按 obstruction 层逐格标记可通行区域。
    void InitMapSize(int width, int height);

    void setTile(int x, int y, int type);

    bool isWalkable(int tileX, int tileY) const;

    // ========================================================
    // 🌟 纯 STL 版 A* 寻路算法（完整实现）
    // ========================================================
    std::vector<GamePoint> findPath(GamePoint start, GamePoint end);
};
