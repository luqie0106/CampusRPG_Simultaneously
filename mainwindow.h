#pragma once

#include "Common.h"
#include "GameEngine.h"

class ShopWindow;
class BackpackWindow;
class TaskWindow;
class CharacterSelectDialog;
class QLabel;
class QGraphicsView;
class QGraphicsScene;
class QProgressBar;
class QVBoxLayout;
class QTimer;
class QGraphicsItem;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // ── 任务 HUD 公开接口 ──────────────────────────────────────────
    // 将一条新任务通知推入队列，窗口将依次淡入淡出显示
    void enqueueTaskNotification(const QString& text);

    // 设置当前追踪任务（-1 表示取消追踪），立即刷新常驻面板
    void setTrackedTask(int taskId);

    // 打开大地图（等效 M 键），并在追踪任务目标位置绘制红色标记
    void openBigMap();

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
    
    // 任务系统交互
    TaskWindow *m_taskWindow;

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

    QLabel *m_gameTimeLabel = nullptr;

    // ========== 任务 HUD ==========
    QLabel  *m_newTaskNotifyLabel = nullptr;  // 新任务提示（淡入淡出轮播）
    QLabel  *m_trackedTaskLabel   = nullptr;  // 常驻追踪面板
    QTimer  *m_notifyTimer        = nullptr;  // 提示轮播定时器（2.5s/条）
    std::queue<QString> m_notifyQueue;        // 待显示的提示文字队列
    int      m_trackedTaskId      = -1;       // 当前追踪任务 ID（-1=无）

    // 大地图追踪标记（红色圆圈），需在清除/重建时管理生命周期
    QList<QGraphicsItem*> m_trackMarkers;

    // 内部辅助
    void _showNextNotification();               // Timer 触发：弹出队列下一项并播放动画
    void _updateTrackedTaskHUD();               // 刷新常驻面板文字
    void _clearMapTrackMarkers();               // 清除大地图上的所有追踪标记
    QGraphicsItem* _placeMapTrackMarkers(int taskId); // 为指定任务绘制目标敌人标记，返回生成的 marker（nullptr 表示无目标）

    // ── 寻路碰撞检测辅助（提取自 eventFilter，避免代码重复）──────────────────────
    bool isTileWalkable(int x, int y);

public slots:
    // 用于接收 BackpackWindow 传来的信号
    void onBattleItemUsed(const QString& resultLog);

private slots:
    // 包装 _showNextNotification() 供 Timer::timeout 连接（避免 overload 歧义）
    void _showNextNotification_slot();
};
