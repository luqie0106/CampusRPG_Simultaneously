#include "Common.h"
#include "../include/TaskWindow.h"

TaskWindow::TaskWindow(GameEngine *engine, QWidget *parent)
    : QWidget(parent), m_engine(engine) {
    buildUI();
}

TaskWindow::~TaskWindow() {}

// ── 键盘焦点修复 ──────────────────────────────────────────────────────────────
// TaskWindow 是 Top-Level 窗口，弹出后会抢占键盘焦点，导致 MainWindow 的
// keyPressEvent 收不到 J / ESC。在此捕获并直接处理关闭逻辑。
void TaskWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_J || event->key() == Qt::Key_Escape) {
        this->hide();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void TaskWindow::buildUI() {
    this->setFixedSize(600, 500);
    this->setStyleSheet("background-color: #2b2b2b; color: #f0f0f0; font-family: 'Microsoft YaHei';");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    QLabel *title = new QLabel("任务列表 (按 J 或 ESC 关闭)", this);
    title->setStyleSheet("font-size: 24px; font-weight: bold; color: #ffcc00;");
    title->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(title);

    QScrollArea *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet("QScrollArea { border: 1px solid #444; background: #333; }");
    
    m_scrollContent = new QWidget(scroll);
    m_scrollContent->setStyleSheet("background-color: transparent;");
    m_scrollLayout = new QVBoxLayout(m_scrollContent);
    m_scrollLayout->setAlignment(Qt::AlignTop);
    
    scroll->setWidget(m_scrollContent);
    mainLayout->addWidget(scroll);
}

void TaskWindow::refreshTasks() {
    // 清空旧布局
    QLayoutItem *child;
    while ((child = m_scrollLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }

    const auto& tasks = m_engine->GetTaskManager().GetTasks();
    bool hasAnyTask = false;

    for (const auto& t : tasks) {
        if (t->status == TaskStatus::NotStarted) continue;
        
        hasAnyTask = true;
        
        QFrame *card = new QFrame(m_scrollContent);
        card->setFrameShape(QFrame::StyledPanel);
        card->setStyleSheet("QFrame { background-color: #3c3f41; border: 1px solid #555; border-radius: 5px; margin: 5px; padding: 5px; }");
        
        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(10, 10, 10, 10);
        cardLayout->setSpacing(5);
        
        // 任务名称
        QLabel *nameLabel = new QLabel(QString::fromStdString(t->description), card);
        nameLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #a9b7c6; border: none;");
        cardLayout->addWidget(nameLabel);
        
        // 状态
        QString statusStr;
        QString statusColor;
        if (t->status == TaskStatus::InProgress) {
            statusStr = "进行中";
            statusColor = "#e6a23c"; // orange
        } else if (t->status == TaskStatus::Completed) {
            statusStr = "已完成，请找 [" + QString::fromStdString(t->submitNPC) + "] 提交";
            statusColor = "#67c23a"; // green
        } else if (t->status == TaskStatus::Submitted) {
            statusStr = "已完成";
            statusColor = "#909399"; // gray
        }
        
        QLabel *statusLabel = new QLabel("状态: " + statusStr, card);
        statusLabel->setStyleSheet(QString("color: %1; border: none; font-size: 14px;").arg(statusColor));
        cardLayout->addWidget(statusLabel);
        
        // 目标进度
        if (t->status == TaskStatus::InProgress || t->status == TaskStatus::Completed) {
            for (const auto& obj : t->objectives) {
                QString target = QString::fromStdString(obj.targetName);
                if (obj.type == TaskType::BuyHealItem) target = "回复道具";
                else if (obj.type == TaskType::AnyBuy) target = "任意物品";
                
                QString objStr = QString("- %1: %2/%3").arg(target).arg(obj.currentAmount).arg(obj.targetAmount);
                QLabel *objLabel = new QLabel(objStr, card);
                objLabel->setStyleSheet("color: #cccccc; border: none; font-size: 13px;");
                cardLayout->addWidget(objLabel);
            }
        }
        
        // 奖励
        QString rewardStr = "奖励: ";
        if (t->rewardGold > 0) rewardStr += QString("金币 x%1  ").arg(t->rewardGold);
        if (t->rewardExp > 0) rewardStr += QString("经验 x%1  ").arg(t->rewardExp);
        for (const auto& item : t->rewardItems) {
            rewardStr += QString::fromStdString(item->getName()) + "  ";
        }
        QLabel *rewardLabel = new QLabel(rewardStr, card);
        rewardLabel->setStyleSheet("color: #e5c07b; border: none; font-size: 13px;");
        cardLayout->addWidget(rewardLabel);

        // ── [追踪] 按钮（仅进行中任务）────────────────────────────────────────
        if (t->status == TaskStatus::InProgress) {
            QPushButton *trackBtn = new QPushButton("📍 追踪", card);
            trackBtn->setStyleSheet(
                "QPushButton { background: #3a6186; color: white; border-radius: 4px;"
                "              padding: 4px 14px; font-size: 13px; border: none; margin-top: 4px; }"
                "QPushButton:hover { background: #4a81b6; }"
                "QPushButton:pressed { background: #2a5170; }");
            int capturedId = t->id;
            connect(trackBtn, &QPushButton::clicked, this, [this, capturedId]() {
                emit trackTask(capturedId);  // 通知 MainWindow
                this->hide();               // 关闭任务窗口，打开大地图
            });
            cardLayout->addWidget(trackBtn);
        }
        
        m_scrollLayout->addWidget(card);
    }

    if (!hasAnyTask) {
        QLabel *emptyLabel = new QLabel("暂无进行的任务。\n\n去校园里找NPC对话看看吧！", m_scrollContent);
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("color: #888888; font-size: 16px; margin-top: 50px; border: none;");
        m_scrollLayout->addWidget(emptyLabel);
    }
}
