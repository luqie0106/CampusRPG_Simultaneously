#pragma once

#include "Common.h"
#include "GameEngine.h"

class ShopWindow : public QWidget {
    Q_OBJECT
public:
    explicit ShopWindow(GameEngine *engine, bool isBlackMarket = false, QWidget *parent = nullptr);
    ~ShopWindow();

    void loadItemsFromEngine();

private:
    QString getImagePath(const QString& itemName);
    void buildUI();
    void refreshShopList();

    GameEngine *m_engine;
    QWidget *m_scrollContent;
    QVBoxLayout *m_scrollLayout;
    QLabel *m_goldLabel;
    bool m_isBlackMarket;

    struct ShopEntry {
        QString name;
        QString imagePath;
        int price;
    };
    QVector<ShopEntry> m_entries;
};
