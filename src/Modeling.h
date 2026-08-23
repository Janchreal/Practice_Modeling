#pragma once
#include "ui_Modeling.h"
#include <QMainWindow>

class Modeling : public QMainWindow {
    Q_OBJECT
    
public:
    Modeling(QWidget* parent = nullptr);
    ~Modeling();

private:
    Ui_Modeling* ui;
};