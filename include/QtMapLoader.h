#ifndef QTMAPLOADER_H
#define QTMAPLOADER_H

#include <QString>
#include <QGraphicsScene>

class QtMapLoader {
public:
    // 解析指定的 Tiled JSON 地图文件，并将图形元素添加到给定的 QGraphicsScene 中
    // offsetX 和 offsetY 用于大世界拼图时设置单张地图的整体坐标偏移
    static void LoadMapToScene(const QString& jsonPath, QGraphicsScene* scene, int offsetX = 0, int offsetY = 0);

    // 解析 Tiled 导出的 .world 文件，将其中包含的所有子地图加载到同一个 QGraphicsScene 中实现无缝拼接
    static void LoadWorldToScene(const QString& worldJsonPath, QGraphicsScene* scene);
};

#endif // QTMAPLOADER_H
