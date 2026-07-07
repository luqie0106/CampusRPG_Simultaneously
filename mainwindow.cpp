#include "Common.h"

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QtMapLoader.h"
#include "ShopWindow.h"

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
    m_shopWindow = nullptr;
    currentPathIndex = 0;
    selectedInteractionIndex = 0;

    ui->setupUi(this);

    // 初始化场景与视图
    mapScene = new QGraphicsScene(this);
    view = new QGraphicsView(mapScene, this);
    this->setCentralWidget(view);

    // 修复 Mac 下按键无反应的问题（焦点与输入法拦截）
    this->setFocusPolicy(Qt::StrongFocus);
    view->setFocusPolicy(Qt::NoFocus);
    this->setAttribute(Qt::WA_InputMethodEnabled, false);

    // 事件过滤器，用于处理右键寻路
    view->viewport()->installEventFilter(this);

    // 初始化全屏大地图
    bigMapView = new QGraphicsView(mapScene, this);
    bigMapView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    bigMapView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    bigMapView->setFocusPolicy(Qt::NoFocus);
    bigMapView->viewport()->installEventFilter(this);
    bigMapView->hide();

    // 统一交互系统 UI
    interactionWidget = new QWidget(this);
    interactionWidget->setStyleSheet("background-color: rgba(0, 0, 0, 150); border-radius: 8px;");
    interactionLayout = new QVBoxLayout(interactionWidget);
    interactionLayout->setContentsMargins(10, 10, 10, 10);
    interactionLayout->setSpacing(5);
    interactionWidget->hide();

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

    // 角色2: Adam_idle_16x16.png
    // 实际尺寸: 64x32, 单行 4 帧, 每帧 16x32
    // 帧序: 0右, 1上, 2左, 3下
    QPixmap char2Sheet(":/data/data/Characters_free/Adam_idle_16x16.png");
    const int c2W = 16;
    const int c2H = 32;

    if (!char2Sheet.isNull()) {
        playerFrames[2][0] = char2Sheet.copy(c2W * 2, 0, c2W, c2H);  // 左
        playerFrames[2][1] = char2Sheet.copy(c2W * 1, 0, c2W, c2H);  // 上
        playerFrames[2][2] = char2Sheet.copy(c2W * 0, 0, c2W, c2H);  // 右
        playerFrames[2][3] = char2Sheet.copy(c2W * 3, 0, c2W, c2H);  // 下
    } else {
        qWarning() << "未能加载角色2(Adam)精灵图，请检查 qrc 路径与构建！";
    }

    player = new QGraphicsPixmapItem();
    if (!playerFrames[0][3].isNull()) {
        player->setPixmap(playerFrames[0][3]); // 默认角色0朝下
    }
    player->setZValue(10); // 层级高于地图，不会被瓦片遮挡
    mapScene->addItem(player);
    
    // 读档逻辑
    m_engine.LoadGame();
    GamePoint pos;
    if (m_engine.GetState() == GameState::InGame) {
        pos = m_engine.GetWorldMap().GetPlayerPos();
    } else {
        // 显式创建默认角色，防止后续因 m_player 为空而在访问时报错
        m_engine.CreatePlayer("Steve", 3);
        pos = m_engine.GetWorldMap().GetSpawnPoint();
    }
    
    // 获取真实坐标并换算为像素坐标 (瓦片大小默认 16)
    QPointF spawnPx(pos.x * 16.0, pos.y * 16.0);
    player->setPos(spawnPx);
    // 记录脚底锚点位置（top-left + 宽度一半, 高度），切换角色时以此为基准重算
    playerLogicalPos = QPointF(spawnPx.x() + 32.0 / 2.0, spawnPx.y() + 32.0);

    // 移动定时器 16ms刷新一次（60帧）
    moveTimer = new QTimer(this);
    connect(moveTimer, &QTimer::timeout, this, [=, this]()
            {
                int dx = 0;
                int dy = 0;
                // 检查按键中断自动寻路
                if (keyW || keyS || keyA || keyD) {
                    autoPath.clear();
                }

                // 计算移动偏移并更新人物朝向
                if (!autoPath.empty() && currentPathIndex < autoPath.size()) {
                    // 自动寻路移动
                    GamePoint targetPoint = autoPath[currentPathIndex];
                    // 目标像素为瓦片中心
                    QPointF targetPx(targetPoint.x * 16.0 + 8.0, targetPoint.y * 16.0 + 8.0);
                    
                    // 以玩家 logicalPos 计算方向
                    qreal diffX = targetPx.x() - playerLogicalPos.x();
                    qreal diffY = targetPx.y() - playerLogicalPos.y();
                    qreal dist = std::sqrt(diffX * diffX + diffY * diffY);

                    if (dist <= moveSpeed) {
                        // 已经很接近当前节点，切换到下一个
                        currentPathIndex++;
                        if (currentPathIndex >= autoPath.size()) {
                            autoPath.clear();
                        }
                    } else {
                        // 按比例移动
                        qreal moveDistX = (diffX / dist) * moveSpeed;
                        qreal moveDistY = (diffY / dist) * moveSpeed;
                        dx = static_cast<int>(std::round(moveDistX));
                        dy = static_cast<int>(std::round(moveDistY));

                        // 更新朝向
                        if (std::abs(dx) > std::abs(dy)) {
                            if (dx > 0 && !playerFrames[currentCharacter][2].isNull()) player->setPixmap(playerFrames[currentCharacter][2]);
                            else if (dx < 0 && !playerFrames[currentCharacter][0].isNull()) player->setPixmap(playerFrames[currentCharacter][0]);
                        } else {
                            if (dy > 0 && !playerFrames[currentCharacter][3].isNull()) player->setPixmap(playerFrames[currentCharacter][3]);
                            else if (dy < 0 && !playerFrames[currentCharacter][1].isNull()) player->setPixmap(playerFrames[currentCharacter][1]);
                        }
                    }
                } else {
                    // 手动移动
                    if (keyW) { dy -= moveSpeed; if(!playerFrames[currentCharacter][1].isNull()) player->setPixmap(playerFrames[currentCharacter][1]); }
                    if (keyS) { dy += moveSpeed; if(!playerFrames[currentCharacter][3].isNull()) player->setPixmap(playerFrames[currentCharacter][3]); }
                    if (keyA) { dx -= moveSpeed; if(!playerFrames[currentCharacter][0].isNull()) player->setPixmap(playerFrames[currentCharacter][0]); }
                    if (keyD) { dx += moveSpeed; if(!playerFrames[currentCharacter][2].isNull()) player->setPixmap(playerFrames[currentCharacter][2]); }
                }

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
                // 同步后端引擎玩家坐标（以瓦片格子为单位）
                m_engine.GetWorldMap().SetPlayerPos(GamePoint(playerLogicalPos.x() / 16.0, playerLogicalPos.y() / 16.0));

                // 镜头跟随玩家居中
                view->centerOn(player);

                // 更新统一交互 UI
                if (m_shopWindow == nullptr) {
                    updateInteractionUI();
                } else {
                    if (interactionWidget->isVisible()) {
                        interactionWidget->hide();
                    }
                }
            });
    moveTimer->start(16);
}

