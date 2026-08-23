#include "Modeling.h"

Modeling::Modeling(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui_Modeling)
{
    ui->setupUi(this);
}

Modeling::~Modeling()
{
    delete ui; 
}