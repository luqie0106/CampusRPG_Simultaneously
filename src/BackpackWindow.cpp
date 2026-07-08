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

    // 获取背包物品
    const auto& backpackItems = m_engine->GetBackpackItems();

    // 构建"已装备"槽位映射：装备名称 → 槽位（用于判断某物品是否已被装备）
    EquipSlot eqSlots[] = { EquipSlot::Weapon, EquipSlot::Head, EquipSlot::Body, EquipSlot::Legs, EquipSlot::Feet };
    QMap<QString, EquipSlot> equippedNameToSlot;
    for (int i = 0; i < 5; ++i) {
        auto eq = m_engine->GetPlayer()->GetEquipmentAt(eqSlots[i]);
        if (eq) {
            equippedNameToSlot[QString::fromStdString(eq->getName())] = eqSlots[i];
        }
    }

    if (backpackItems.empty() && equippedNameToSlot.isEmpty()) {
        QLabel *empty = new QLabel("背包空空如也", m_scrollContent);
        empty->setStyleSheet("color: white; font-size: 16px;");
        m_scrollLayout->addWidget(empty);
        return;
    }

    // 统计背包中每种物品的数量，记录首次出现索引
    QMap<QString, int> itemCountMap;
    QMap<QString, int> firstIndexMap;
    for (int i = 0; i < (int)backpackItems.size(); ++i) {
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

    auto addCard = [&](const QString& itemName, int count, int itemIndex, bool isEquipped, EquipSlot equipSlot) {
        int row = displayIndex / cols;
        int col = displayIndex % cols;
        displayIndex++;

        QWidget *card = new QWidget(m_scrollContent);
        card->setFixedSize(150, 200);
        if (isEquipped) {
            card->setStyleSheet(
                "QWidget { background-color: #2a3a2a; border-radius: 8px; border: 2px solid #4CAF50; }");
        } else {
            card->setStyleSheet(
                "QWidget { background-color: #3a3a3a; border-radius: 8px; border: 1px solid #555; }");
        }

        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(8, 8, 8, 8);
        cardLayout->setSpacing(4);

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

        QLabel *nameLabel = new QLabel(itemName, card);
        nameLabel->setStyleSheet(isEquipped ? "color: #4CAF50; font-size: 11px; font-weight: bold;" : "color: white; font-size: 11px;");
        nameLabel->setAlignment(Qt::AlignCenter);
        nameLabel->setWordWrap(true);
        nameLabel->setFixedHeight(32);
        cardLayout->addWidget(nameLabel);

        QLabel *countLabel = new QLabel(isEquipped ? "✓ 已装备" : QString("数量: %1").arg(count), card);
        countLabel->setStyleSheet(isEquipped ? "color: #4CAF50; font-size: 12px; font-weight: bold;" : "color: #aaa; font-size: 12px; font-weight: bold;");
        countLabel->setAlignment(Qt::AlignCenter);
        cardLayout->addWidget(countLabel);

        QHBoxLayout *btnRow = new QHBoxLayout();
        QPushButton *useBtn = new QPushButton(isEquipped ? "脱下" : "使用", card);
        useBtn->setFixedSize(60, 24);
        useBtn->setEnabled(count > 0 || isEquipped);
        if (isEquipped) {
            useBtn->setStyleSheet(
                "QPushButton { background-color: #FF9800; color: white; border-radius: 3px; font-size: 11px; }"
                "QPushButton:hover { background-color: #FFB74D; }"
            );
        } else {
            useBtn->setStyleSheet(
                "QPushButton { background-color: #4CAF50; color: white; border-radius: 3px; font-size: 11px; }"
                "QPushButton:hover { background-color: #66BB6A; }"
                "QPushButton:disabled { background-color: #666; color: #999; }");
        }

        connect(useBtn, &QPushButton::clicked, this, [this, itemIndex, isEquipped, equipSlot]() {
            if (!m_engine || m_engine->GetPlayer() == nullptr) {
                QMessageBox::warning(this, "错误", "请先创建角色");
                return;
            }

            if (isEquipped) {
                // 脱下装备：只清除装备槽，物品仍留在背包 items
                m_engine->GetPlayer()->UnequipItem(equipSlot);
                refreshBackpack();
            } else if (itemIndex > 0) {
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

        if (!isEquipped) {
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
        }

        cardLayout->addLayout(btnRow);
        grid->addWidget(card, row, col);
    };

    // 遍历背包物品，已装备的同名物品标记为已装备状态（每种只标记一次）
    QSet<QString> markedAsEquipped; // 已标记为已装备的物品名称集合
    for (auto it = firstIndexMap.constBegin(); it != firstIndexMap.constEnd(); ++it) {
        QString itemName = it.key();
        int count = itemCountMap.value(itemName, 0);
        int itemIndex = it.value();

        bool isEquipped = false;
        EquipSlot equipSlot = EquipSlot::Head; // 默认值

        // 检查是否已装备（且尚未被标记过）
        if (equippedNameToSlot.contains(itemName) && !markedAsEquipped.contains(itemName)) {
            isEquipped = true;
            equipSlot = equippedNameToSlot[itemName];
            markedAsEquipped.insert(itemName);
            // 已装备的物品显示数量要减1（因为有一个是已装备的）
            int displayCount = count - 1;
            addCard(itemName, displayCount, itemIndex, true, equipSlot);
            // 如果还有剩余数量（count > 1），再显示一个未装备的卡片
            if (displayCount > 0) {
                addCard(itemName, displayCount, itemIndex, false, EquipSlot::Head);
            }
        } else {
            addCard(itemName, count, itemIndex, false, EquipSlot::Head);
        }
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
