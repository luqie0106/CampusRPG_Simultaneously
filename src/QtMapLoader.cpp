#include "Common.h"
#include "QtMapLoader.h"
#include "Enemy.h"
#include <QXmlStreamReader>
#include <QFileInfo>
#include <QRegularExpression>
#include <array>


// ─────────────────────────────────────────────────────────────────────────────
// 静态 Tileset 图片缓存：避免重复加载同一张大图
// ─────────────────────────────────────────────────────────────────────────────
static QHash<QString, QPixmap> s_pixmapCache;

// ─────────────────────────────────────────────────────────────────────────────
// 怪物/NPC 贴图缓存：spriteId → 4 方向贴图 [0]=左, [1]=上, [2]=右, [3]=下
// 贴图规格：96×128 精灵表（3列×4行，每帧32×32），取中间列(index=1)
// ─────────────────────────────────────────────────────────────────────────────
static QHash<int, std::array<QPixmap, 4>> s_entitySpriteCache;
static bool s_entitySpriteCacheInitialized = false;

static void initEntitySpriteCache() {
    if (s_entitySpriteCacheInitialized) return;
    s_entitySpriteCacheInitialized = true;

    constexpr int FRAME_W = 32;
    constexpr int FRAME_H = 32;
    constexpr int MID_COL = 1; // 中间列 index

    // 加载一张精灵表并切出中间列4帧
    auto loadSpriteSheet = [&](int spriteId, const QString& fileName) {
        // 优先从 data/tiles/ 加载，回退到 data/sprite_split/
        QString path = QString(PROJECT_DATA_DIR) + "/tiles/" + fileName;
        if (!QFile::exists(path)) {
            path = QString(PROJECT_DATA_DIR) + "/sprite_split/" + fileName;
        }
        if (!QFile::exists(path)) {
            // 尝试在 sprite_split 子目录中查找
            QString baseName = QFileInfo(fileName).baseName();
            // 提取角色编号，如 character_22
            QRegularExpression rx("(character_\\d+)");
            auto match = rx.match(baseName);
            if (match.hasMatch()) {
                QString folder = match.captured(1);
                path = QString(PROJECT_DATA_DIR) + "/sprite_split/" + folder + "/" + fileName;
            }
        }

        QPixmap sheet(path);
        if (sheet.isNull()) {
            qWarning() << "QtMapLoader: 无法加载实体贴图 spriteId=" << spriteId << "path=" << path;
            return;
        }

        std::array<QPixmap, 4> frames;
        // 行序：0=下, 1=左, 2=右, 3=上
        // 方向映射：[0]=左, [1]=上, [2]=右, [3]=下
        frames[0] = sheet.copy(FRAME_W * MID_COL, FRAME_H * 1, FRAME_W, FRAME_H); // 左
        frames[1] = sheet.copy(FRAME_W * MID_COL, FRAME_H * 3, FRAME_W, FRAME_H); // 上
        frames[2] = sheet.copy(FRAME_W * MID_COL, FRAME_H * 2, FRAME_W, FRAME_H); // 右
        frames[3] = sheet.copy(FRAME_W * MID_COL, FRAME_H * 0, FRAME_W, FRAME_H); // 下

        s_entitySpriteCache[spriteId] = frames;
        qDebug() << "QtMapLoader: 加载实体贴图 spriteId=" << spriteId << "path=" << path;
    };

    // ── 怪物贴图注册 ────────────────────────────────────────────────
    loadSpriteSheet(100, "character_16_frame32x32.png"); // 小树林霸主 ForestBoss
    loadSpriteSheet(101, "character_22_frame32x32.png"); // 校园混混 Bully
    loadSpriteSheet(102, "character_1_frame32x32.png");  // 逃课大神 Skipper
    loadSpriteSheet(103, "character_2_frame32x32.png");  // 考试黄牛 Cheater
    loadSpriteSheet(104, "character_7_frame32x32.png");  // 小弟弟 GangMember
    loadSpriteSheet(105, "character_11_frame32x32.png"); // 树林野犬 ForestMonster1
    loadSpriteSheet(106, "character_14_frame32x32.png"); // 幽暗黑影 ForestMonster2
    loadSpriteSheet(107, "character_6_frame32x32.png");  // 午夜卷王幽灵 MidnightNerd
    loadSpriteSheet(201, "character_18_frame32x32.png"); // 教导主任 DeanOfStudents
    loadSpriteSheet(202, "character_20_frame32x32.png"); // 体育委员长 PECommittee
    loadSpriteSheet(203, "character_23_frame32x32.png"); // 校长 Principal
    loadSpriteSheet(204, "character_5_frame32x32.png");  // 宿管阿姨 DormGuard

    // ── NPC 贴图注册（500-599 段为 NPC 专用） ────────────────────
    loadSpriteSheet(500, "character_32_frame32x32.png"); // 办公室 王老师
    loadSpriteSheet(501, "character_30_frame32x32.png"); // 办公室 刘老师
    loadSpriteSheet(502, "character_31_frame32x32.png"); // 办公室 周老师
    loadSpriteSheet(503, "character_26_frame32x32.png"); // 宿舍 小王
    loadSpriteSheet(504, "character_22_frame32x32.png"); // 教室1 小明
    loadSpriteSheet(505, "character_23_frame32x32.png"); // 教室2 小华
    loadSpriteSheet(506, "character_24_frame32x32.png"); // 商店 商人
    // 507 character_25 备用
    // TODO: 后续补充其他怪物和NPC的贴图
    // ...
}

