#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QtMapLoader.h"
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 初始化场景和视图
    mapScene = new QGraphicsScene(this);
    view = new QGraphicsView(mapScene, this);
    
    // 像素风游戏通常关闭抗锯齿，背景设为黑色
    view->setRenderHint(QPainter::Antialiasing, false);
    view->setBackgroundBrush(Qt::black);

    // 把视图塞满主窗口
    setCentralWidget(view);

    // 测试：加载第一张地图 classroom1.json，使用 CMake 传入的绝对物理路径
    QString mapPath = QString(PROJECT_DATA_DIR) + "/maps/classroom1.json";
    QtMapLoader::LoadMapToScene(mapPath, mapScene);

    // 默认放大3倍，方便看清 16x16 像素小人
    view->scale(3.0, 3.0);
}

MainWindow::~MainWindow()
{
    delete ui;
}
