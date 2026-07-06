#pragma once

#include "Common.h"

  // 新增：切换地图函数需要QPointF类型

namespace Ui {
class MainWindow;
}

#include <QGraphicsPixmapItem>
#include <QPixmap>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    // 键盘按下事件声明
    void keyPressEvent(QKeyEvent *event) override;
    // 键盘松开事件声明
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    Ui::MainWindow *ui;
    QGraphicsScene *mapScene;
    QGraphicsView *view;

    // 玩家贴图对象
    QGraphicsPixmapItem *player;
    // 存储玩家的4个朝向贴图 (0左, 1上, 2右, 3下)
    QPixmap playerFrames[4];
    // 移动速度（只声明，不赋值）
    int moveSpeed;
    // 按键状态标记（只声明，不赋值）
    bool keyW;
    bool keyA;
    bool keyS;
    bool keyD;
    // 刷新移动定时器
    QTimer *moveTimer;
};

