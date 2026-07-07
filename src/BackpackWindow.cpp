#include "Common.h"
#include "../include/BackpackWindow.h"

BackpackWindow::BackpackWindow(GameEngine *engine, QWidget *parent)
    : QWidget(parent), m_engine(engine) {
    setWindowTitle("背包");
    setMinimumSize(700, 500);
    setStyleSheet("background-color: #2c2c2c;");
    setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 顶部：标题 + 金币
    QHBoxLayout *topBar = new QHBoxLayout();
    QLabel *title = new QLabel("我的背包", this);
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
    QPushButton *closeBtn = new QPushButton("关闭背包 (Esc)", this);
    closeBtn->setFixedHeight(40);
    closeBtn->setStyleSheet(
        "QPushButton { background-color: #555; color: white; font-size: 16px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #777; }");
    connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);
    mainLayout->addWidget(closeBtn);

    refreshBackpack();
}

BackpackWindow::~BackpackWindow() {}

void BackpackWindow::refreshBackpack() {
    // 清除旧内容
    QLayoutItem *child;
    while ((child = m_scrollLayout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }

    if (!m_engine || m_engine->GetPlayer() == nullptr) {
        QLabel *empty = new QLabel("请先创建角色", m_scrollContent);
        empty->setStyleSheet("color: white; font-size: 16px;");
        m_scrollLayout->addWidget(empty);
        m_goldLabel->setText("金币: 0");
        return;
    }

    m_goldLabel->setText(QString("金币: %1").arg(m_engine->GetPlayerGold()));

    // 获取商店物品列表和背包物品
    const auto& shopItems = m_engine->GetShopItemList();
    const auto& backpackItems = m_engine->GetBackpackItems();

    if (shopItems.empty()) {
        QLabel *empty = new QLabel("背包空空如也", m_scrollContent);
        empty->setStyleSheet("color: white; font-size: 16px;");
        m_scrollLayout->addWidget(empty);
        return;
    }

    // 统计背包中每种物品的数量
    QMap<QString, int> itemCountMap;
    QMap<QString, int> firstIndexMap; // 记录每种物品第一次出现的索引（1-based）
    for (int i = 0; i < backpackItems.size(); ++i) {
        const auto& item = backpackItems[i];
        if (!item) continue;
        QString name = QString::fromStdString(item->getName());
        if (!itemCountMap.contains(name)) {
            itemCountMap[name] = 0;
            firstIndexMap[name] = i + 1; // 1-based
        }
        itemCountMap[name]++;
    }

    // 每行显示 4 个物品
    const int cols = 4;
    QGridLayout *grid = new QGridLayout();
    grid->setSpacing(12);

    int displayIndex = 0;
    for (int i = 0; i < shopItems.size(); ++i) {
        const auto& shopItem = shopItems[i];
        if (!shopItem.item) continue;

        QString itemName = QString::fromStdString(shopItem.item->getName());
        int count = itemCountMap.value(itemName, 0);

        int row = displayIndex / cols;
        int col = displayIndex % cols;
        displayIndex++;

        // 卡片容器
        QWidget *card = new QWidget(m_scrollContent);
        card->setFixedSize(150, 200);
        card->setStyleSheet(
            "QWidget { background-color: #3a3a3a; border-radius: 8px; border: 1px solid #555; }");

        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(8, 8, 8, 8);
        cardLayout->setSpacing(4);

        // 物品图片
        QString imagePath = getImagePath(itemName);
        QLabel *imgLabel = new QLabel(card);
        QPixmap pix(imagePath);
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
        QLabel *nameLabel = new QLabel(itemName, card);
        nameLabel->setStyleSheet("color: white; font-size: 11px;");
        nameLabel->setAlignment(Qt::AlignCenter);
        nameLabel->setWordWrap(true);
        nameLabel->setFixedHeight(32);
        cardLayout->addWidget(nameLabel);

        // 数量显示
        QLabel *countLabel = new QLabel(QString("数量: %1").arg(count), card);
        countLabel->setStyleSheet("color: #aaa; font-size: 12px; font-weight: bold;");
        countLabel->setAlignment(Qt::AlignCenter);
        cardLayout->addWidget(countLabel);

        // 使用按钮（只有数量>0时才可点击）
        QHBoxLayout *btnRow = new QHBoxLayout();
        QPushButton *useBtn = new QPushButton("使用", card);
        useBtn->setFixedSize(60, 24);
        useBtn->setEnabled(count > 0);
        useBtn->setStyleSheet(
            "QPushButton { background-color: #4CAF50; color: white; border-radius: 3px; font-size: 11px; }"
            "QPushButton:hover { background-color: #66BB6A; }"
            "QPushButton:disabled { background-color: #666; color: #999; }");

        int itemIndex = firstIndexMap.value(itemName, 0);
        connect(useBtn, &QPushButton::clicked, this, [this, itemIndex, itemName]() {
            if (!m_engine || m_engine->GetPlayer() == nullptr) {
                QMessageBox::warning(this, "错误", "请先创建角色");
                return;
            }

            if (itemIndex > 0) {
                std::string result;
                if (m_engine->GetState() == GameState::Battle) {
                    result = m_engine->BattleUseItemBeforeAction(itemIndex);
                    emit battleItemUsed(QString::fromStdString(result));
                } else {
                    result = m_engine->UseBackpackItem(itemIndex);
                    QMessageBox::information(this, "使用物品", QString::fromStdString(result));
                }
                refreshBackpack();
            }
        });

        btnRow->addWidget(useBtn);
        btnRow->addStretch();

        // 丢弃按钮（只有数量>0时才可点击）
        QPushButton *dropBtn = new QPushButton("丢弃", card);
        dropBtn->setFixedSize(60, 24);
        dropBtn->setEnabled(count > 0);
        dropBtn->setStyleSheet(
            "QPushButton { background-color: #f44336; color: white; border-radius: 3px; font-size: 11px; }"
            "QPushButton:hover { background-color: #ef5350; }"
            "QPushButton:disabled { background-color: #666; color: #999; }");

        connect(dropBtn, &QPushButton::clicked, this, [this, itemIndex, itemName]() {
            if (!m_engine || m_engine->GetPlayer() == nullptr) {
                QMessageBox::warning(this, "错误", "请先创建角色");
                return;
            }

            if (itemIndex > 0) {
                QMessageBox::StandardButton reply = QMessageBox::question(
                    this, "确认丢弃",
                    QString("确定要丢弃 %1 吗？").arg(itemName),
                    QMessageBox::Yes | QMessageBox::No);

                if (reply == QMessageBox::Yes) {
                    std::string result = m_engine->RemoveBackpackItem(itemIndex);
                    QMessageBox::information(this, "丢弃物品", QString::fromStdString(result));
                    refreshBackpack();
                }
            }
        });

        btnRow->addWidget(dropBtn);
        cardLayout->addLayout(btnRow);

        grid->addWidget(card, row, col);
    }

    m_scrollLayout->addLayout(grid);
}

QString BackpackWindow::getImagePath(const QString& itemName) {
    QString base = QString(PROJECT_DATA_DIR) + "/items/";

    // Food & Potions
    if (itemName.contains("猪肉") || itemName.contains("Pork")) return base + "porkchop_cooked.png";
    if (itemName.contains("牛排") || itemName.contains("Steak")) return base + "beef_cooked.png";
    if (itemName.contains("金苹果") || itemName.contains("Apple")) return base + "apple_golden.png";
    if (itemName.contains("药水") || itemName.contains("Potion")) return base + "potion_bottle_drinkable.png";

    // Iron Equipments
    if (itemName.contains("铁剑") || itemName.contains("Iron Sword")) return base + "iron_sword.png";
    if (itemName.contains("铁头盔") || itemName.contains("Iron Helmet")) return base + "iron_helmet.png";
    if (itemName.contains("铁胸甲") || itemName.contains("Iron Armor")) return base + "iron_chestplate.png";
    if (itemName.contains("铁护腿") || itemName.contains("Iron Leggings")) return base + "iron_leggings.png";
    if (itemName.contains("铁靴") || itemName.contains("Iron Boots")) return base + "iron_boots.png";

    // Gold Equipments
    if (itemName.contains("金剑") || itemName.contains("Golden Sword")) return base + "gold_sword.png";
    if (itemName.contains("金头盔") || itemName.contains("Golden Helmet")) return base + "gold_helmet.png";
    if (itemName.contains("金胸甲") || itemName.contains("Golden Armor")) return base + "gold_chestplate.png";
    if (itemName.contains("金护腿") || itemName.contains("Golden Leggings")) return base + "gold_leggings.png";
    if (itemName.contains("金靴") || itemName.contains("Golden Boots")) return base + "gold_boots.png";

    // Diamond Equipments
    if (itemName.contains("钻石剑") || itemName.contains("Diamond Sword")) return base + "diamond_sword.png";
    if (itemName.contains("钻石头盔") || itemName.contains("Diamond Helmet")) return base + "diamond_helmet.png";
    if (itemName.contains("钻石胸甲") || itemName.contains("Diamond Armor")) return base + "diamond_chestplate.png";
    if (itemName.contains("钻石护腿") || itemName.contains("Diamond Leggings")) return base + "diamond_leggings.png";
    if (itemName.contains("钻石靴") || itemName.contains("Diamond Boots")) return base + "diamond_boots.png";

    return base + "items.png";
}
