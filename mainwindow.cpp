#include "Common.h"

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QtMapLoader.h"
#include "ShopDialog.h"
#include "ShopWindow.h"
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    // 全部变量初始化放在构造函数内部
    moveSpeed = 5;
    keyW = false;
    keyA = false;
    keyS = false;
    keyD = false;
    currentCharacter = 0;
    shopDialogActive = false;
    m_shopDialog = nullptr;
    m_shopWindow = nullptr;

    // 商店青色桌子在世界坐标中的中心位置
    shopTableCenter = QPointF(396, 1348);

    ui->setupUi(this);

    // 初始化场景与视图
    mapScene = new QGraphicsScene(this);
    view = new QGraphicsView(mapScene, this);
    this->setCentralWidget(view);

    // 修复 Mac 下按键无反应的问题（焦点与输入法拦截）
    this->setFocusPolicy(Qt::StrongFocus);
    view->setFocusPolicy(Qt::NoFocus);
    this->setAttribute(Qt::WA_InputMethodEnabled, false);

    // ========== 初始化游戏引擎 ==========
    m_engine.Init();

    // ========== 无缝加载大世界地图（同步碰撞数据和实体到引擎）==========
    QString worldPath = QString(PROJECT_DATA_DIR) + "/maps/game.world";
    qDebug() << "开始加载大世界:" << worldPath;
    QtMapLoader::LoadWorldToScene(worldPath, mapScene, &m_engine.GetWorldMap());

    // 锁死整个大世界地图边界
    mapScene->setSceneRect(mapScene->itemsBoundingRect());

    // ========== 玩家贴图切片与初始化 ==========
    // 角色0: male_01_hat_walk_32x32_3frames.png
    // 规格: 每帧 32x32, 每行 3 帧, 共 4 行(4个方向)
    // 实测行序: 0下, 1左, 2右, 3上
    QPixmap char0Sheet(":/data/data/tiles/male_01_hat_walk_32x32_3frames.png");
    const int c0W = 32;
    const int c0H = 32;

    if (!char0Sheet.isNull()) {
        // 取每行中间帧(第2列, index=1)作为该方向静止贴图
        playerFrames[0][0] = char0Sheet.copy(c0W * 1, c0H * 1, c0W, c0H);  // 左
        playerFrames[0][1] = char0Sheet.copy(c0W * 1, c0H * 3, c0W, c0H);  // 上
        playerFrames[0][2] = char0Sheet.copy(c0W * 1, c0H * 2, c0W, c0H);  // 右
        playerFrames[0][3] = char0Sheet.copy(c0W * 1, c0H * 0, c0W, c0H);  // 下
    } else {
        qWarning() << "未能加载角色0精灵图，请检查 qrc 路径与构建！";
    }

    // 角色1: Alex_idle_16x16.png
    // 实际尺寸: 64x32, 单行 4 帧, 每帧 16x32
    // 帧序: 0右, 1上, 2左, 3下
    QPixmap char1Sheet(":/data/data/Characters_free/Alex_idle_16x16.png");
    const int c1W = 16;
    const int c1H = 32;

    if (!char1Sheet.isNull()) {
        playerFrames[1][0] = char1Sheet.copy(c1W * 2, 0, c1W, c1H);  // 左
        playerFrames[1][1] = char1Sheet.copy(c1W * 1, 0, c1W, c1H);  // 上
        playerFrames[1][2] = char1Sheet.copy(c1W * 0, 0, c1W, c1H);  // 右
        playerFrames[1][3] = char1Sheet.copy(c1W * 3, 0, c1W, c1H);  // 下
    } else {
        qWarning() << "未能加载角色1(Alex)精灵图，请检查 qrc 路径与构建！";
    }

    player = new QGraphicsPixmapItem();
    if (!playerFrames[0][3].isNull()) {
        player->setPixmap(playerFrames[0][3]); // 默认角色0朝下
    }
    player->setZValue(10); // 层级高于地图，不会被瓦片遮挡
    mapScene->addItem(player);
    
    // 获取真实出生点并换算为像素坐标 (瓦片大小默认 16)
    GamePoint spawnPoint = m_engine.GetWorldMap().GetSpawnPoint();
    QPointF spawnPx(spawnPoint.x * 16.0, spawnPoint.y * 16.0);
    player->setPos(spawnPx);
    // 记录脚底锚点位置（top-left + 宽度一半, 高度），切换角色时以此为基准重算
    playerLogicalPos = QPointF(spawnPx.x() + 32.0 / 2.0, spawnPx.y() + 32.0);

    // 移动定时器 16ms刷新一次（60帧）
    moveTimer = new QTimer(this);
    connect(moveTimer, &QTimer::timeout, this, [=, this]()
            {
                int dx = 0;
                int dy = 0;
                // 计算移动偏移并更新人物朝向
                if (keyW) { dy -= moveSpeed; if(!playerFrames[currentCharacter][1].isNull()) player->setPixmap(playerFrames[currentCharacter][1]); }
                if (keyS) { dy += moveSpeed; if(!playerFrames[currentCharacter][3].isNull()) player->setPixmap(playerFrames[currentCharacter][3]); }
                if (keyA) { dx -= moveSpeed; if(!playerFrames[currentCharacter][0].isNull()) player->setPixmap(playerFrames[currentCharacter][0]); }
                if (keyD) { dx += moveSpeed; if(!playerFrames[currentCharacter][2].isNull()) player->setPixmap(playerFrames[currentCharacter][2]); }

                // ========== 碰撞检测与边界限制 ==========
                QRectF mapRange = mapScene->sceneRect();
                QRectF playerSize = player->boundingRect();
                
                // 1. 临时目标坐标，先应用场景最外层边界限制（平滑贴边）
                qreal nextX = player->x() + dx;
                qreal nextY = player->y() + dy;

                if(nextX < mapRange.left()) nextX = mapRange.left();
                if(nextX + playerSize.width() > mapRange.right()) nextX = mapRange.right() - playerSize.width();
                if(nextY < mapRange.top()) nextY = mapRange.top();
                if(nextY + playerSize.height() > mapRange.bottom()) nextY = mapRange.bottom() - playerSize.height();

                // 2. 定义角色碰撞箱 (相对于人物左上角偏移和大小)
                // 取人物偏下方的区域，避免头碰到墙壁就卡住
                // 按精灵图尺寸缩放，适配不同角色
                qreal spriteW = playerSize.width();
                qreal spriteH = playerSize.height();
                qreal colOffsetX = spriteW * 6.0 / 32.0;
                qreal colOffsetY = spriteH * 16.0 / 32.0;
                qreal colWidth = spriteW * 20.0 / 32.0;
                qreal colHeight = spriteH * 15.0 / 32.0;

                // 辅助函数：检查指定坐标的碰撞箱是否全部覆盖在Walkable格子上
                auto checkWalkable = [&](qreal tx, qreal ty) {
                    qreal tileSize = 16.0;
                    int tlX = static_cast<int>(std::floor((tx + colOffsetX) / tileSize));
                    int tlY = static_cast<int>(std::floor((ty + colOffsetY) / tileSize));
                    int trX = static_cast<int>(std::floor((tx + colOffsetX + colWidth - 1) / tileSize));
                    int trY = static_cast<int>(std::floor((ty + colOffsetY) / tileSize));
                    int blX = static_cast<int>(std::floor((tx + colOffsetX) / tileSize));
                    int blY = static_cast<int>(std::floor((ty + colOffsetY + colHeight - 1) / tileSize));
                    int brX = static_cast<int>(std::floor((tx + colOffsetX + colWidth - 1) / tileSize));
                    int brY = static_cast<int>(std::floor((ty + colOffsetY + colHeight - 1) / tileSize));

                    auto& mapSys = m_engine.GetWorldMap().GetMapSystem();
                    return mapSys.isWalkable(tlX, tlY) &&
                           mapSys.isWalkable(trX, trY) &&
                           mapSys.isWalkable(blX, blY) &&
                           mapSys.isWalkable(brX, brY);
                };

                // 3. 滑动碰撞逻辑：分别验证 X 轴和 Y 轴
                qreal finalX = player->x();
                qreal finalY = player->y();

                if (nextX != player->x() && checkWalkable(nextX, player->y())) {
                    finalX = nextX;
                }
                if (nextY != player->y() && checkWalkable(finalX, nextY)) {
                    finalY = nextY;
                }

                // 赋值限制后的坐标
                player->setPos(finalX, finalY);
                // 同步脚底锚点
                playerLogicalPos = QPointF(finalX + playerSize.width() / 2.0, finalY + playerSize.height());

                // 镜头跟随玩家居中
                view->centerOn(player);

                // 检测是否靠近商店青色桌子（NPC 对象位置）
                if (!shopDialogActive && m_shopWindow == nullptr) {
                    QPointF playerCenter(player->x() + playerSize.width() / 2.0,
                                         player->y() + playerSize.height() / 2.0);
                    qreal dist = QLineF(playerCenter, shopTableCenter).length();
                    qDebug() << "playerCenter:" << playerCenter << "shopTableCenter:" << shopTableCenter << "dist:" << dist;
                    if (dist < 80.0) {
                        shopDialogActive = true;
                        m_shopDialog = new ShopDialog(this);
                        connect(m_shopDialog, &ShopDialog::accepted, this, [this]() {
                            if (m_shopWindow == nullptr) {
                                m_shopWindow = new ShopWindow(&m_engine, this);
                                QString itemsDir = QString(PROJECT_DATA_DIR) + "/items";
                                m_shopWindow->loadItemsFromDirectory(itemsDir);
                                m_shopWindow->show();
                                connect(m_shopWindow, &QWidget::destroyed, this, [this]() {
                                    m_shopWindow = nullptr;
                                });
                            } else {
                                m_shopWindow->show();
                                m_shopWindow->raise();
                            }
                        });
                        connect(m_shopDialog, &ShopDialog::rejected, this, [this]() {
                            // 拒绝，不做任何事
                        });
                        connect(m_shopDialog, &QDialog::finished, this, [this]() {
                            shopDialogActive = false;
                            m_shopDialog = nullptr;
                        });
                        m_shopDialog->show();
                    }
                }
            });
    moveTimer->start(16);
}

