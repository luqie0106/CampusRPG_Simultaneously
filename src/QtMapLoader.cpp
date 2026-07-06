#include "Common.h"

#include "QtMapLoader.h"
void QtMapLoader::LoadMapToScene(const QString& jsonPath, QGraphicsScene* scene, int offsetX, int offsetY) {
    if (!scene) return;

    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open map file:" << jsonPath;
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Failed to parse JSON map:" << jsonPath;
        return;
    }

    QJsonObject mapObj = doc.object();
    int mapWidth = mapObj["width"].toInt();
    int mapHeight = mapObj["height"].toInt();
    int tileWidth = mapObj["tilewidth"].toInt();
    int tileHeight = mapObj["tileheight"].toInt();

    // 1. 解析 Tilesets
    struct TilesetInfo {
        int firstGid;
        QPixmap image;
        int imageWidth;
        int imageHeight;
        int tileWidth;
        int tileHeight;
    };
    QList<TilesetInfo> tilesets;

    QJsonArray tilesetsArray = mapObj["tilesets"].toArray();
    for (int i = 0; i < tilesetsArray.size(); ++i) {
        QJsonObject tsObj = tilesetsArray[i].toObject();
        TilesetInfo tsInfo;
        tsInfo.firstGid = tsObj["firstgid"].toInt();
        
        // 处理图片路径: 将 "../tiles/Interiors_free_16x16.png" 转换为绝对物理路径
        QString imagePath = tsObj["image"].toString();
        int lastSlash = imagePath.lastIndexOf('/');
        if (lastSlash != -1) {
            QString fileName = imagePath.mid(lastSlash + 1);
            // 使用 CMake 传进来的跨平台绝对路径宏
            imagePath = QString(PROJECT_DATA_DIR) + "/tiles/" + fileName;
        }
        
        tsInfo.image = QPixmap(imagePath);
        if (tsInfo.image.isNull()) {
            qWarning() << "Failed to load tileset image:" << imagePath;
        }

        tsInfo.imageWidth = tsObj["imagewidth"].toInt();
        tsInfo.imageHeight = tsObj["imageheight"].toInt();
        tsInfo.tileWidth = tsObj["tilewidth"].toInt();
        if (tsInfo.tileWidth == 0) tsInfo.tileWidth = tileWidth;
        tsInfo.tileHeight = tsObj["tileheight"].toInt();
        if (tsInfo.tileHeight == 0) tsInfo.tileHeight = tileHeight;

        tilesets.append(tsInfo);
    }

    // 根据 firstGid 降序排序
    std::sort(tilesets.begin(), tilesets.end(), [](const TilesetInfo& a, const TilesetInfo& b) {
        return a.firstGid > b.firstGid;
    });

    // 2. 解析 Layers（仅处理 tilelayer）
    QJsonArray layersArray = mapObj["layers"].toArray();
    for (int i = 0; i < layersArray.size(); ++i) {
        QJsonObject layerObj = layersArray[i].toObject();
        if (layerObj["type"].toString() != "tilelayer") {
            continue;
        }

        QJsonArray dataArray = layerObj["data"].toArray();
        if (dataArray.isEmpty()) continue;

        for (int y = 0; y < mapHeight; ++y) {
            for (int x = 0; x < mapWidth; ++x) {
                int index = y * mapWidth + x;
                int tileId = dataArray[index].toInt();

                if (tileId == 0) continue; // 0 表示没有贴图

                // 查找该 tileId 属于哪个 Tileset
                for (const TilesetInfo& ts : tilesets) {
                    if (tileId >= ts.firstGid) {
                        int localId = tileId - ts.firstGid;
                        
                        int columns = ts.imageWidth / ts.tileWidth;
                        if (columns == 0) columns = 1;

                        int srcX = (localId % columns) * ts.tileWidth;
                        int srcY = (localId / columns) * ts.tileHeight;

                        QPixmap tileImg = ts.image.copy(srcX, srcY, ts.tileWidth, ts.tileHeight);
                        
                        QGraphicsPixmapItem* item = new QGraphicsPixmapItem(tileImg);
                        item->setPos(x * tileWidth + offsetX, y * tileHeight + offsetY);
                        scene->addItem(item);

                        break; // 找到对应瓦片就跳出查找
                    }
                }
            }
        }
    }
}

void QtMapLoader::LoadWorldToScene(const QString& worldJsonPath, QGraphicsScene* scene) {
    if (!scene) return;

    QFile file(worldJsonPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open world file:" << worldJsonPath;
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Failed to parse JSON world:" << worldJsonPath;
        return;
    }

    QJsonObject worldObj = doc.object();
    QJsonArray mapsArray = worldObj["maps"].toArray();

    // 遍历世界上所有的地图，根据坐标拼接到大幕布里
    for (int i = 0; i < mapsArray.size(); ++i) {
        QJsonObject mapItem = mapsArray[i].toObject();
        QString fileName = mapItem["fileName"].toString();
        
        // 由于 Tiled 导出的是 .tmj 扩展名，而我们的文件都是 .json
        if (fileName.endsWith(".tmj")) {
            fileName.replace(".tmj", ".json");
        }

        int x = mapItem["x"].toInt();
        int y = mapItem["y"].toInt();

        // 假设所有的子地图都在 world 文件所在的同一个 maps 文件夹下
        QString mapPath = QString(PROJECT_DATA_DIR) + "/maps/" + fileName;
        
        // 调用单张地图加载函数，传入物理偏移量
        qDebug() << "Loading chunk:" << fileName << "at" << x << "," << y;
        LoadMapToScene(mapPath, scene, x, y);
    }
}
