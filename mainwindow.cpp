#include "Common.h"

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QtMapLoader.h"

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

    ui->setupUi(this);

    // 初始化场景与视图
    mapScene = new QGraphicsScene(this);
    view = new QGraphicsView(mapScene, this);
    this->setCentralWidget(view);

    // 修复 Mac 下按键无反应的问题（焦点与输入法拦截）
    this->setFocusPolicy(Qt::StrongFocus);
    view->setFocusPolicy(Qt::NoFocus);
    this->setAttribute(Qt::WA_InputMethodEnabled, false);

    // ========== 无缝加载大世界地图 ==========
    QString worldPath = QString(PROJECT_DATA_DIR) + "/maps/game.world";
    qDebug() << "开始加载大世界:" << worldPath;
    QtMapLoader::LoadWorldToScene(worldPath, mapScene);
    
    // 锁死整个大世界地图边界
    mapScene->setSceneRect(mapScene->itemsBoundingRect());

    // ========== 玩家贴图切片与初始化 ==========
    // TODO: 目前放了一个占位路径，请将你的贴图放入项目中并在此处替换正确的路径
    QPixmap fullSpriteSheet(":/data/tiles/player_sprite.png"); 
    int frameW = fullSpriteSheet.width() / 24; 
    int frameH = fullSpriteSheet.height();
    
    if (!fullSpriteSheet.isNull() && frameW > 0 && frameH > 0) {
        playerFrames[0] = fullSpriteSheet.copy(0, 0, frameW, frameH);            // 左
        playerFrames[1] = fullSpriteSheet.copy(6 * frameW, 0, frameW, frameH);   // 上
        playerFrames[2] = fullSpriteSheet.copy(12 * frameW, 0, frameW, frameH);  // 右
        playerFrames[3] = fullSpriteSheet.copy(18 * frameW, 0, frameW, frameH);  // 下
    } else {
        qWarning() << "未能加载玩家精灵图，或者尺寸计算错误！请检查路径。";
    }

    player = new QGraphicsPixmapItem();
    if (!fullSpriteSheet.isNull()) {
        player->setPixmap(playerFrames[3]); // 默认朝下
    }
    player->setZValue(10); // 层级高于地图，不会被瓦片遮挡
    mapScene->addItem(player);
    player->setPos(0, -200); // 初始出生坐标 (在 outside 的安全区域)

    // 移动定时器 16ms刷新一次（60帧）
    moveTimer = new QTimer(this);
    connect(moveTimer, &QTimer::timeout, this, [=, this]()
            {
                int dx = 0;
                int dy = 0;
                // 计算移动偏移并更新人物朝向
                if (keyW) { dy -= moveSpeed; if(!playerFrames[1].isNull()) player->setPixmap(playerFrames[1]); }
                if (keyS) { dy += moveSpeed; if(!playerFrames[3].isNull()) player->setPixmap(playerFrames[3]); }
                if (keyA) { dx -= moveSpeed; if(!playerFrames[0].isNull()) player->setPixmap(playerFrames[0]); }
                if (keyD) { dx += moveSpeed; if(!playerFrames[2].isNull()) player->setPixmap(playerFrames[2]); }

                // ========== 地图边界限制，防止跑出地图 ==========
                QRectF mapRange = mapScene->sceneRect();
                QRectF playerSize = player->boundingRect();
                qreal targetX = player->x() + dx;
                qreal targetY = player->y() + dy;

                // 左右边界限制
                if(targetX < mapRange.left())
                    targetX = mapRange.left();
                if(targetX + playerSize.width() > mapRange.right())
                    targetX = mapRange.right() - playerSize.width();

                // 上下边界限制
                if(targetY < mapRange.top())
                    targetY = mapRange.top();
                if(targetY + playerSize.height() > mapRange.bottom())
                    targetY = mapRange.bottom() - playerSize.height();

                // 赋值限制后的坐标
                player->setPos(targetX, targetY);

                // 镜头跟随玩家居中
                view->centerOn(player);

                // 现已采用无缝大地图，无需传送门检测
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
