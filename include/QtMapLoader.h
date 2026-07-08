#pragma once

#include "Common.h"
#include "WorldMap.h"
#include <array>

class QtMapLoader {
public:
    // 解析指定的 Tiled JSON 地图文件，并将图形元素添加到给定的 QGraphicsScene 中。
    // worldMap    : 非空时同步碰撞数据（obstruction 层）和可交互实体（objectgroup 层）
    // pixelOffsetX/Y : 场景中的像素偏移（用于大世界拼图）
    // tileOffsetX/Y  : MapSystem 中的格子偏移（worldMap 坐标系中的起始格子位置）
    static void LoadMapToScene(const QString& jsonPath,
                               QGraphicsScene* scene,
                               WorldMap* worldMap   = nullptr,
                               int pixelOffsetX     = 0,
                               int pixelOffsetY     = 0,
                               int tileOffsetX      = 0,
                               int tileOffsetY      = 0);

    // 解析 Tiled 导出的 .world 文件，将其中包含的所有子地图加载到同一个 QGraphicsScene 中实现无缝拼接。
    // worldMap 非空时会：
    //   1. 根据所有子地图的总覆盖范围调用 worldMap->InitMapSize(totalW, totalH)（全障碍初始化）
    //   2. 对每张子地图调用 LoadMapToScene 同步碰撞和实体数据
    static void LoadWorldToScene(const QString& worldJsonPath,
                                 QGraphicsScene* scene,
                                 WorldMap* worldMap = nullptr);

    // 获取怪物/NPC 的 4 方向贴图 [0]=左, [1]=上, [2]=右, [3]=下
    // spriteId: Tiled 地图中配置的 spriteId 属性值
    static const std::array<QPixmap, 4>* GetEntitySprites(int spriteId);
};
