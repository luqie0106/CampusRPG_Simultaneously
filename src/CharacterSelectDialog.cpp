#include "Common.h"
#include <QLineEdit>
#include <QRadioButton>
#include <QGroupBox>

#include "../include/CharacterSelectDialog.h"

CharacterSelectDialog::CharacterSelectDialog(GameEngine *engine, QWidget *parent)
    : QDialog(parent), m_engine(engine), m_selectedClass(3), m_created(false) {
    setWindowTitle("选择角色");
    setMinimumSize(600, 450);
    setModal(true);
    setStyleSheet("background-color: #2c2c2c;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // 标题
    QLabel *title = new QLabel("校园RPG - 选择你的角色", this);
    title->setStyleSheet("color: white; font-size: 22px; font-weight: bold;");
    title->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(title);

    // 职业选择说明
    const QString classText = m_engine->GetClassSelectionText().c_str();
    QLabel *classLabel = new QLabel(classText, this);
    classLabel->setStyleSheet("color: #ddd; font-size: 13px; background-color: #3a3a3a; padding: 10px; border-radius: 5px;");
    classLabel->setWordWrap(true);
    mainLayout->addWidget(classLabel);

    // 角色名输入
    QHBoxLayout *nameRow = new QHBoxLayout();
    QLabel *nameLbl = new QLabel("角色名称：", this);
    nameLbl->setStyleSheet("color: white; font-size: 16px;");
    nameRow->addWidget(nameLbl);
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("请输入角色名称...");
    m_nameEdit->setMaxLength(20);
    m_nameEdit->setFixedHeight(36);
    m_nameEdit->setStyleSheet("QLineEdit { background-color: #444; color: white; border: 1px solid #666; border-radius: 4px; padding: 0 8px; font-size: 14px; }");
    nameRow->addWidget(m_nameEdit);
    mainLayout->addLayout(nameRow);

    // 职业单选按钮
    QGroupBox *classGroup = new QGroupBox("选择职业", this);
    classGroup->setStyleSheet("QGroupBox { color: white; font-size: 15px; border: 1px solid #555; border-radius: 5px; margin-top: 8px; padding-top: 10px; }");
    QVBoxLayout *classLayout = new QVBoxLayout(classGroup);

    QRadioButton *rb1 = new QRadioButton("体育生 (Athlete) - HP:35 ATK:16 DEF:3 Dodge:5%", this);
    rb1->setStyleSheet("color: #eee; font-size: 14px; padding: 6px 8px; border-radius: 4px; border: 1px solid transparent;");
    rb1->setChecked(false);
    classLayout->addWidget(rb1);

    QRadioButton *rb2 = new QRadioButton("学霸 (Nerd) - HP:14 ATK:8 DEF:10 Dodge:25%", this);
    rb2->setStyleSheet("color: #eee; font-size: 14px; padding: 6px 8px; border-radius: 4px; border: 1px solid transparent;");
    classLayout->addWidget(rb2);

    QRadioButton *rb3 = new QRadioButton("普通学生 (Steve) - HP:20 ATK:10 DEF:5 Dodge:12% [推荐]", this);
    rb3->setStyleSheet("color: #4CAF50; font-size: 14px; font-weight: bold; padding: 6px 8px; background-color: #2e4a2e; border: 2px solid #4CAF50; border-radius: 4px;");
    rb3->setChecked(true);
    classLayout->addWidget(rb3);

    auto resetAll = [rb1, rb2, rb3]() {
        rb1->setStyleSheet("QRadioButton { color: #eee; font-size: 14px; padding: 6px 8px; border-radius: 4px; border: 1px solid transparent; }");
        rb2->setStyleSheet("QRadioButton { color: #eee; font-size: 14px; padding: 6px 8px; border-radius: 4px; border: 1px solid transparent; }");
        rb3->setStyleSheet("QRadioButton { color: #eee; font-size: 14px; padding: 6px 8px; border-radius: 4px; border: 1px solid transparent; }");
    };
    auto highlight = [](QRadioButton *btn) {
        btn->setStyleSheet("QRadioButton { color: #4CAF50; font-size: 14px; font-weight: bold; padding: 6px 8px; background-color: #2e4a2e; border: 2px solid #4CAF50; border-radius: 4px; }");
    };

    connect(rb1, &QRadioButton::toggled, this, [this, rb1, rb2, rb3, resetAll, highlight](bool checked){
        if (checked) { m_selectedClass = 1; resetAll(); highlight(rb1); }
    });
    connect(rb2, &QRadioButton::toggled, this, [this, rb1, rb2, rb3, resetAll, highlight](bool checked){
        if (checked) { m_selectedClass = 2; resetAll(); highlight(rb2); }
    });
    connect(rb3, &QRadioButton::toggled, this, [this, rb1, rb2, rb3, resetAll, highlight](bool checked){
        if (checked) { m_selectedClass = 3; resetAll(); highlight(rb3); }
    });

    mainLayout->addWidget(classGroup);

    // 确认按钮
    QPushButton *confirmBtn = new QPushButton("开始冒险！", this);
    confirmBtn->setFixedHeight(44);
    confirmBtn->setStyleSheet(
        "QPushButton { background-color: #4CAF50; color: white; font-size: 17px; font-weight: bold; border-radius: 6px; }"
        "QPushButton:hover { background-color: #66BB6A; }"
        "QPushButton:pressed { background-color: #388E3C; }");
    connect(confirmBtn, &QPushButton::clicked, this, [this]() {
        QString name = m_nameEdit->text().trimmed();
        if (name.isEmpty()) {
            QMessageBox::warning(this, "提示", "请输入角色名称！");
            return;
        }
        std::string result = m_engine->CreatePlayer(name.toStdString(), m_selectedClass);
        if (result.find("错误") != std::string::npos) {
            QMessageBox::critical(this, "创建失败", result.c_str());
            return;
        }
        m_created = true;
        accept();
    });
    mainLayout->addWidget(confirmBtn);
}

CharacterSelectDialog::~CharacterSelectDialog() {}
