#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QBrush>
#include <QColor>
#include <QFile>
#include <QDebug>
#include <QThread>
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

    // 创建蓝色玩家方块 30×30
    player = new QGraphicsRectItem(0, 0, 30, 30);
    player->setBrush(QBrush(QColor(0, 120, 255)));
    player->setZValue(10); // 方块层级高于地图，不会被瓦片遮挡
    mapScene->addItem(player);
    player->setPos(0, -200); // 初始出生坐标 (在 outside 的安全区域)

    // 移动定时器 16ms刷新一次（60帧）
    moveTimer = new QTimer(this);
    connect(moveTimer, &QTimer::timeout, this, [=, this]()
            {
                int dx = 0;
                int dy = 0;
                // 计算移动偏移
                if (keyW) dy -= moveSpeed;
                if (keyS) dy += moveSpeed;
                if (keyA) dx -= moveSpeed;
                if (keyD) dx += moveSpeed;

                // ========== 新增：地图边界限制，防止跑出地图 ==========
                QRectF mapRange = mapScene->sceneRect();
                QRectF playerSize = player->rect();
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