MainWindow::~MainWindow()
{

    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (m_engine.GetState() == GameState::InGame) {
        m_engine.SaveGame();
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::updateInteractionUI() {
    if (!player) return;
    
    QPointF playerCenter(player->x() + player->boundingRect().width() / 2.0,
                         player->y() + player->boundingRect().height() / 2.0);

    // 获取附近目标（放宽至半径5格以包含更远的物体，后续再用像素过滤）
    std::vector<InteractableInfo> nearby = m_engine.CheckNearbyInteractables(5);
    std::vector<InteractableInfo> inRange;
    
    for (const auto& info : nearby) {
        // 将格坐标转换为像素坐标（假设16x16格）
        QPointF targetPx(info.pos.x * 16.0 + 8.0, info.pos.y * 16.0 + 8.0);
        qreal dist = QLineF(playerCenter, targetPx).length();
        if (dist <= 48.0) { // 3格像素距离
            inRange.push_back(info);
        }
    }

    // 判断列表是否有变化（简化判断：数量不同或ID不同则认为变化）
    bool changed = false;
    if (inRange.size() != currentInteractables.size()) {
        changed = true;
    } else {
        for (size_t i = 0; i < inRange.size(); ++i) {
            if (inRange[i].id != currentInteractables[i].id) {
                changed = true;
                break;
            }
        }
    }

    if (changed) {
        currentInteractables = inRange;
        selectedInteractionIndex = 0;

        // 清空布局
        QLayoutItem *child;
        while ((child = interactionLayout->takeAt(0)) != nullptr) {
            delete child->widget();
            delete child;
        }

        if (currentInteractables.empty()) {
            interactionWidget->hide();
        } else {
            // 重新填充列表
            for (size_t i = 0; i < currentInteractables.size(); ++i) {
                const auto& info = currentInteractables[i];
                QLabel *label = new QLabel(this);
                
                QString actionText;
                if (info.defaultInteraction == InteractionType::PickUpItem) {
                    actionText = "[拾取] ";
                } else if (info.defaultInteraction == InteractionType::EnterShop || info.defaultInteraction == InteractionType::EnterBlackMarket) {
                    actionText = "[商店] ";
                } else if (info.defaultInteraction == InteractionType::TalkToNPC) {
                    actionText = "[交谈] ";
                } else if (info.defaultInteraction == InteractionType::StartBattle) {
                    actionText = "[战斗] ";
                } else {
                    actionText = "[交互] ";
                }
                
                label->setText(actionText + QString::fromStdString(info.displayName));
                label->setFont(QFont("Arial", 14, QFont::Bold));
                
                if (static_cast<int>(i) == selectedInteractionIndex) {
                    label->setStyleSheet("color: yellow; background-color: rgba(255, 255, 255, 50);");
                } else {
                    label->setStyleSheet("color: white; background-color: transparent;");
                }
                interactionLayout->addWidget(label);
            }

            interactionWidget->adjustSize();
            
            // 位置强制：中心点位于屏幕正中心点与主窗口右下角的连线正中间
            // 主窗口大小：view->width(), view->height()
            int targetX = view->width() * 0.75 - interactionWidget->width() / 2.0;
            int targetY = view->height() * 0.75 - interactionWidget->height() / 2.0;
            interactionWidget->move(targetX, targetY);
            
            interactionWidget->show();
            interactionWidget->raise();
        }
    } else if (!currentInteractables.empty()) {
        // 如果没有变化，但是需要更新高亮（比如通过按键改变了 selectedInteractionIndex）
        for (int i = 0; i < interactionLayout->count(); ++i) {
            QWidget *w = interactionLayout->itemAt(i)->widget();
            if (QLabel *lbl = qobject_cast<QLabel*>(w)) {
                if (i == selectedInteractionIndex) {
                    lbl->setStyleSheet("color: yellow; background-color: rgba(255, 255, 255, 50);");
                } else {
                    lbl->setStyleSheet("color: white; background-color: transparent;");
                }
            }
        }
        
        // 动态调整位置（应对窗口大小变化）
        int targetX = view->width() * 0.75 - interactionWidget->width() / 2.0;
        int targetY = view->height() * 0.75 - interactionWidget->height() / 2.0;
        interactionWidget->move(targetX, targetY);
    }
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
        currentCharacter = (currentCharacter + 1) % 3;
        player->setPixmap(playerFrames[currentCharacter][3]); // 切换后默认朝下
        // 以脚底锚点为基准重算左上角，避免切换时视觉偏移
        {
            qreal newW = player->boundingRect().width();
            qreal newH = player->boundingRect().height();
            player->setPos(playerLogicalPos.x() - newW / 2.0, playerLogicalPos.y() - newH);
        }
        break;
    case Qt::Key_Up:
        if (!currentInteractables.empty()) {
            selectedInteractionIndex = (selectedInteractionIndex - 1 + currentInteractables.size()) % currentInteractables.size();
            updateInteractionUI();
        }
        break;
    case Qt::Key_Down:
        if (!currentInteractables.empty()) {
            selectedInteractionIndex = (selectedInteractionIndex + 1) % currentInteractables.size();
            updateInteractionUI();
        }
        break;
    case Qt::Key_F:
        if (!currentInteractables.empty() && selectedInteractionIndex >= 0 && selectedInteractionIndex < currentInteractables.size()) {
            const auto& info = currentInteractables[selectedInteractionIndex];
            
            if (info.defaultInteraction == InteractionType::EnterShop || info.defaultInteraction == InteractionType::EnterBlackMarket) {
                if (m_shopWindow == nullptr) {
                    m_shopWindow = new ShopWindow(&m_engine, this);
                    // 状态机会在 ExecuteInteraction 内部切换
                    m_engine.ExecuteInteraction(info.defaultInteraction, info.id);
                    m_shopWindow->loadItemsFromEngine();
                    m_shopWindow->show();
                    connect(m_shopWindow, &QWidget::destroyed, this, [this]() {
                        m_shopWindow = nullptr;
                    });
                    if (interactionWidget->isVisible()) {
                        interactionWidget->hide();
                    }
                }
            } else {
                std::string result = m_engine.ExecuteInteraction(info.defaultInteraction, info.id);
                qDebug() << "Interaction result:" << QString::fromStdString(result);
                // 强制刷新：由于物品可能被拾取/被移除，清空当前列表并立刻重新检测
                currentInteractables.clear();
                updateInteractionUI();
            }
        }
        break;
    case Qt::Key_M:
        if (bigMapView->isVisible()) {
            bigMapView->hide();
        } else {
            bigMapView->setGeometry(view->geometry());
            bigMapView->show();
            bigMapView->raise();
            bigMapView->fitInView(mapScene->sceneRect(), Qt::KeepAspectRatio);
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

// 事件过滤器，处理鼠标点击寻路
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        
        // 主视图的右键点击
        if (obj == view->viewport() && mouseEvent->button() == Qt::RightButton) {
            QPointF scenePos = view->mapToScene(mouseEvent->pos());
            int targetX = scenePos.x() / 16.0;
            int targetY = scenePos.y() / 16.0;

            QMenu menu;
            QAction *goAction = menu.addAction("前往");
            QAction *selectedAction = menu.exec(mouseEvent->globalPosition().toPoint());

            if (selectedAction == goAction) {
                int startX = playerLogicalPos.x() / 16.0;
                int startY = playerLogicalPos.y() / 16.0;
                
                auto checkNodeValid = [&](int x, int y) {
                    qreal targetPx_x = x * 16.0 + 8.0;
                    qreal targetPx_y = y * 16.0 + 8.0;
                    qreal tx = targetPx_x - player->boundingRect().width() / 2.0;
                    qreal ty = targetPx_y - player->boundingRect().height();

                    qreal spriteW = player->boundingRect().width();
                    qreal spriteH = player->boundingRect().height();
                    qreal colOffsetX = spriteW * 6.0 / 32.0;
                    qreal colOffsetY = spriteH * 16.0 / 32.0;
                    qreal colWidth = spriteW * 20.0 / 32.0;
                    qreal colHeight = spriteH * 15.0 / 32.0;

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

                auto path = m_engine.GetWorldMap().GetMapSystem().findPath(GamePoint(startX, startY), GamePoint(targetX, targetY), checkNodeValid);
                if (!path.empty()) {
                    autoPath = path;
                    currentPathIndex = 0;
                }
            }
            return true; // 拦截事件
        }
        // 大地图的左键点击
        else if (obj == bigMapView->viewport() && mouseEvent->button() == Qt::LeftButton) {
            QPointF scenePos = bigMapView->mapToScene(mouseEvent->pos());
            int targetX = scenePos.x() / 16.0;
            int targetY = scenePos.y() / 16.0;

            int startX = playerLogicalPos.x() / 16.0;
            int startY = playerLogicalPos.y() / 16.0;
            
            auto checkNodeValid = [&](int x, int y) {
                qreal targetPx_x = x * 16.0 + 8.0;
                qreal targetPx_y = y * 16.0 + 8.0;
                qreal tx = targetPx_x - player->boundingRect().width() / 2.0;
                qreal ty = targetPx_y - player->boundingRect().height();

                qreal spriteW = player->boundingRect().width();
                qreal spriteH = player->boundingRect().height();
                qreal colOffsetX = spriteW * 6.0 / 32.0;
                qreal colOffsetY = spriteH * 16.0 / 32.0;
                qreal colWidth = spriteW * 20.0 / 32.0;
                qreal colHeight = spriteH * 15.0 / 32.0;

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

            auto path = m_engine.GetWorldMap().GetMapSystem().findPath(GamePoint(startX, startY), GamePoint(targetX, targetY), checkNodeValid);
            if (!path.empty()) {
                autoPath = path;
                currentPathIndex = 0;
                bigMapView->hide(); // 寻路成功后关闭大地图
            }
            return true; // 拦截事件
        }
    }
    return QMainWindow::eventFilter(obj, event);
}
