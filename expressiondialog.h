#ifndef EXPRESSIONDIALOG_H
#define EXPRESSIONDIALOG_H

#include <QDialog>

// 前向声明
class Widget;

namespace Ui {
class ExpressionDialog;
}

class ExpressionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExpressionDialog(Widget *parent = nullptr);
    ~ExpressionDialog();

    // 更新模型参数显示
    void updateModelParameters();

private slots:
    void on_addExpressionBtn_clicked();
    void on_modifyExpressionBtn_clicked();
    void on_applyBtn_clicked();
    void on_okBtn_clicked();
    void on_cancelBtn_clicked();
    void on_controlGroupTable_itemSelectionChanged();

private:
    Ui::ExpressionDialog *ui;
    Widget *parentWidget;  // 指向主窗口的指针，用于访问模型数据

    void initializeControlGroup();
    void updateControlGroupTable();
};

#endif // EXPRESSIONDIALOG_H
