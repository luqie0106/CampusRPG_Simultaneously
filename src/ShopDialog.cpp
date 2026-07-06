#include "Common.h"
#include "../include/ShopDialog.h"

ShopDialog::ShopDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("商店");
    setFixedSize(300, 120);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *label = new QLabel("是否进入商店？", this);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("font-size: 16px; margin: 10px;");
    layout->addWidget(label);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *yesBtn = new QPushButton("是 (Y)", this);
    QPushButton *noBtn = new QPushButton("否 (N)", this);
    yesBtn->setFixedSize(100, 36);
    noBtn->setFixedSize(100, 36);
    btnLayout->addStretch();
    btnLayout->addWidget(yesBtn);
    btnLayout->addWidget(noBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    connect(yesBtn, &QPushButton::clicked, this, [this]() {
        emit accepted();
        accept();
    });
    connect(noBtn, &QPushButton::clicked, this, [this]() {
        emit rejected();
        reject();
    });
}