MainWindow::~MainWindow()
{

    delete ui;
}

// 按键按下触发
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_W: keyW = true; break;
    case Qt::Key_A: keyA = true; break;
    case Qt::Key_S: keyS = true; break;
    case Qt::Key_D: keyD = true; break;
    case Qt::Key_T:
        currentCharacter = (currentCharacter + 1) % 2;
        player->setPixmap(playerFrames[currentCharacter][3]); // 切换后默认朝下
        // 以脚底锚点为基准重算左上角，避免切换时视觉偏移
        {
            qreal newW = player->boundingRect().width();
            qreal newH = player->boundingRect().height();
            player->setPos(playerLogicalPos.x() - newW / 2.0, playerLogicalPos.y() - newH);
        }
        break;
    case Qt::Key_Y:
        if (shopDialogActive && m_shopDialog) {
            m_shopDialog->accept();
        }
        break;
    case Qt::Key_N:
        if (shopDialogActive && m_shopDialog) {
            m_shopDialog->reject();
        }
        break;
    default:
        QMainWindow::keyPressEvent(event);
        break;
    }
}

// 按键松开触发
void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_W: keyW = false; break;
    case Qt::Key_A: keyA = false; break;
    case Qt::Key_S: keyS = false; break;
    case Qt::Key_D: keyD = false; break;
    default:
        QMainWindow::keyReleaseEvent(event);
        break;
    }
}
