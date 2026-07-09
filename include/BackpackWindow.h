#pragma once

#include "Common.h"
#include "GameEngine.h"

class BackpackWindow : public QWidget {
    Q_OBJECT
public:
    explicit BackpackWindow(GameEngine *engine, QWidget *parent = nullptr);
    ~BackpackWindow();

    void refreshBackpack();

signals:
    void battleItemUsed(const QString& resultLog);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    QString getImagePath(const QString& itemName);
    void buildUI();

    GameEngine *m_engine;
    QWidget *m_scrollContent;
    QVBoxLayout *m_scrollLayout;
    QLabel *m_goldLabel;
};
