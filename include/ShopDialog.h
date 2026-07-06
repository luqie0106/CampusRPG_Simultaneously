#pragma once

#include "Common.h"

class ShopDialog : public QDialog {
    Q_OBJECT
public:
    explicit ShopDialog(QWidget *parent = nullptr);

signals:
    void accepted();
    void rejected();
};
