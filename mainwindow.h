#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QMainWindow>
#include <QGraphicsRectItem>
#include <QKeyEvent>
#include <QTimer>
#include <QPointF>  // 新增：切换地图函数需要QPointF类型

namespace Ui {
class MainWindow;
}

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

    // 玩家方块对象
    QGraphicsRectItem *player;
    // 移动速度（只声明，不赋值）
    int moveSpeed;
    // 按键状态标记（只声明，不赋值）
    bool keyW;
    bool keyA;
    bool keyS;
    bool keyD;
    // 刷新移动定时器
    QTimer *moveTimer;

    // ========== 下面是新增的传送门相关成员，和cpp完全对应 ==========
    QGraphicsRectItem *blackMask;        // 黑屏过渡遮罩
    void checkTeleport();                // 每帧检测传送门坐标
    void switchMap(const QString& mapPath, QPointF spawnPos); // 切换地图函数
};

#endif // MAINWINDOW_H