const std::array<QPixmap, 4>* QtMapLoader::GetEntitySprites(int spriteId) {
    initEntitySpriteCache();
    auto it = s_entitySpriteCache.find(spriteId);
    if (it != s_entitySpriteCache.end()) {
        return &it.value();
    }
    return nullptr;
}

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
// 从 Tiled properties 数组中按名称查找属性值
// ─────────────────────────────────────────────────────────────────────────────
static QJsonValue getProperty(const QJsonArray& properties, const QString& name) {
    for (int i = 0; i < properties.size(); ++i) {
        QJsonObject p = properties[i].toObject();
        if (p["name"].toString() == name) {
            return p["value"];
        }
    }
    return QJsonValue();
}

// ─────────────────────────────────────────────────────────────────────────────
// 解析单个 objectgroup 图层，向 worldMap 注册可交互实体
//
// 坐标约定：
//   obj["x"], obj["y"]  — 对象在当前子地图中的像素坐标（左上角）
//   tileOffsetX/Y       — 当前子地图的左上角在 MapSystem 格子坐标系中的偏移
//   tileWidth/Height    — 单个瓦片的像素尺寸
//
// 支持的 object type：
//   "Monster"    → MakeEnemy / MakeBoss
//   "NPC"        → MakeNPC
//   "Door"       → MakeDoor（预留，当前地图无 Door 对象）
//   "PlayerSpawn"→ SetSpawnPoint
//   "Item"       → 暂不处理（拾取系统待实现）
// ─────────────────────────────────────────────────────────────────────────────
static void parseObjectGroup(const QJsonObject& layerObj,
                             WorldMap* worldMap,
                             QGraphicsScene* scene,
                             int tileWidth, int tileHeight,
                             int tileOffsetX, int tileOffsetY,
                             int pixelOffsetX, int pixelOffsetY)
{
    if (!worldMap) return;

    QJsonArray objects = layerObj["objects"].toArray();
    for (int oi = 0; oi < objects.size(); ++oi) {
        QJsonObject obj = objects[oi].toObject();

        // 像素坐标 → 格子坐标（向下取整）
        int px = static_cast<int>(obj["x"].toDouble());
        int py = static_cast<int>(obj["y"].toDouble());
        int tileX = tileOffsetX + px / tileWidth;
        int tileY = tileOffsetY + py / tileHeight;

        QString objType  = obj["type"].toString();
        QString objName  = obj["name"].toString();
        int     objId    = obj["id"].toInt();          // Tiled 自动分配，地图内唯一
        QJsonArray props = obj["properties"].toArray();

        GamePoint pos(tileX, tileY);

        // ── PlayerSpawn：设置出生点 ────────────────────────────────────
        if (objType == QStringLiteral("PlayerSpawn")) {
            worldMap->SetSpawnPoint(pos);
            qDebug() << "  [PlayerSpawn] 出生点 →" << tileX << "," << tileY;
            continue;
        }

        // ── Door（传送点，预留） ───────────────────────────────────────
        if (objType == QStringLiteral("Door")) {
            QString targetMap = getProperty(props, "targetMap").toString();
            int     targetX   = getProperty(props, "targetX").toInt(0);
            int     targetY   = getProperty(props, "targetY").toInt(0);
            worldMap->AddInteractable(
                InteractableInfo::MakeDoor(objId, pos,
                                           objName.isEmpty() ? "门" : objName.toStdString(),
                                           targetMap.toStdString(),
                                           targetX, targetY));
            qDebug() << "  [Door] id=" << objId << "tile=(" << tileX << "," << tileY
                     << ") → " << targetMap;
            continue;
        }

        // ── Monster（普通怪物或 Boss） ────────────────────────────────
        if (objType == QStringLiteral("Monster")) {
            bool isBoss = getProperty(props, "isBoss").toBool(false);
            int  sprId  = getProperty(props, "spriteId").toInt(0);

            Enemy tpl = Enemy::Bully(); // 默认：校园混混
            std::string displayName = objName.isEmpty() ? "神秘怪物" : objName.toStdString();

            if (isBoss) {
                switch (sprId) {
                    case 100: tpl = Enemy::ForestBoss();     break;
                    case 201: tpl = Enemy::DeanOfStudents(); break;
                    case 202: tpl = Enemy::PECommittee();    break;
                    case 203: tpl = Enemy::Principal();      break;
                    case 204: tpl = Enemy::DormGuard();      break;
                    default:  tpl = Enemy::DormGuard();      break;
                }
            } else {
                // 普通怪物按 spriteId 映射
                switch (sprId) {
                    case 101: tpl = Enemy::Bully();          break;
                    case 102: tpl = Enemy::Skipper();        break;
                    case 103: tpl = Enemy::Cheater();        break;
                    case 104: tpl = Enemy::GangMember();     break;
                    case 105: tpl = Enemy::ForestMonster1(); break;
                    case 106: tpl = Enemy::ForestMonster2(); break;
                    case 107: tpl = Enemy::MidnightNerd();   break;
                    default:  tpl = Enemy::Bully();          break;
                }
            }
            
            displayName = tpl.GetName();

            InteractableInfo info = InteractableInfo::MakeEnemy(objId, pos, displayName, tpl);
            if (isBoss) info.type = InteractableType::Boss;
            if (sprId == 204 || sprId == 107) info.isNightOnly = true;
            worldMap->AddInteractable(std::move(info));

            // 在地图上绘制怪物贴图
            if (scene) {
                initEntitySpriteCache();
                QGraphicsPixmapItem* item = nullptr;
                const auto* sprites = QtMapLoader::GetEntitySprites(sprId);

                if (sprites && !(*sprites)[3].isNull()) {
                    // 有实际贴图：使用朝下帧（默认方向）
                    item = new QGraphicsPixmapItem((*sprites)[3]);
                    item->setPos(px + pixelOffsetX, py + pixelOffsetY);
                } else {
                    // 无贴图：使用纯色方块作为占位符
                    QPixmap pix(tileWidth, tileHeight);
                    pix.fill(Qt::transparent);
                    QPainter painter(&pix);
                    if (isBoss) {
                        painter.fillRect(0, 0, tileWidth, tileHeight, QColor(128, 0, 128, 200));
                    } else {
                        painter.fillRect(0, 0, tileWidth, tileHeight, QColor(255, 0, 0, 200));
                    }
                    item = new QGraphicsPixmapItem(pix);
                    item->setPos(px + pixelOffsetX, py + pixelOffsetY);
                }

                item->setZValue(5);
                item->setData(0, objId);
                item->setData(1, "Monster");
                item->setData(2, isBoss);
                item->setData(3, sprId);    // spriteId，供 updateEntityFacing 查询贴图
                item->setData(4, 3);        // 当前方向索引，默认朝下(3)
                scene->addItem(item);
            }

            qDebug() << "  [Monster] id=" << objId << "isBoss=" << isBoss
                     << "spriteId=" << sprId << "tile=(" << tileX << "," << tileY << ")";
            continue;
        }

        // ── NPC（对话/任务 NPC） ──────────────────────────────────────
        if (objType == QStringLiteral("NPC")) {
            QString npcName = getProperty(props, "npcName").toString();
            if (npcName.isEmpty()) npcName = objName;
            bool hasShop = getProperty(props, "hasShop").toBool(false);
            int  sprId   = getProperty(props, "spriteId").toInt(0);

            InteractableInfo info;
            if (hasShop) {
                info = InteractableInfo::MakeShop(objId, pos, npcName.toStdString());
            } else {
                info = InteractableInfo::MakeNPC(objId, pos, npcName.toStdString());
            }
            worldMap->AddInteractable(std::move(info));

            // 在地图上绘制 NPC 贴图
            if (scene) {
                initEntitySpriteCache();
                QGraphicsPixmapItem* item = nullptr;
                const auto* sprites = QtMapLoader::GetEntitySprites(sprId);

                if (sprites && !(*sprites)[3].isNull()) {
                    item = new QGraphicsPixmapItem((*sprites)[3]);
                    item->setPos(px + pixelOffsetX, py + pixelOffsetY);
                } else {
                    // 无贴图：使用蓝色方块作为占位符
                    QPixmap pix(tileWidth, tileHeight);
                    pix.fill(Qt::transparent);
                    QPainter painter(&pix);
                    painter.fillRect(0, 0, tileWidth, tileHeight, QColor(0, 0, 255, 200));
                    item = new QGraphicsPixmapItem(pix);
                    item->setPos(px + pixelOffsetX, py + pixelOffsetY);
                }

                item->setZValue(5);
                item->setData(0, objId);
                item->setData(1, "NPC");
                item->setData(2, hasShop);
                item->setData(3, sprId);
                item->setData(4, 3);
                scene->addItem(item);
            }

            qDebug() << "  [NPC] id=" << objId << "name=" << npcName
                     << "hasShop=" << hasShop << "spriteId=" << sprId
                     << "tile=(" << tileX << "," << tileY << ")";
            continue;
        }

        // ── Item（暂不处理，拾取系统待实现） ─────────────────────────
        if (objType == QStringLiteral("Item")) {
            int itemId = getProperty(props, "itemId").toInt(0);
            qDebug() << "  [Item] id=" << objId << "itemId=" << itemId
                     << "tile=(" << tileX << "," << tileY << ") - 暂未注册（拾取系统待实现）";
            continue;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// LoadMapToScene：加载单张地图 JSON，渲染所有 tilelayer 到 scene
//
// 同时（当 worldMap != nullptr 时）：
//   A. 对 obstruction 图层逐格写入 MapSystem：
//      - tileId != 0 → setTile(worldX, worldY, 1)（障碍）
//      - tileId == 0 → setTile(worldX, worldY, 0)（可通行）
//   B. 对 objectgroup 图层解析实体并注册到 WorldMap
// ─────────────────────────────────────────────────────────────────────────────
void QtMapLoader::LoadMapToScene(const QString& jsonPath,
                                 QGraphicsScene* scene,
                                 WorldMap* worldMap,
                                 int pixelOffsetX,
                                 int pixelOffsetY,
                                 int tileOffsetX,
                                 int tileOffsetY)
{
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
        // 该 tileset 中标记了 "solid": true 的瓦片 localId 集合
        // 用于在 processTile 中判断该格子是否为障碍物
        std::unordered_set<int> solidTiles;
    };
    QList<TilesetInfo> tilesets;

    QJsonArray tilesetsArray = mapObj["tilesets"].toArray();
    for (int i = 0; i < tilesetsArray.size(); ++i) {
        QJsonObject tsObj = tilesetsArray[i].toObject();

        // 检查是否为外部 .tsx 引用
        if (!tsObj.contains("image") && tsObj.contains("source")) {
            QString source = tsObj["source"].toString();
            QString tsxPath = QFileInfo(jsonPath).dir().filePath(source);
            
            QFile tsxFile(tsxPath);
            if (tsxFile.open(QIODevice::ReadOnly)) {
                QXmlStreamReader xml(&tsxFile);
                TilesetInfo ts;
                ts.firstGid = tsObj["firstgid"].toInt();
                ts.tileW = tileWidth;
                ts.tileH = tileHeight;
                ts.spacing = 0;
                ts.margin = 0;
                ts.columns = 1;
                
                while (!xml.atEnd() && !xml.hasError()) {
                    QXmlStreamReader::TokenType token = xml.readNext();
                    if (token == QXmlStreamReader::StartElement) {
                        if (xml.name() == QStringLiteral("tileset")) {
                            if (xml.attributes().hasAttribute("tilewidth")) ts.tileW = xml.attributes().value("tilewidth").toInt();
                            if (xml.attributes().hasAttribute("tileheight")) ts.tileH = xml.attributes().value("tileheight").toInt();
                            if (xml.attributes().hasAttribute("spacing")) ts.spacing = xml.attributes().value("spacing").toInt();
                            if (xml.attributes().hasAttribute("margin")) ts.margin = xml.attributes().value("margin").toInt();
                            if (xml.attributes().hasAttribute("columns")) ts.columns = xml.attributes().value("columns").toInt();
                        } else if (xml.name() == QStringLiteral("image")) {
                            QString imgSource = xml.attributes().value("source").toString();
                            QString imagePath = resolveImagePath(imgSource);
                            if (!s_pixmapCache.contains(imagePath)) {
                                QPixmap px(imagePath);
                                if (px.isNull()) {
                                    qWarning() << "LoadMapToScene: 无法加载 TSX tileset 图片:" << imagePath;
                                }
                                s_pixmapCache.insert(imagePath, px);
                            }
                            ts.image = s_pixmapCache[imagePath];
                            
                            if (ts.columns <= 0) {
                                int iw = xml.attributes().value("width").toInt();
                                if (iw > 0 && ts.tileW > 0) {
                                    ts.columns = (iw - 2 * ts.margin + ts.spacing) / (ts.tileW + ts.spacing);
                                }
                                if (ts.columns <= 0) ts.columns = 1;
                            }
                        } else if (xml.name() == QStringLiteral("tile")) {
                            int localId = xml.attributes().value("id").toInt();
                            while (!xml.atEnd()) {
                                QXmlStreamReader::TokenType t = xml.readNext();
                                if (t == QXmlStreamReader::EndElement && xml.name() == QStringLiteral("tile")) {
                                    break;
                                }
                                if (t == QXmlStreamReader::StartElement && xml.name() == QStringLiteral("property")) {
                                    if (xml.attributes().value("name") == QStringLiteral("solid") &&
                                        xml.attributes().value("value") == QStringLiteral("true")) {
                                        ts.solidTiles.insert(localId);
                                    }
                                }
                            }
                        }
                    }
                }
                
                if (!ts.image.isNull()) {
                    tilesets.append(ts);
                }
            } else {
                qWarning() << "LoadMapToScene: 无法读取外部 TSX 文件:" << tsxPath;
            }
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

        // ── 解析 solid 瓦片属性 ────────────────────────────────────────────
        // Tiled 在 tileset 的 "tiles" 数组中存储各瓦片的自定义属性。
        // 若某瓦片有 "solid": true，则将其 localId 记入 solidTiles 集合。
        // processTile 渲染时检查 localId，若存在则标记 MapSystem 对应格子为障碍(1)。
        QJsonArray tilesArr = tsObj["tiles"].toArray();
        for (int ti = 0; ti < tilesArr.size(); ++ti) {
            QJsonObject tileObj = tilesArr[ti].toObject();
            QJsonArray  tileProps = tileObj["properties"].toArray();
            for (int pi = 0; pi < tileProps.size(); ++pi) {
                QJsonObject prop = tileProps[pi].toObject();
                if (prop["name"].toString() == QStringLiteral("solid") &&
                    prop["value"].toBool(false)) {
                    ts.solidTiles.insert(tileObj["id"].toInt());
                    break;
                }
            }
        }

        if (!ts.image.isNull()) {
            tilesets.append(ts);
        }
    }

    // 按 firstGid 降序排列，方便后续 >= 查找
    std::sort(tilesets.begin(), tilesets.end(), [](const TilesetInfo& a, const TilesetInfo& b) {
        return a.firstGid > b.firstGid;
    });

    // ── 2. 遍历 Layers ───────────────────────────────────────────────────────
    QJsonArray layersArray = mapObj["layers"].toArray();
    for (int li = 0; li < layersArray.size(); ++li) {
        QJsonObject layerObj = layersArray[li].toObject();
        QString layerType    = layerObj["type"].toString();
        QString layerName    = layerObj["name"].toString();

        // ── objectgroup 层：解析实体，注册到 WorldMap ──────────────────────
        if (layerType == QStringLiteral("objectgroup")) {
            parseObjectGroup(layerObj, worldMap, scene, tileWidth, tileHeight,
                             tileOffsetX, tileOffsetY, pixelOffsetX, pixelOffsetY);
            continue;
        }

        // ── tilelayer 层 ───────────────────────────────────────────────────
        if (layerType != QStringLiteral("tilelayer")) continue;
        if (!layerObj["visible"].toBool(true)) continue; // 跳过隐藏层

        // ── Tiled 翻转标志（高3位）──────────────────────────────────────────
        // bit31: 水平翻转, bit30: 垂直翻转, bit29: 对角翻转（90°旋转）
        constexpr quint32 FLAG_FLIP_H = 0x80000000u;
        constexpr quint32 FLAG_FLIP_V = 0x40000000u;
        constexpr quint32 FLAG_FLIP_D = 0x20000000u;
        constexpr quint32 FLAG_MASK   = ~(FLAG_FLIP_H | FLAG_FLIP_V | FLAG_FLIP_D);

        // 统一为 (localX, localY, rawTileId) 三元组处理
        auto processTile = [&](int lx, int ly, int rawTileId) {

            // ── 碰撞同步（基于 solid 瓦片属性）──────────────────────────────
            // 任意图层上，只要瓦片的 localId 在该 tileset 的 solidTiles 集合中，
            // 该格子就标记为障碍(1)。setTile 是覆盖写入，多层叠加时只要有一层
            // 存在 solid 瓦片该格子就永久为障碍，不会被其他层的空格覆盖归零。
            // 注意：碰撞写入在渲染之前，避免空格早退 return 导致漏写。

            if (rawTileId == 0) return; // 0 = 空格，无需渲染也无需写碰撞

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

                    // ── solid 属性碰撞写入 ────────────────────────────────
                    // 只要 localId 在 solidTiles 中，无论该格子属于哪个图层，
                    // 都将世界格子标记为障碍(1)。
                    if (worldMap && !ts.solidTiles.empty() &&
                        ts.solidTiles.count(localId)) {
                        int wx = tileOffsetX + lx;
                        int wy = tileOffsetY + ly;
                        worldMap->GetMapSystem().setTile(wx, wy, 1);
                    }

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
                    item->setPos(lx * tileWidth  + pixelOffsetX,
                                 ly * tileHeight + pixelOffsetY);
                    scene->addItem(item);
                    break;
                }
            }
        };

        // 支持分块压缩（chunk）格式
        bool hasChunks = layerObj.contains("chunks");

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
//
// 当 worldMap != nullptr 时，额外执行：
//   1. 遍历所有子地图，计算世界格栅的总覆盖范围（minTile / maxTile）
//   2. 调用 worldMap->InitMapSize(totalW, totalH)（全格障碍初始化）
//   3. 对每张子地图调用 LoadMapToScene 并传入格子偏移（tileOffsetX/Y）
//
// 坐标系：
//   - 每张子地图在 .world 文件中的 (x, y) 是像素坐标，相对于世界原点
//   - 世界原点可能为负值；我们将最小 (minPx, minPy) 归零作为格子坐标系原点
// ─────────────────────────────────────────────────────────────────────────────
void QtMapLoader::LoadWorldToScene(const QString& worldJsonPath,
                                   QGraphicsScene* scene,
                                   WorldMap* worldMap)
{
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

    QJsonObject worldObj  = doc.object();
    QJsonArray  mapsArray = worldObj["maps"].toArray();

    s_pixmapCache.clear(); // 每次加载 World 时清空缓存，避免跨世界脏数据

    // ── Step 1: 收集所有子地图的元信息 ──────────────────────────────────
    struct SubMapEntry {
        QString mapPath;
        int pixelX;   // .world 中的像素偏移 X
        int pixelY;   // .world 中的像素偏移 Y
        int pixelW;   // .world 中记录的像素宽度
        int pixelH;   // .world 中记录的像素高度
    };

    // 默认瓦片尺寸（绝大多数地图一致；若子地图不同会以其自身为准）
    constexpr int DEFAULT_TILE_W = 16;
    constexpr int DEFAULT_TILE_H = 16;

    QList<SubMapEntry> entries;
    int minPx = INT_MAX, minPy = INT_MAX;
    int maxPx = INT_MIN, maxPy = INT_MIN;

    // worldDir 未使用（mapPath 已由 PROJECT_DATA_DIR 拼接）

    for (int i = 0; i < mapsArray.size(); ++i) {
        QJsonObject mapItem = mapsArray[i].toObject();
        QString fileName = mapItem["fileName"].toString();



        QString mapPath = QString(PROJECT_DATA_DIR) + "/maps/" + fileName;
        if (!QFile::exists(mapPath)) {
            qWarning() << "LoadWorldToScene: 地图文件不存在，跳过:" << mapPath;
            continue;
        }

        SubMapEntry e;
        e.mapPath = mapPath;
        e.pixelX  = mapItem["x"].toInt();
        e.pixelY  = mapItem["y"].toInt();
        e.pixelW  = mapItem["width"].toInt();
        e.pixelH  = mapItem["height"].toInt();
        entries.append(e);

        minPx = std::min(minPx, e.pixelX);
        minPy = std::min(minPy, e.pixelY);
        maxPx = std::max(maxPx, e.pixelX + e.pixelW);
        maxPy = std::max(maxPy, e.pixelY + e.pixelH);
    }

    if (entries.isEmpty()) {
        qWarning() << "LoadWorldToScene: 没有可加载的子地图";
        return;
    }

    // ── Step 2: 计算世界格栅总尺寸并初始化 MapSystem ──────────────────
    if (worldMap) {
        int totalPxW = maxPx - minPx;
        int totalPxH = maxPy - minPy;
        int totalTileW = (totalPxW + DEFAULT_TILE_W - 1) / DEFAULT_TILE_W; // 向上取整
        int totalTileH = (totalPxH + DEFAULT_TILE_H - 1) / DEFAULT_TILE_H;

        qDebug() << "LoadWorldToScene: 世界格栅初始化"
                 << totalTileW << "x" << totalTileH
                 << "（像素范围" << minPx << "," << minPy << "->" << maxPx << "," << maxPy << "）";

        // 全障碍初始化；后续每张子地图的 obstruction 层负责逐格开通可行走区域
        worldMap->InitMapSize(totalTileW, totalTileH);
    }

    // ── Step 3: 逐张加载子地图 ───────────────────────────────────────────
    for (const SubMapEntry& e : entries) {
        // 场景中的像素偏移：相对最小角归零
        int sceneOffX = e.pixelX - minPx;
        int sceneOffY = e.pixelY - minPy;

        // MapSystem 中的格子偏移（将负数像素坐标映射到非负格子坐标）
        int tileOffX = (e.pixelX - minPx) / DEFAULT_TILE_W;
        int tileOffY = (e.pixelY - minPy) / DEFAULT_TILE_H;

        qDebug() << "\u52a0\u8f7d\u5b50\u5730\u56fe:" << QString(e.mapPath).split('/').last()
                 << "scene偏移(" << sceneOffX << "," << sceneOffY << ")"
                 << "tile偏移(" << tileOffX << "," << tileOffY << ")";

        LoadMapToScene(e.mapPath, scene, worldMap,
                       sceneOffX, sceneOffY,
                       tileOffX,  tileOffY);
    }
}
