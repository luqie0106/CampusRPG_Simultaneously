#pragma once

#include "Common.h"
#include "GameEngine.h"

class ShopWindow;
class BackpackWindow;
class CharacterSelectDialog;
class QLabel;
class QGraphicsView;
class QGraphicsScene;
class QProgressBar;
class QVBoxLayout;

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

    // 事件过滤器，用于处理鼠标点击事件
    bool eventFilter(QObject *obj, QEvent *event) override;

    // 窗口关闭事件，用于自动存档
    void closeEvent(QCloseEvent *event) override;

    // ========== 地图实体管理 ==========
    QMap<int, QGraphicsItem*> interactableGraphics;
    void updateEntityFacing(int entityId, const QPointF& playerPx);

private:
    Ui::MainWindow *ui;
    QGraphicsScene *mapScene;
    QGraphicsView *view;
    QGraphicsView *bigMapView; // 全屏大地图视图

    // 玩家贴图对象
    QGraphicsPixmapItem *player;
    // 存储三个角色的4个朝向贴图 [角色索引][方向] (方向: 0左, 1上, 2右, 3下)
    QPixmap playerFrames[3][4];
    // 当前角色索引 (0=男角色, 1=Alex, 2=Adam)
    int currentCharacter;
    // 玩家逻辑位置（以脚底为锚点，避免切换角色时视觉偏移）
    QPointF playerLogicalPos;
    // 移动速度（只声明，不赋值）
    int moveSpeed;
    // 按键状态标记（只声明，不赋值）
    bool keyW;
    bool keyA;
    bool keyS;
    bool keyD;
    bool keySpace;
    // 刷新移动定时器
    QTimer *moveTimer;

    // 自动寻路状态
    std::vector<GamePoint> autoPath;
    size_t currentPathIndex;

    // 游戏引擎实例（持有 WorldMap、碰撞数据、玩家状态等）
    GameEngine m_engine;

    // 统一交互系统
    QWidget *interactionWidget;
    QVBoxLayout *interactionLayout;
    std::vector<InteractableInfo> currentInteractables;
    int selectedInteractionIndex;
    void updateInteractionUI();
    
    ShopWindow *m_shopWindow;

    // 背包交互
    BackpackWindow *m_backpackWindow;

    // 角色选择
    CharacterSelectDialog *m_charSelectDialog;

    // ========== 战斗 UI ==========
    QProgressBar *playerHealthBar;
    QProgressBar *enemyHealthBar;
    QProgressBar *enemyStaggerBar;
    std::vector<QString> battleActions;
    void updateBattleUI();

    // ========== 装备耐久度 UI ==========
    QWidget *equipmentWidget;
    QVBoxLayout *equipmentLayout;
    QLabel *equipLabels[5];
    void updateEquipmentUI();

public slots:
    // 用于接收 BackpackWindow 传来的信号
    void onBattleItemUsed(const QString& resultLog);
};

