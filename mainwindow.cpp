#include "Common.h"

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QtMapLoader.h"
#include "ShopWindow.h"
#include "BackpackWindow.h"
#include "CharacterSelectDialog.h"

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
    m_backpackWindow = nullptr;
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

    // ========== 战斗 UI ==========
    playerHealthBar = new QProgressBar(this);
    playerHealthBar->setFixedSize(300, 20);
    playerHealthBar->setTextVisible(false);
    playerHealthBar->setStyleSheet(
        "QProgressBar { border: 1px solid black; border-radius: 5px; background-color: #555555; }"
        "QProgressBar::chunk { background-color: #4CAF50; border-radius: 5px; }"
    );
    playerHealthBar->show();

    enemyHealthBar = new QProgressBar(this);
    enemyHealthBar->setFixedSize(300, 20);
    enemyHealthBar->setTextVisible(false);
    enemyHealthBar->setStyleSheet(
        "QProgressBar { border: 1px solid black; border-radius: 5px; background-color: #555555; }"
        "QProgressBar::chunk { background-color: #f44336; border-radius: 5px; }"
    );
    enemyHealthBar->hide();

    enemyStaggerBar = new QProgressBar(this);
    enemyStaggerBar->setFixedSize(300, 10);
    enemyStaggerBar->setTextVisible(false);
    enemyStaggerBar->setStyleSheet(
        "QProgressBar { border: 1px solid black; border-radius: 3px; background-color: #555555; }"
        "QProgressBar::chunk { background-color: #FFEB3B; border-radius: 3px; }"
    );
    enemyStaggerBar->hide();

    equipmentWidget = new QWidget(this);
    equipmentLayout = new QVBoxLayout(equipmentWidget);
    equipmentLayout->setContentsMargins(10, 10, 10, 10);
    equipmentLayout->setSpacing(5);
    for (int i = 0; i < 5; ++i) {
        equipLabels[i] = new QLabel(this);
        equipLabels[i]->setStyleSheet("color: white; background-color: rgba(0, 0, 0, 150); padding: 5px; border-radius: 4px; font-weight: bold; font-size: 14px;");
        equipLabels[i]->hide();
        equipmentLayout->addWidget(equipLabels[i]);
    }
    equipmentWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    equipmentWidget->show();

    battleActions = {"[攻击]", "[使用物品]", "[逃跑]"};

    // ========== 初始化游戏引擎 ==========
    m_engine.Init();

    // 读档逻辑
    m_engine.LoadGame();
    bool hasSavedGame = (m_engine.GetState() == GameState::InGame);

    if (!hasSavedGame) {
        // ========== 角色选择对话框 ==========
        m_charSelectDialog = new CharacterSelectDialog(&m_engine, this);
        m_charSelectDialog->exec();
        if (!m_charSelectDialog->isPlayerCreated()) {
            // 用户取消或未创建角色，直接退出
            QTimer::singleShot(0, this, &QMainWindow::close);
            return;
        }
        // 根据选择的职业设置对应的角色精灵
        // 1=Athlete->角色0, 2=Nerd->角色1, 3=Steve->角色2
        currentCharacter = m_charSelectDialog->selectedClass() - 1;
        delete m_charSelectDialog;
        m_charSelectDialog = nullptr;
    } else {
        // 如果有存档，使用默认的贴图（因为存档尚未保存角色贴图信息）
        currentCharacter = 2; // 默认 Steve
    }

    // ========== 无缝加载大世界地图（同步碰撞数据和实体到引擎）==========
    QString worldPath = QString(PROJECT_DATA_DIR) + "/maps/game.world";
    qDebug() << "开始加载大世界:" << worldPath;
    QtMapLoader::LoadWorldToScene(worldPath, mapScene, &m_engine.GetWorldMap());

    for (QGraphicsItem* item : mapScene->items()) {
        if (!item->data(0).isNull()) {
            interactableGraphics[item->data(0).toInt()] = item;
        }
    }

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
    if (!playerFrames[currentCharacter][3].isNull()) {
        player->setPixmap(playerFrames[currentCharacter][3]); // 默认朝下
    }
    player->setZValue(10); // 层级高于地图，不会被瓦片遮挡
    mapScene->addItem(player);
    GamePoint pos;
    if (hasSavedGame) {
        pos = m_engine.GetWorldMap().GetPlayerPos();
    } else {
        pos = m_engine.GetWorldMap().GetSpawnPoint();
        m_engine.GetWorldMap().SetPlayerPos(pos); // 同步引擎位置
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
                if (m_engine.GetState() == GameState::Battle) {
                    autoPath.clear();
                } else if (!autoPath.empty() && currentPathIndex < autoPath.size()) {
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
                // 取人物偏下方的区域，缩小宽度以通过1格(16px)宽的通道
                qreal spriteW = playerSize.width();
                qreal spriteH = playerSize.height();
                qreal colOffsetX = spriteW * 10.0 / 32.0;
                qreal colOffsetY = spriteH * 20.0 / 32.0;
                qreal colWidth = spriteW * 12.0 / 32.0;
                qreal colHeight = spriteH * 10.0 / 32.0;

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
                
                // 脱困机制：如果当前坐标已经卡在不可行走的区域，则暂时允许移动直到脱困
                bool currentlyStuck = !checkWalkable(player->x(), player->y());

                if (nextX != player->x() && (currentlyStuck || checkWalkable(nextX, player->y()))) {
                    finalX = nextX;
                }
                if (nextY != player->y() && (currentlyStuck || checkWalkable(finalX, nextY))) {
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

                // 战斗 UI 跟随
                if (m_engine.GetState() == GameState::Battle) {
                    enemyHealthBar->move(view->width() / 2 - enemyHealthBar->width() / 2, 10);
                    enemyStaggerBar->move(view->width() / 2 - enemyStaggerBar->width() / 2, 35);
                }
                
                // 常驻 UI 跟随
                playerHealthBar->move(view->width() / 2 - playerHealthBar->width() / 2, view->height() - 30);
                equipmentWidget->move(10, view->height() - equipmentWidget->sizeHint().height() - 40);

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
    
    if (m_engine.GetState() == GameState::Battle) {
        // 战斗状态：渲染快捷键提示
        QLayoutItem *child;
        while ((child = interactionLayout->takeAt(0)) != nullptr) {
            delete child->widget();
            delete child;
        }
        QLabel *label = new QLabel("F【攻击】  B【打开背包】  P【逃跑】", this);
        label->setFont(QFont("Arial", 16, QFont::Bold));
        label->setStyleSheet("color: yellow; background-color: transparent;");
        interactionLayout->addWidget(label);
        interactionWidget->adjustSize();
        
        int targetX = view->width() * 0.75 - interactionWidget->width() / 2.0;
        int targetY = view->height() * 0.75 - interactionWidget->height() / 2.0;
        interactionWidget->move(targetX, targetY);
        if (!interactionWidget->isVisible()) {
            interactionWidget->show();
            interactionWidget->raise();
        }
        return; // 战斗中不检测附近物品
    }
    
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
            // 如果是怪物，并且在附近，让怪物面对玩家
            if (info.type == InteractableType::Enemy || info.type == InteractableType::Boss) {
                updateEntityFacing(info.id, playerLogicalPos);
            }
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
            this->setFocus();
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
    if (event->key() == Qt::Key_Escape) {
        bool closedSomething = false;
        if (bigMapView && bigMapView->isVisible()) {
            bigMapView->hide();
            closedSomething = true;
        }
        if (m_shopWindow != nullptr) {
            m_shopWindow->close();
            closedSomething = true;
        }
        if (m_backpackWindow != nullptr) {
            m_backpackWindow->close();
            closedSomething = true;
        }
        if (closedSomething) return;
    }

    if (m_engine.GetState() == GameState::Battle) {
        if (event->key() == Qt::Key_F) {
            std::string result = m_engine.BattlePlayerAttack();
            updateBattleUI();
            updateEquipmentUI();
            if (result.find("被击败了") != std::string::npos || result.find("倒下") != std::string::npos) {
                QMessageBox::information(this, "战斗结束", QString::fromStdString(result));
            } else if (result.find("获得了") != std::string::npos || result.find("胜利") != std::string::npos) {
                QMessageBox::information(this, "战斗胜利", QString::fromStdString(result));
            }
        } else if (event->key() == Qt::Key_B) {
            if (m_backpackWindow == nullptr) {
                m_backpackWindow = new BackpackWindow(&m_engine, nullptr);
                connect(m_backpackWindow, &BackpackWindow::battleItemUsed, this, &MainWindow::onBattleItemUsed);
                connect(m_backpackWindow, &QObject::destroyed, this, [this]() { m_backpackWindow = nullptr; updateEquipmentUI(); });
                m_backpackWindow->show();
            } else {
                m_backpackWindow->raise();
                m_backpackWindow->activateWindow();
            }
        } else if (event->key() == Qt::Key_P) {
            m_engine.BattleFlee();
            updateBattleUI();
            updateEquipmentUI();
        }
        return; // 战斗期间拦截其他按键
    }

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
                    m_engine.ExecuteInteraction(info.defaultInteraction, info.id);
                    m_shopWindow->loadItemsFromEngine();
                    m_shopWindow->show();
                    connect(m_shopWindow, &QWidget::destroyed, this, [this]() {
                        m_shopWindow = nullptr;
                        updateEquipmentUI();
                    });
                    if (interactionWidget->isVisible()) {
                        interactionWidget->hide();
                        this->setFocus();
                    }
                }
            } else {
                std::string result = m_engine.ExecuteInteraction(info.defaultInteraction, info.id);
                qDebug() << "Interaction result:" << QString::fromStdString(result);
                
                // 怪物朝向玩家：计算相对位置，翻转怪物贴图
                if (info.defaultInteraction == InteractionType::StartBattle && interactableGraphics.contains(info.id)) {
                    QGraphicsItem* monsterItem = interactableGraphics[info.id];
                    if (monsterItem) {
                        qreal px = playerLogicalPos.x();
                        qreal mx = monsterItem->x() + monsterItem->boundingRect().width() / 2.0;
                        QTransform transform;
                        // 假设默认朝左，如果玩家在右侧，则翻转贴图使其朝右
                        if (px > mx) {
                            transform.scale(-1, 1);
                            transform.translate(-monsterItem->boundingRect().width(), 0);
                        }
                        monsterItem->setTransform(transform);
                    }
                }

                if (info.defaultInteraction == InteractionType::StartBattle) {
                    updateBattleUI();
                }
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
    case Qt::Key_B:
        if (m_backpackWindow == nullptr) {
            m_backpackWindow = new BackpackWindow(&m_engine, nullptr);
            connect(m_backpackWindow, &QObject::destroyed, this, [this]() { m_backpackWindow = nullptr; updateEquipmentUI(); });
            m_backpackWindow->show();
        } else {
            m_backpackWindow->raise();
            m_backpackWindow->activateWindow();
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

void MainWindow::updateBattleUI() {
    const Enemy* enemy = m_engine.GetCurrentEnemy();
    if (enemy) {
        enemyHealthBar->setMaximum(enemy->GetMaxHealth());
        enemyHealthBar->setValue(enemy->GetHealth());
        enemyHealthBar->show();

        if (enemy->IsBoss()) {
            enemyStaggerBar->setMaximum(static_cast<int>(enemy->GetMaxStaggerPoints()));
            enemyStaggerBar->setValue(static_cast<int>(enemy->GetCurrentStaggerPoints()));
            enemyStaggerBar->show();
        } else {
            enemyStaggerBar->hide();
        }
    }

    if (m_engine.GetState() != GameState::Battle) {
        // 战斗结束，延迟隐藏怪物状态并恢复菜单
        enemyHealthBar->hide();
        enemyStaggerBar->hide();
        
        // 检查并同步复活/传送导致的位置突变
        GamePoint enginePos = m_engine.GetWorldMap().GetPlayerPos();
        QPointF expectedPx(enginePos.x * 16.0, enginePos.y * 16.0);
        QPointF currentPx = player->pos();
        
        if (std::abs(expectedPx.x() - currentPx.x()) > 16.0 || 
            std::abs(expectedPx.y() - currentPx.y()) > 16.0) {
            player->setPos(expectedPx);
            playerLogicalPos = QPointF(expectedPx.x() + player->boundingRect().width() / 2.0, 
                                       expectedPx.y() + player->boundingRect().height());
            view->centerOn(player);
            autoPath.clear();
        }
        
        currentInteractables.clear();
        interactionWidget->hide();
        this->setFocus();
        updateInteractionUI();
    }
}

void MainWindow::updateEquipmentUI() {
    if (!m_engine.GetPlayer()) return;
    auto playerChar = m_engine.GetPlayer();
    
    // Update player health bar
    playerHealthBar->setMaximum(playerChar->GetMaxHealth());
    playerHealthBar->setValue(playerChar->GetHealth());
    
    EquipSlot eqSlots[] = { EquipSlot::Weapon, EquipSlot::Head, EquipSlot::Body, EquipSlot::Legs, EquipSlot::Feet };
    QString slotNames[] = { "武器", "头盔", "胸甲", "裤腿", "靴子" };
    int labelIdx = 0;
    
    for (int i = 0; i < 5; ++i) {
        auto eq = playerChar->GetEquipmentAt(eqSlots[i]);
        if (eq) {
            auto equipment = std::dynamic_pointer_cast<Equipment>(eq);
            if (equipment) {
                equipLabels[labelIdx]->setText(QString("%1 耐久: %2/%3")
                                               .arg(slotNames[i])
                                               .arg(equipment->GetDurability())
                                               .arg(equipment->GetMaxDurability()));
                equipLabels[labelIdx]->show();
                labelIdx++;
            }
        }
    }
    
    for (int i = labelIdx; i < 5; ++i) {
        equipLabels[i]->hide();
    }
    equipmentWidget->adjustSize();
}

void MainWindow::onBattleItemUsed(const QString& resultLog) {
    updateBattleUI();
    updateEquipmentUI();
}

void MainWindow::updateEntityFacing(int entityId, const QPointF& playerPx) {
    if (!interactableGraphics.contains(entityId)) return;
    QGraphicsItem* item = interactableGraphics[entityId];
    if (item->data(1).toString() != "Monster") return;

    QGraphicsPixmapItem* pixItem = dynamic_cast<QGraphicsPixmapItem*>(item);
    if (!pixItem) return;

    bool isBoss = item->data(2).toBool();

    QPointF enemyCenter = pixItem->sceneBoundingRect().center();
    qreal dx = playerPx.x() - enemyCenter.x();
    qreal dy = playerPx.y() - enemyCenter.y();

    int w = pixItem->pixmap().width();
    int h = pixItem->pixmap().height();
    QPixmap newPix(w, h);
    newPix.fill(Qt::transparent);
    QPainter p(&newPix);
    p.fillRect(0, 0, w, h, QColor(isBoss ? 128 : 255, 0, isBoss ? 128 : 0, 200));
    
    p.setBrush(Qt::black);
    p.setPen(Qt::NoPen);

    if (qAbs(dx) > qAbs(dy)) {
        if (dx > 0) {
            p.drawEllipse(w - 6, h / 2 - 4, 4, 4);
            p.drawEllipse(w - 6, h / 2 + 4, 4, 4);
        } else {
            p.drawEllipse(2, h / 2 - 4, 4, 4);
            p.drawEllipse(2, h / 2 + 4, 4, 4);
        }
    } else {
        if (dy > 0) {
            p.drawEllipse(w / 2 - 4, h - 6, 4, 4);
            p.drawEllipse(w / 2 + 4, h - 6, 4, 4);
        } else {
            p.setBrush(Qt::darkGray);
            p.drawEllipse(w / 2 - 4, 2, 4, 4);
            p.drawEllipse(w / 2 + 4, 2, 4, 4);
        }
    }
    
    pixItem->setPixmap(newPix);
}
