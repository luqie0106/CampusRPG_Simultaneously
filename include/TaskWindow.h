#pragma once

#include "Common.h"
#include "GameEngine.h"

class TaskWindow : public QWidget {
    Q_OBJECT
public:
    explicit TaskWindow(GameEngine *engine, QWidget *parent = nullptr);
    ~TaskWindow();

    void refreshTasks();

signals:
    // 用户点击 [追踪] 按钮时发出，携带任务 ID
    void trackTask(int taskId);

protected:
    // 捕获 J / ESC 键，避免焦点被抢后无法关闭窗口
    void keyPressEvent(QKeyEvent *event) override;

private:
    void buildUI();

    GameEngine *m_engine;
    QWidget *m_scrollContent;
    QVBoxLayout *m_scrollLayout;
};
