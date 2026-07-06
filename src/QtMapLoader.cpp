#include "Common.h"

#include "QtMapLoader.h"

// ─────────────────────────────────────────────────────────────────────────────
// 静态 Tileset 图片缓存：避免重复加载同一张大图
// ─────────────────────────────────────────────────────────────────────────────
static QHash<QString, QPixmap> s_pixmapCache;

// ─────────────────────────────────────────────────────────────────────────────
// 将 JSON 里的相对图片路径（../tiles/xxx.png）解析为绝对物理路径
// ─────────────────────────────────────────────────────────────────────────────
static QString resolveImagePath(const QString& rawPath) {
    // 只取文件名，统一放到 PROJECT_DATA_DIR/tiles/ 下
    int lastSlash = rawPath.lastIndexOf('/');
    QString fileName = (lastSlash != -1) ? rawPath.mid(lastSlash + 1) : rawPath;
    return QString(PROJECT_DATA_DIR) + "/tiles/" + fileName;
}

// ─────────────────────────────────────────────────────────────────────────────
// LoadMapToScene：加载单张地图 JSON，渲染所有 tilelayer 到 scene
// ─────────────────────────────────────────────────────────────────────────────
void QtMapLoader::LoadMapToScene(const QString& jsonPath, QGraphicsScene* scene, int offsetX, int offsetY) {
    if (!scene) return;

    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "LoadMapToScene: 无法打开地图文件:" << jsonPath;
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "LoadMapToScene: JSON 解析失败:" << jsonPath;
        return;
    }

    QJsonObject mapObj = doc.object();
    int mapWidth   = mapObj["width"].toInt();
    int mapHeight  = mapObj["height"].toInt();
    int tileWidth  = mapObj["tilewidth"].toInt();
    int tileHeight = mapObj["tileheight"].toInt();

    if (mapWidth <= 0 || mapHeight <= 0 || tileWidth <= 0 || tileHeight <= 0) {
        qWarning() << "LoadMapToScene: 地图尺寸/瓦片尺寸无效:" << jsonPath;
        return;
    }

    // ── 1. 解析 Tilesets ────────────────────────────────────────────────────
    struct TilesetInfo {
        int     firstGid;
        int     columns;    // 直接来自 JSON，最准确
        int     tileW;
        int     tileH;
        int     spacing;    // tile 间距（像素）
        int     margin;     // 图片边距（像素）
        QPixmap image;      // 完整 tileset 大图
    };
    QList<TilesetInfo> tilesets;

    QJsonArray tilesetsArray = mapObj["tilesets"].toArray();
    for (int i = 0; i < tilesetsArray.size(); ++i) {
        QJsonObject tsObj = tilesetsArray[i].toObject();

        // 跳过外部 .tsx 引用（暂不支持）
        if (!tsObj.contains("image")) {
            qWarning() << "LoadMapToScene: 跳过外部 .tsx tileset (firstgid="
                       << tsObj["firstgid"].toInt() << ") in" << jsonPath;
            continue;
        }

        TilesetInfo ts;
        ts.firstGid = tsObj["firstgid"].toInt();
        ts.tileW    = tsObj["tilewidth"].toInt();
        ts.tileH    = tsObj["tileheight"].toInt();
        ts.spacing  = tsObj["spacing"].toInt(0);   // 默认 0
        ts.margin   = tsObj["margin"].toInt(0);    // 默认 0
        ts.columns  = tsObj["columns"].toInt(0);

        // fallback：tileW/tileH 若缺失则用地图默认值
        if (ts.tileW  <= 0) ts.tileW  = tileWidth;
        if (ts.tileH  <= 0) ts.tileH  = tileHeight;

        // fallback：columns 若缺失则用 imagewidth 计算
        if (ts.columns <= 0) {
            int iw = tsObj["imagewidth"].toInt();
            if (iw > 0 && ts.tileW > 0) {
                // 考虑 margin 和 spacing
                ts.columns = (iw - 2 * ts.margin + ts.spacing) / (ts.tileW + ts.spacing);
            }
            if (ts.columns <= 0) ts.columns = 1;
        }

        // 解析图片路径并带缓存加载
        QString imagePath = resolveImagePath(tsObj["image"].toString());
        if (!s_pixmapCache.contains(imagePath)) {
            QPixmap px(imagePath);
            if (px.isNull()) {
                qWarning() << "LoadMapToScene: 无法加载 tileset 图片:" << imagePath;
            }
            s_pixmapCache.insert(imagePath, px);
        }
        ts.image = s_pixmapCache[imagePath];

        if (!ts.image.isNull()) {
            tilesets.append(ts);
        }
    }

    // 按 firstGid 降序排列，方便后续 >= 查找
    std::sort(tilesets.begin(), tilesets.end(), [](const TilesetInfo& a, const TilesetInfo& b) {
        return a.firstGid > b.firstGid;
    });

    // ── 2. 遍历 Layers，仅处理 tilelayer ────────────────────────────────────
    QJsonArray layersArray = mapObj["layers"].toArray();
    for (int li = 0; li < layersArray.size(); ++li) {
        QJsonObject layerObj = layersArray[li].toObject();
        if (layerObj["type"].toString() != "tilelayer") continue;
        if (!layerObj["visible"].toBool(true)) continue; // 跳过隐藏层

        // 支持分块压缩（chunk）格式
        bool hasChunks = layerObj.contains("chunks");

        // ── Tiled 翻转标志（高3位）──────────────────────────────────────────
        // bit31: 水平翻转, bit30: 垂直翻转, bit29: 对角翻转（90°旋转）
        constexpr quint32 FLAG_FLIP_H = 0x80000000u;
        constexpr quint32 FLAG_FLIP_V = 0x40000000u;
        constexpr quint32 FLAG_FLIP_D = 0x20000000u;
        constexpr quint32 FLAG_MASK   = ~(FLAG_FLIP_H | FLAG_FLIP_V | FLAG_FLIP_D);

        // 统一为 (localX, localY, rawTileId) 三元组处理
        auto processTile = [&](int lx, int ly, int rawTileId) {
            if (rawTileId == 0) return; // 0 = 空格

            // 解析翻转标志，提取真实 GID
            quint32 raw = static_cast<quint32>(rawTileId);
            bool flipH = raw & FLAG_FLIP_H;
            bool flipV = raw & FLAG_FLIP_V;
            bool flipD = raw & FLAG_FLIP_D;
            int  tileId = static_cast<int>(raw & FLAG_MASK);
            if (tileId == 0) return;

            // 查找所属 Tileset（已降序排列，第一个 firstGid <= tileId 的就是）
            for (const TilesetInfo& ts : tilesets) {
                if (tileId >= ts.firstGid) {
                    int localId = tileId - ts.firstGid;

                    // 计算在 tileset 大图里的像素坐标（考虑 margin/spacing）
                    int col  = localId % ts.columns;
                    int row  = localId / ts.columns;
                    int srcX = ts.margin + col * (ts.tileW + ts.spacing);
                    int srcY = ts.margin + row * (ts.tileH + ts.spacing);

                    // 越界保护
                    if (srcX + ts.tileW > ts.image.width() ||
                        srcY + ts.tileH > ts.image.height()) {
                        break;
                    }

                    QPixmap tileImg = ts.image.copy(srcX, srcY, ts.tileW, ts.tileH);
                    if (tileImg.isNull()) break;

                    // 应用翻转/旋转变换（与 Tiled 规范一致）
                    // 顺序：先对角转置(D)，再水平(H)，再垂直(V)
                    if (flipH || flipV || flipD) {
                        QImage img = tileImg.toImage();
                        if (flipD) {
                            // 对角翻转 = 转置矩阵（交换 x/y 轴）
                            QTransform t(0, 1, 0, 1, 0, 0, 0, 0, 1);
                            img = img.transformed(t);
                        }
                        if (flipH) img = img.flipped(Qt::Horizontal);
                        if (flipV) img = img.flipped(Qt::Vertical);
                        tileImg = QPixmap::fromImage(img);
                    }

                    QGraphicsPixmapItem* item = new QGraphicsPixmapItem(tileImg);
                    item->setPos(lx * tileWidth + offsetX,
                                 ly * tileHeight + offsetY);
                    scene->addItem(item);
                    break;
                }
            }
        };

        if (hasChunks) {
            // 分块格式（infinite map）
            QJsonArray chunks = layerObj["chunks"].toArray();
            for (int ci = 0; ci < chunks.size(); ++ci) {
                QJsonObject chunk = chunks[ci].toObject();
                int chunkX = chunk["x"].toInt();
                int chunkY = chunk["y"].toInt();
                int chunkW = chunk["width"].toInt();
                int chunkH = chunk["height"].toInt();
                QJsonArray data = chunk["data"].toArray();
                for (int cy = 0; cy < chunkH; ++cy) {
                    for (int cx = 0; cx < chunkW; ++cx) {
                        // 使用 toDouble() 确保超过 INT_MAX 的翻转标志值能被正确读取
                        int tileId = static_cast<int>(static_cast<quint32>(data[cy * chunkW + cx].toDouble()));
                        processTile(chunkX + cx, chunkY + cy, tileId);
                    }
                }
            }
        } else {
            // 普通平铺格式
            QJsonArray dataArray = layerObj["data"].toArray();
            if (dataArray.isEmpty()) continue;
            for (int y = 0; y < mapHeight; ++y) {
                for (int x = 0; x < mapWidth; ++x) {
                    // 使用 toDouble() 确保超过 INT_MAX 的翻转标志值能被正确读取
                    int tileId = static_cast<int>(static_cast<quint32>(dataArray[y * mapWidth + x].toDouble()));
                    processTile(x, y, tileId);
                }
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// LoadWorldToScene：加载 .world 文件，拼接所有子地图到 scene
// ─────────────────────────────────────────────────────────────────────────────
void QtMapLoader::LoadWorldToScene(const QString& worldJsonPath, QGraphicsScene* scene) {
    if (!scene) return;

    QFile file(worldJsonPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "LoadWorldToScene: 无法打开 world 文件:" << worldJsonPath;
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "LoadWorldToScene: JSON 解析失败:" << worldJsonPath;
        return;
    }

    QJsonObject worldObj = doc.object();
    QJsonArray mapsArray = worldObj["maps"].toArray();

    s_pixmapCache.clear(); // 每次加载 World 时清空缓存，避免跨世界脏数据

    for (int i = 0; i < mapsArray.size(); ++i) {
        QJsonObject mapItem = mapsArray[i].toObject();
        QString fileName = mapItem["fileName"].toString();

        // Tiled 导出 .tmj，我们用的是 .json 副本
        if (fileName.endsWith(".tmj")) {
            fileName.replace(".tmj", ".json");
        }

        QString mapPath = QString(PROJECT_DATA_DIR) + "/maps/" + fileName;

        // 文件不存在则跳过（防止废弃条目导致崩溃）
        if (!QFile::exists(mapPath)) {
            qWarning() << "LoadWorldToScene: 地图文件不存在，跳过:" << mapPath;
            continue;
        }

        int x = mapItem["x"].toInt();
        int y = mapItem["y"].toInt();

        qDebug() << "加载地图块:" << fileName << "偏移 (" << x << "," << y << ")";
        LoadMapToScene(mapPath, scene, x, y);
    }
}
