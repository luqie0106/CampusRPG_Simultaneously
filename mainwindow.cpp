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

    // ========== 加载初始室外地图背景 ==========
    QFile testMap(":/data/maps/outside.json");
    if(testMap.exists())
    {
        qDebug() << "地图资源加载成功";
        QtMapLoader::LoadMapToScene(":/data/maps/outside.json", mapScene);
        
        // --- 修复：彻底锁死地图边界 ---
        // 防止由于玩家自带画笔导致的动态 sceneRect 不断膨胀（即缓慢穿墙Bug）
        mapScene->setSceneRect(mapScene->itemsBoundingRect());
    }
    else
    {
        qDebug() << "地图资源读取失败，请检查qrc和构建";
    }

    // ========== 黑屏过渡遮罩（传送切换用） ==========
    blackMask = new QGraphicsRectItem(mapScene->sceneRect());
    blackMask->setBrush(Qt::black);
    blackMask->setZValue(9999); // 最高层级盖住所有内容
    blackMask->hide();
    mapScene->addItem(blackMask);

    // 创建蓝色玩家方块 30×30
    player = new QGraphicsRectItem(0, 0, 30, 30);
    player->setBrush(QBrush(QColor(0, 120, 255)));
    player->setZValue(10); // 方块层级高于地图，不会被瓦片遮挡
    mapScene->addItem(player);
    player->setPos(200, 200); // 初始出生坐标

    // 移动定时器 16ms刷新一次（60帧）
    moveTimer = new QTimer(this);
    connect(moveTimer, &QTimer::timeout, this, [=]()
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

                // 每帧检测传送门
                checkTeleport();
            });
    moveTimer->start(16);
}

MainWindow::~MainWindow()
{
    delete blackMask; // 释放遮罩内存，防止泄漏
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

// 检测传送门坐标（坐标改到画面可视中间，扩大触发范围）
void MainWindow::checkTeleport()
{
    QPointF p = player->pos();
    // 室外地图传送点：画面中间区域，容易走到
    const qreal tpX = 400;
    const qreal tpY = 300;
    // 60像素超大触发范围，不会踩不到
    if(p.x() >= tpX && p.x() <= tpX + 60 && p.y() >= tpY && p.y() <= tpY + 60)
    {
        switchMap(":/data/maps/office.json", QPointF(80, 80));
    }

    // 办公室回程传送点（切回室外）
    const qreal backX = 60;
    const qreal backY = 60;
    if(p.x() >= backX && p.x() <= backX + 60 && p.y() >= backY && p.y() <= backY + 60)
    {
        switchMap(":/data/maps/outside.json", QPointF(420, 320));
    }
}
// 切换地图核心函数
void MainWindow::switchMap(const QString& mapPath, QPointF spawnPos)
{
    blackMask->show();
    view->update();
    QThread::msleep(200);

    mapScene->clear();
    // 加载新地图
    QtMapLoader::LoadMapToScene(mapPath, mapScene);

    // 重置玩家位置，重新添加到场景
    player->setPos(spawnPos);
    mapScene->addItem(player);
    // 遮罩放回顶层
    mapScene->addItem(blackMask);

    view->centerOn(player);
    blackMask->hide();
}