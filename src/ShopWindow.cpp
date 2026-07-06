#include "Common.h"
#include "../include/ShopWindow.h"
#include <QDir>
#include <QFileInfo>
#include <QScrollArea>
#include <QGridLayout>
#include <QPushButton>
#include <QMessageBox>

ShopWindow::ShopWindow(GameEngine *engine, QWidget *parent)
    : QWidget(parent), m_engine(engine) {
    setWindowTitle("校园商店");
    setMinimumSize(700, 500);
    setStyleSheet("background-color: #2c2c2c;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 顶部：标题 + 金币
    QHBoxLayout *topBar = new QHBoxLayout();
    QLabel *title = new QLabel("校园商店", this);
    title->setStyleSheet("color: white; font-size: 22px; font-weight: bold;");
    topBar->addWidget(title);
    topBar->addStretch();
    m_goldLabel = new QLabel(this);
    m_goldLabel->setStyleSheet("color: gold; font-size: 18px;");
    topBar->addWidget(m_goldLabel);
    mainLayout->addLayout(topBar);

    // 滚动区域
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: none; background: transparent; }");

    m_scrollContent = new QWidget();
    m_scrollLayout = new QVBoxLayout(m_scrollContent);
    m_scrollLayout->setAlignment(Qt::AlignTop);
    m_scrollLayout->setSpacing(8);
    m_scrollLayout->setContentsMargins(10, 10, 10, 10);
    scrollArea->setWidget(m_scrollContent);
    mainLayout->addWidget(scrollArea);

    // 底部关闭按钮
    QPushButton *closeBtn = new QPushButton("关闭商店 (Esc)", this);
    closeBtn->setFixedHeight(40);
    closeBtn->setStyleSheet(
        "QPushButton { background-color: #555; color: white; font-size: 16px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #777; }");
    connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);
    mainLayout->addWidget(closeBtn);
}

ShopWindow::~ShopWindow() {}

void ShopWindow::loadItemsFromDirectory(const QString &dirPath) {
    m_entries.clear();
    QDir dir(dirPath);
    if (!dir.exists()) {
        qWarning() << "物品目录不存在:" << dirPath;
        return;
    }

    QStringList filters;
    filters << "*.png";
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files, QDir::Name);

    for (const QFileInfo &fi : fileList) {
        // 从文件名生成显示名（去掉扩展名，下划线替换为空格，首字母大写）
        QString rawName = fi.baseName();
        QString displayName = rawName.replace('_', ' ');
        // 简单首字母大写
        if (!displayName.isEmpty()) {
            displayName[0] = displayName[0].toUpper();
        }

        ShopEntry entry;
        entry.name = displayName;
        entry.imagePath = fi.absoluteFilePath();
        entry.price = 10; // 默认价格，后续可按需调整
        m_entries.append(entry);
    }

    buildUI();
}

void ShopWindow::buildUI() {
    // 清除旧内容
    QLayoutItem *child;
    while ((child = m_scrollLayout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }

    if (m_entries.isEmpty()) {
        QLabel *empty = new QLabel("商店暂无商品", m_scrollContent);
        empty->setStyleSheet("color: white; font-size: 16px;");
        m_scrollLayout->addWidget(empty);
        return;
    }

    // 每行显示 4 个商品
    const int cols = 4;
    int rows = (m_entries.size() + cols - 1) / cols;

    QGridLayout *grid = new QGridLayout();
    grid->setSpacing(12);

    for (int i = 0; i < m_entries.size(); ++i) {
        const ShopEntry &entry = m_entries[i];
        int row = i / cols;
        int col = i % cols;

        // 卡片容器
        QWidget *card = new QWidget(m_scrollContent);
        card->setFixedSize(150, 180);
        card->setStyleSheet(
            "QWidget { background-color: #3a3a3a; border-radius: 8px; border: 1px solid #555; }");

        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(8, 8, 8, 8);
        cardLayout->setSpacing(4);

        // 物品图片
        QLabel *imgLabel = new QLabel(card);
        QPixmap pix(entry.imagePath);
        if (!pix.isNull()) {
            pix = pix.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            imgLabel->setPixmap(pix);
        } else {
            imgLabel->setText("?");
        }
        imgLabel->setAlignment(Qt::AlignCenter);
        imgLabel->setFixedHeight(72);
        cardLayout->addWidget(imgLabel);

        // 物品名称
        QLabel *nameLabel = new QLabel(entry.name, card);
        nameLabel->setStyleSheet("color: white; font-size: 11px;");
        nameLabel->setAlignment(Qt::AlignCenter);
        nameLabel->setWordWrap(true);
        nameLabel->setFixedHeight(32);
        cardLayout->addWidget(nameLabel);

        // 价格 + 购买按钮
        QHBoxLayout *bottomRow = new QHBoxLayout();
        QLabel *priceLabel = new QLabel(QString("%1G").arg(entry.price), card);
        priceLabel->setStyleSheet("color: gold; font-size: 12px;");
        bottomRow->addWidget(priceLabel);
        bottomRow->addStretch();

        QPushButton *buyBtn = new QPushButton("购买", card);
        buyBtn->setFixedSize(50, 24);
        buyBtn->setStyleSheet(
            "QPushButton { background-color: #4CAF50; color: white; border-radius: 3px; font-size: 11px; }"
            "QPushButton:hover { background-color: #66BB6A; }");

        int itemIndex = i; // 捕获当前索引
        connect(buyBtn, &QPushButton::clicked, this, [this, itemIndex]() {
            if (!m_engine || m_engine->GetPlayer() == nullptr) {
                QMessageBox::warning(this, "错误", "请先创建角色");
                return;
            }

            const ShopEntry &entry = m_entries[itemIndex];
            int playerGold = m_engine->GetPlayerGold();

            if (playerGold < entry.price) {
                QMessageBox::warning(this, "金币不足",
                    QString("需要 %1 金币，你只有 %2 金币").arg(entry.price).arg(playerGold));
                return;
            }

            // 扣除金币
            m_engine->GetPlayer()->SpendGold(entry.price);

            // 创建物品并添加到背包
            std::unique_ptr<Item> newItem = std::make_unique<Item>(
                entry.name.toStdString(), entry.price);
            m_engine->GetPlayer()->GetBackpack().AddItem(std::move(newItem));

            QMessageBox::information(this, "购买成功",
                QString("购买了 %1，花费 %2 金币").arg(entry.name).arg(entry.price));
            refreshShopList();
        });

        bottomRow->addWidget(buyBtn);
        cardLayout->addLayout(bottomRow);

        grid->addWidget(card, row, col);
    }

    m_scrollLayout->addLayout(grid);
    refreshShopList();
}

void ShopWindow::refreshShopList() {
    if (m_engine && m_engine->GetPlayer() != nullptr) {
        m_goldLabel->setText(QString("金币: %1").arg(m_engine->GetPlayerGold()));
    } else {
        m_goldLabel->setText("金币: 0");
    }
}
