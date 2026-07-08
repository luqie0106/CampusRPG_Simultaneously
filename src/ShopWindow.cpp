#include "Common.h"
#include "../include/ShopWindow.h"

ShopWindow::ShopWindow(GameEngine *engine, bool isBlackMarket, QWidget *parent)
    : QWidget(parent), m_engine(engine), m_isBlackMarket(isBlackMarket) {
    setWindowTitle(m_isBlackMarket ? "神秘黑市" : "校园商店");
    setMinimumSize(700, 500);
    setStyleSheet("background-color: #2c2c2c;");
    setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 顶部：标题 + 金币
    QHBoxLayout *topBar = new QHBoxLayout();
    QLabel *title = new QLabel(m_isBlackMarket ? "神秘黑市" : "校园商店", this);
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

void ShopWindow::loadItemsFromEngine() {
    m_entries.clear();
    
    if (!m_engine) return;
    
    if (m_isBlackMarket) {
        const auto& shopItems = m_engine->GetBlackMarketItems();
        for (const auto& shopItem : shopItems) {
            if (!shopItem.item) continue;
            
            ShopEntry entry;
            entry.name = QString::fromStdString(shopItem.item->getName());
            entry.price = shopItem.item->getValue();
            entry.imagePath = getImagePath(entry.name);
            m_entries.append(entry);
        }
    } else {
        const auto& shopItems = m_engine->GetShopItemList();
        for (const auto& shopItem : shopItems) {
            if (!shopItem.item) continue;
            
            ShopEntry entry;
            entry.name = QString::fromStdString(shopItem.item->getName());
            entry.price = shopItem.item->getValue();
            entry.imagePath = getImagePath(entry.name);
            m_entries.append(entry);
        }
    }

    buildUI();
}

QString ShopWindow::getImagePath(const QString& itemName) {
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

            std::string result;
            if (m_isBlackMarket) {
                result = m_engine->BuyBlackMarketItem(itemIndex + 1);
            } else {
                result = m_engine->BuyItem(itemIndex + 1);
            }
            QMessageBox::information(this, "购买结果", QString::fromStdString(result));
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
