#ifndef QTMAPLOADER_H
#define QTMAPLOADER_H

#include <QString>
#include <QGraphicsScene>

class QtMapLoader {
public:
    // 解析指定的 Tiled JSON 地图文件，并将图形元素添加到给定的 QGraphicsScene 中
    static void LoadMapToScene(const QString& jsonPath, QGraphicsScene* scene);
};

#endif // QTMAPLOADER_H
