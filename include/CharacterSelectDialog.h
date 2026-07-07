#pragma once

#include "Common.h"
#include "GameEngine.h"

class QLineEdit;

class CharacterSelectDialog : public QDialog {
    Q_OBJECT
public:
    explicit CharacterSelectDialog(GameEngine *engine, QWidget *parent = nullptr);
    ~CharacterSelectDialog();

    // 返回是否成功创建角色
    bool isPlayerCreated() const { return m_created; }

    // 返回选中的职业 (1=Athlete, 2=Nerd, 3=Steve)
    int selectedClass() const { return m_selectedClass; }

private:
    void buildUI();

    GameEngine *m_engine;
    QLineEdit *m_nameEdit;
    int m_selectedClass; // 1=Athlete, 2=Nerd, 3=Steve
    bool m_created;
};
